#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dac1220.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "rom/ets_sys.h"

DAC1220::DAC1220(BinMode bin_mode, int sck, int miso, int mosi, int cs, uint32_t freq)
    : bin_mode(bin_mode), sckPin(sck), misoPin(miso), mosiPin(mosi), csPin(cs), frequency(freq) {
}

void DAC1220::set_bin_mode(BinMode bin_mode) {
  if (bin_mode == StraightBinary) {
    cmr |= (1 << CMR_DF);   // set the bit
  } else {
     cmr &= ~(1 << CMR_DF);  // clear the bit
  }
  set_command_register(cmr);
}


void DAC1220::begin() {
  reset_all();
  initSPI();
  
  // Ideally apply the reset pattern, which enters the Normal mode when complete.
  // but this does all sorts of wacky bit banging so see how we go without it
  // reset_all();

  // Set configuration to 20-bit, straight binary mode.
  cmr |= (1 << CMR_RES);
  set_command_register(cmr);

  Serial.printf("Setting mode %d\n", bin_mode);
  set_bin_mode(bin_mode);

  // Calibrate with the output connected.
  calibrate(true);
}

void DAC1220::reset_all() {
    // Ensure SPI is not in use
    spi_bus_free(HSPI_HOST);

	auto gcs = static_cast<gpio_num_t>(csPin);
	auto gsk = static_cast<gpio_num_t>(sckPin);
    // Configure CS pin as output
    gpio_set_direction(gcs, GPIO_MODE_OUTPUT);
    gpio_set_level(gcs, 0); // Activate DAC1220 CS

    // Manually reset DAC by toggling SCK
    gpio_set_direction(gsk, GPIO_MODE_OUTPUT);

    // Reset sequence
    gpio_set_level(gsk, 0);
    ets_delay_us(1000); // Third high period
    // vTaskDelay(pdMS_TO_TICKS(1)); // Ensure the DAC1220 recognizes the low state
    gpio_set_level(gsk, 1);
    ets_delay_us(240); // First high period
    gpio_set_level(gsk, 0);
    ets_delay_us(5); // Short low period
    gpio_set_level(gsk, 1);
    ets_delay_us(480); // Second high period
    gpio_set_level(gsk, 0);
    ets_delay_us(5); // Short low period
    gpio_set_level(gsk, 1);
    ets_delay_us(960); // Third high period
    gpio_set_level(gsk, 0);
    // vTaskDelay(pdMS_TO_TICKS(1)); // Ensure the DAC1220 recognizes the low state again
    ets_delay_us(1000); // Third high period
    // Deactivate CS
    gpio_set_level(gcs, 1); // Deactivate DAC1220 CS
}

void DAC1220::initSPI() {
    spi_bus_config_t buscfg;
	memset(&buscfg, 0, sizeof (spi_bus_config_t));
    buscfg.mosi_io_num = mosiPin;
    buscfg.miso_io_num = misoPin;
    buscfg.sclk_io_num = sckPin;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;


    spi_device_interface_config_t spi_devcfg;
    memset(&spi_devcfg, 0, sizeof(spi_device_interface_config_t));
    spi_devcfg.mode = 1;
    spi_devcfg.clock_speed_hz = static_cast<int>(frequency);
    spi_devcfg.input_delay_ns = 20; //?
    spi_devcfg.spics_io_num = csPin;
    spi_devcfg.queue_size = 7;

    esp_err_t init_bus_result = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (init_bus_result != ESP_OK) {
        Serial.printf("spi bus init fail\n");
    }

    esp_err_t add_device_result = spi_bus_add_device(HSPI_HOST, &spi_devcfg, &spi);
    if (add_device_result != ESP_OK) {
        Serial.printf("spi bus add  device fail\n");
        // Handle error
    }
}

/**
 * Calibrates the DAC and switches automatically to Normal mode when complete.
 */
void DAC1220::calibrate(bool output_on) {
  // Set desired output state during calibration.
  if (output_on) {
    cmr |= (1 << CMR_CALPIN);
  } else {
    cmr &= ~(1 << CMR_CALPIN);
  }
  set_command_register(cmr);

  // Start calibration, in a separate call.
  //uint32_t cal_cmd = cmr | (CMR_MD_CAL << CMR_MD);
  uint32_t cal_cmd = (CMR_MD_CAL << CMR_MD);
  Serial.printf("calibrate start\n");
  set_command_register(cal_cmd);
  // Wait 500 ms for calibration to complete.
  delay(1000);
  Serial.printf("calibrate end\n");
}



void printBinary(unsigned int value, int bitsCount = 8) {
    for (int i = bitsCount - 1; i >= 0; --i) {
        Serial.print((value >> i) & 1);
        if (bitsCount == 3)
          Serial.print(".");
    }
}

void DAC1220::write_register(uint8_t cmd, uint32_t reg) {
    uint8_t buffer[4] = {cmd, 0, 0, 0}; // Ensure buffer is large enough for command + data
    spi_transaction_t t;

    memset(&t, 0, sizeof(t)); // Zero out the transaction structure

    // Set up the transaction based on the command byte's indication of register byte length
    switch (cmd & ((uint8_t)0b11 << CB_MB)) {
        case (CB_MB_1 << CB_MB): // 1 byte
            buffer[1] = (char)(reg & 0xFF);
            t.length = 16; // Command byte + 1 data byte = 16 bits
            break;
        case (CB_MB_2 << CB_MB): // 2 bytes
            buffer[1] = (char)((reg >> 8) & 0xFF);
            buffer[2] = (char)(reg & 0xFF);
            t.length = 24; // Command byte + 2 data bytes = 24 bits
            break;
        case (CB_MB_3 << CB_MB): // 3 bytes
            buffer[1] = (char)((reg >> 16) & 0xFF);
            buffer[2] = (char)((reg >> 8) & 0xFF);
            buffer[3] = (char)(reg & 0xFF);
            t.length = 32; // Command byte + 3 data bytes = 32 bits
            break;
    }
    for (auto ii = 0; ii < t.length / 8; ii++) {
      Serial.printf("%2x ", buffer[ii]);
      printBinary(buffer[ii]);
      Serial.printf(" ");
    }
    Serial.printf("\n");
    t.tx_buffer = buffer; // Set the transmit buffer
    spi_device_transmit(spi, &t); // Transmit the transaction
}




/**
 * Generates output voltages specified as left-justified 20-bit numbers
 * within a right-aligned 24-bit (xxFFFFFxh) integer value.
 */
void DAC1220::set_value(uint32_t value) {
  // Assumes 20-bit, straight-binary code.
  set_data_input_register(value);
}

/**
 * Generates output voltages specified as floating-point values between
 * 0 and 2*referenceVoltage volts.
 */
#if 1
void DAC1220::set_voltage(double value) {
  // Range check to prevent overflow.
  if (value < 0.0) {
    Serial.printf("too low %g\n", value);
  } else if (value >= 2*referenceVoltage) {
    Serial.printf("too high %g\n", value);
  }
  auto ratio = value / referenceVoltage;
  uint32_t count = ratio * 1048576; // 2^20
  Serial.printf("value %g reference %g ratio %g count %d\n", value, referenceVoltage, ratio, count);
  set_value(count << 4);
  // Assumes 20-bit, straight-binary code.
  //uint32_t code = (uint32_t) ((double)0x80000 * (value / referenceVoltage)); // Table 8 in datasheet.
//  uint32_t code = (uint32_t) ((double)0x80000 * (value / referenceVoltage)); // Table 8 in datasheet.
//  set_value(code);
}
#endif

#if 0
void DAC1220::set_voltage(double voltage) {
    uint32_t dacValue;

    if (bin_mode == TwosCompliment) {
        // Handling for two's complement mode
        double scaleFactor = static_cast<double>(maxDacValue) / (2 * referenceVoltage);
        dacValue = static_cast<uint32_t>((voltage + referenceVoltage) * scaleFactor);
    } else {
        // Handling for straight binary mode
        if (voltage < 0 || voltage > referenceVoltage) {
            Serial.printf("Voltage out of range\n");
            return;
        }
        double scaleFactor = static_cast<double>(maxDacValue) / referenceVoltage;
        dacValue = static_cast<uint32_t>(voltage * scaleFactor);
    }

    // Left-justify the DAC value for 20-bit to 24-bit alignment
    dacValue <<= 4;
    set_value(dacValue);
}
#endif
void DAC1220::set_command_register(uint32_t cmr) {
  // Register value set as a right-aligned 16-bit number (xxxxFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_2 << CB_MB) | (CMR_ADR << CB_ADR);
  write_register(cmd, cmr);
}


void DAC1220::set_data_input_register(uint32_t dir) {
  // Register value set as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_3 << CB_MB) | (DIR_ADR << CB_ADR);
  write_register(cmd, dir);
}

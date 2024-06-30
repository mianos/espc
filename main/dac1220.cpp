#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <cstring>

#include "dac1220.h"

#define TAG "DAC1220"

DAC1220::DAC1220(BinMode bin_mode, gpio_num_t sck, gpio_num_t miso, gpio_num_t mosi, gpio_num_t cs, uint32_t freq)
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

void DAC1220::reset_all() {
    // Ensure SPI is not in use
    spi_bus_free(SPI2_HOST);


    // Configure CS pin as output
	gpio_reset_pin(csPin);
    gpio_set_direction(csPin, GPIO_MODE_OUTPUT);
    gpio_set_level(csPin, 0); // Activate DAC1220 CS

	gpio_reset_pin(sckPin);
    gpio_set_direction(sckPin, GPIO_MODE_OUTPUT);

    // Reset sequence
    gpio_set_level(sckPin, 0);
    ets_delay_us(1000); // Third high period
    gpio_set_level(sckPin, 1);
    ets_delay_us(240); // First high period
    gpio_set_level(sckPin, 0);
    ets_delay_us(5); // Short low period
    gpio_set_level(sckPin, 1);
    ets_delay_us(480); // Second high period
    gpio_set_level(sckPin, 0);
    ets_delay_us(5); // Short low period
    gpio_set_level(sckPin, 1);
    ets_delay_us(960); // Third high period
    gpio_set_level(sckPin, 0);
    ets_delay_us(1000); // Third high period
    // Deactivate CS
    gpio_set_level(csPin, 1); // Deactivate DAC1220 CS
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

  ESP_LOGD(TAG,"Setting mode %d", bin_mode);
  set_bin_mode(bin_mode);

  // Calibrate with the output connected.
  calibrate(true);
}



void DAC1220::initSPI() {
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.mosi_io_num = mosiPin;
    buscfg.miso_io_num = misoPin;
    buscfg.sclk_io_num = sckPin;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    devcfg.clock_speed_hz = frequency;
    devcfg.mode = 1;
    devcfg.spics_io_num = csPin;
    devcfg.queue_size = 7;
    devcfg.input_delay_ns = 20; //?

    esp_err_t ret;
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    }

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
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
  uint32_t cal_cmd = (CMR_MD_CAL << CMR_MD);
  set_command_register(cal_cmd);
  // Wait 500 ms for calibration to complete. Plus a but for good measure
  vTaskDelay(pdMS_TO_TICKS(600));
}


void printBinary(unsigned int value, int bitsCount = 8) {
    char binaryStr[33]; // Maximum size needed for a 32-bit integer plus null terminator
    int index = 0;

    for (int i = bitsCount - 1; i >= 0; --i) {
        // Append '0' or '1' to binaryStr
        binaryStr[index++] = ((value >> i) & 1) ? '1' : '0';

        // For formatting specific bit groups, adjust as necessary
        if (bitsCount == 3 && i > 0) {
            binaryStr[index++] = '.';
        }
    }

    binaryStr[index] = '\0'; // Null-terminate the string

    // Log the binary string
    ESP_LOGD(TAG, "%s", binaryStr);
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
      ESP_LOGD(TAG,"%2x ", buffer[ii]);
      printBinary(buffer[ii]);
      ESP_LOGD(TAG," ");
    }
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
void DAC1220::set_voltage(double value) {
  // Range check to prevent overflow.
#if 0
  if (value < 0.0) {
    ESP_LOGE(TAG,"too low %g", value);
  } else if (value >= 2*referenceVoltage) {
    ESP_LOGE(TAG,"too high %g", value);
  }
#endif
  auto ratio = value / referenceVoltage;
  uint32_t count = ratio * 1048576; // 2^20
  ESP_LOGI(TAG,"value %g reference %g ratio %g count %lu", value, referenceVoltage, ratio, count);
  set_value(count << 4);
}


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

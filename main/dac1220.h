#pragma once

#include <Arduino.h>
#include "driver/spi_master.h"

// derived from: https://github.com/BoraE/C-SPI-Interface-to-DAC1220-DACs-on-Raspberry-Pi-Computers/blob/main/LICENSE

class DAC1220 {
public:
    enum BinMode {StraightBinary, TwosCompliment} bin_mode;

    DAC1220(BinMode bin_mode=StraightBinary, int sckPin = 25, int misoPin = 27, int mosiPin = 26, int csPin = 33, uint32_t frequency = 10000);
    void begin();
    void reset_all();

private:
    int sckPin, misoPin, mosiPin, csPin;  // Pin assignments
    uint32_t frequency;                   // SPI frequency
    spi_device_handle_t spi;              // SPI device handle
    // Voltage reference
    double referenceVoltage = 20.0; // 10V reference voltage but output to a bipolar op amp

    void write24(uint32_t data);
    void initSPI();

    uint32_t cmr = 0;
public:
  // Constants
  static constexpr uint32_t MAX_VALUE = 0xFFFFF;
  // Command Byte
  static constexpr uint8_t CB_RW = 7;
  static constexpr uint8_t CB_MB = 5;
  static constexpr uint8_t CB_ADR = 0;

  static constexpr uint8_t CB_RW_R = 1;
  static constexpr uint8_t CB_RW_W = 0;

  static constexpr uint8_t CB_MB_1 = 0b00;
  static constexpr uint8_t CB_MB_2 = 0b01;
  static constexpr uint8_t CB_MB_3 = 0b10;

  // Registers addresses
  static constexpr uint8_t DIR_ADR = 0;
  static constexpr uint8_t CMR_ADR = 4;
  static constexpr uint8_t OCR_ADR = 8;
  static constexpr uint8_t FCR_ADR = 12;

  // Command Register
  static constexpr uint32_t CMR_ADPT = 15;
  static constexpr uint32_t CMR_CALPIN = 14;
  static constexpr uint32_t CMR_CRST = 9;
  static constexpr uint32_t CMR_RES = 7;
  static constexpr uint32_t CMR_CLR = 6;
  static constexpr uint32_t CMR_DF = 5;
  static constexpr uint32_t CMR_DISF = 4;
  static constexpr uint32_t CMR_BD = 3;
  static constexpr uint32_t CMR_MSB = 2;
  static constexpr uint32_t CMR_MD = 0;

  static constexpr uint32_t CMR_MD_NORMAL = 0b00;
  static constexpr uint32_t CMR_MD_CAL = 0b01;
  static constexpr uint32_t CMR_MD_SLEEP = 0b10;

  static constexpr uint32_t maxDacValue = 0xFFFFF; // Max value for 20-bit DAC
  static constexpr uint32_t dacMidpoint = 0x80000; // Midpoint for two's complement

  // Modes
  enum class Mode {Sleep, Normal};
private:
  void startup();
  void reset();
  void set_mode(Mode mode);


  void set_command_register(uint32_t cmr);
  void set_data_input_register(uint32_t dir = 0);

  void write_register(uint8_t cmd, uint32_t reg);
public:
  void set_value(uint32_t value);
  void set_voltage(double voltage);
  void set_reference(double voltage) { referenceVoltage = voltage; };
  void set_bin_mode(BinMode bin_mode=TwosCompliment);
  void calibrate(bool output_on = 1);
    // Function to reset DAC
    void reset(bool resetAll = false) {
//        write24(resetAll ? DAC_RESET_ALL : DAC_RESET_INPUT);
    }

    // Function to set reference
    void setReference(bool enableGain2 = false) {
//        write24(enableGain2 ? DAC_REFERENCE_ENABLE_G2 : DAC_REFERENCE);
    }

    // Function to set gain
    void setGain(bool gainB1A1 = false) {
//        write24(gainB1A1 ? DAC_GAIN_B1A1 : DAC_GAIN_B2A2);
    }

    // Function to write and update DAC channel A
    void writeA(uint16_t value) {
      set_value(value);
//        write24(DAC_WRITE_A_UPDATE_A | (value & 0xFFFF));
    }

    // Function to update all channels
    void updateAll() {
//        write24(DAC_UPDATE_ALL);
    }
};


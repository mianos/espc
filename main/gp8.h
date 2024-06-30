#pragma once

#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

constexpr int I2C_MASTER_TIMEOUT_MS = 1000;

class GP8 {
public:
    enum eOutPutRange_t {
        eOutputRange2_5V = 0,
        eOutputRange5V = 1,
        eOutputRange10V = 2,
        eOutputRangeVCC = 3
    };

    GP8() = default;
    virtual ~GP8() = default;
    virtual esp_err_t setVoltage(float voltage, uint8_t channel = 0) = 0;

protected:
    eOutPutRange_t currentRange = eOutputRange5V;
};

class GP8I2C : public GP8 {
protected:
    uint16_t _resolution = 0;
    i2c_master_dev_handle_t dev_handle;

public:
    static constexpr uint16_t RESOLUTION_12_BIT = 0x0FFF;
    static constexpr uint16_t RESOLUTION_15_BIT = 0x7FFF;
    static constexpr uint8_t GP8XXX_CONFIG_CURRENT_REG = 0x02;
    static constexpr uint8_t DFGP8XXX_I2C_DEVICEADDR = 0x58;

    GP8I2C(uint16_t resolution = RESOLUTION_15_BIT, uint8_t deviceAddr = DFGP8XXX_I2C_DEVICEADDR,
           gpio_num_t sda_io_num = static_cast<gpio_num_t>(22), gpio_num_t scl_io_num = static_cast<gpio_num_t>(23));

    esp_err_t writeRegister(uint8_t reg, uint8_t* pBuf, size_t size);
    esp_err_t sendData(uint16_t data, uint8_t channel);
    void setDACOutRange(eOutPutRange_t range);
};

class GP8211S : public GP8I2C {
public:
    GP8211S(uint16_t resolution = RESOLUTION_15_BIT);
    esp_err_t setVoltage(float voltage, uint8_t channel = 0) override;
};


#include "GP8.h"

#define TAG "DFRobot_GP8XXX_IIC"

GP8I2C::GP8I2C(uint16_t resolution, uint8_t deviceAddr, gpio_num_t sda_io_num, gpio_num_t scl_io_num)
    : _resolution(resolution) {
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.scl_io_num = scl_io_num;
    i2c_mst_config.sda_io_num = sda_io_num;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = deviceAddr;
    dev_cfg.scl_speed_hz = 100000;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

esp_err_t GP8I2C::writeRegister(uint8_t reg, uint8_t* pBuf, size_t size) {
    constexpr size_t max_buffer_size = 10;
    if (size > max_buffer_size - 1) {
        return ESP_ERR_INVALID_SIZE;
    }
    uint8_t data[max_buffer_size];
    data[0] = reg;
    memcpy(&data[1], pBuf, size);
    return i2c_master_transmit(dev_handle, data, size + 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t GP8I2C::sendData(uint16_t data, uint8_t channel) {
    uint8_t buff[4] = {uint8_t(data & 0xFF), uint8_t(data >> 8)};
    uint8_t write_buf[5];
    uint8_t reg = GP8XXX_CONFIG_CURRENT_REG;

    if (channel == 1) {
        reg = GP8XXX_CONFIG_CURRENT_REG << 1;
    } else if (channel == 2) {
        reg = GP8XXX_CONFIG_CURRENT_REG;  // Adjust as necessary for other channels
    }

    write_buf[0] = reg;
    memcpy(&write_buf[1], buff, 2); // Copy data into write buffer starting after the register byte
    return i2c_master_transmit(dev_handle, write_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void GP8I2C::setDACOutRange(eOutPutRange_t range) {
    uint8_t data = 0x00;
    switch (range) {
        case eOutputRange5V:
            writeRegister(GP8XXX_CONFIG_CURRENT_REG >> 1, &data, 1);
            currentRange = range;
            ESP_LOGI(TAG, "Setting output range to 5V");
            break;
        case eOutputRange10V:
            data = 0x11;
            writeRegister(GP8XXX_CONFIG_CURRENT_REG >> 1, &data, 1);
            currentRange = range;
            ESP_LOGI(TAG, "Setting output range to 10V");
            break;
        default:
            break;
    }
}

GP8211S::GP8211S(uint16_t resolution) : GP8I2C(resolution) {}

esp_err_t GP8211S::setVoltage(float voltage, uint8_t channel) {
    ESP_RETURN_ON_FALSE(channel == 0, ESP_ERR_INVALID_ARG, TAG, "Invalid channel: %d", channel);
    uint16_t oval;
    switch (_resolution) {
    case RESOLUTION_15_BIT:
        switch (currentRange) {
        case eOutputRange5V:
            oval = static_cast<uint16_t>(voltage * static_cast<double>(0x7FFF) / 5.0);
            break;
        case eOutputRange10V:
            oval = static_cast<uint16_t>(voltage * static_cast<double>(0x7FFF) / 10.0);
            break;
        default:
            ESP_LOGE(TAG, "unsupported range");
            return ESP_ERR_INVALID_ARG;
        }
        sendData(oval, 0);
        ESP_LOGI(TAG, "output value %u", oval);
        break;
    default:
        ESP_LOGE(TAG, "unsupported resolution");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

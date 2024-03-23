#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"  // Use the appropriate PCNT driver header
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"


static const char *TAG = "counter";

#define PCNT_INPUT_SIG_IO   20 // Pulse Input GPIO
#define PCNT_INPUT_SIG_TRIGGER   GPIO_NUM_22 // Pulse Input GPIO

#define EXAMPLE_PCNT_HIGH_LIMIT 20000
#define EXAMPLE_PCNT_LOW_LIMIT  -1

static volatile int cnt = 0;
static volatile int trigger_cnt = 0;

QueueHandle_t queue;
pcnt_unit_handle_t pcnt_unit = NULL;

static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
	cnt++;
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

static void IRAM_ATTR ctrl_signal_isr_handler(void* arg)
{
	trigger_cnt++;
    pcnt_unit_stop(pcnt_unit);
}

void pcnt(void)
{
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PCNT_INPUT_SIG_IO,
        .level_gpio_num = PCNT_INPUT_SIG_TRIGGER
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, EXAMPLE_PCNT_HIGH_LIMIT));
    pcnt_event_callbacks_t cbs = {
        .on_reach = example_pcnt_on_reach,
    };
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));

    // Configure the control signal GPIO as input
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE, // Trigger interrupt on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PCNT_INPUT_SIG_TRIGGER),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Install the GPIO ISR service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    // Hook the ISR handler for the control signal GPIO
    ESP_ERROR_CHECK(gpio_isr_handler_add(PCNT_INPUT_SIG_TRIGGER, ctrl_signal_isr_handler, NULL));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}


void app_main() {
    queue = xQueueCreate(10, sizeof(int));

	extern void init_ledc_square_wave();
	extern void start_pulse_timer();

    init_ledc_square_wave();
    start_pulse_timer();
    pcnt();

    // Report counter value
    int pulse_count = 0;
    int event_count = 0;

    while (1) {
        if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(1000))) {
            ESP_LOGI(TAG, "Watch point event, count: %d", event_count);
        } else {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); 

        printf("cnt is %d trigger_cnt %d\n", cnt, trigger_cnt);
    }
}

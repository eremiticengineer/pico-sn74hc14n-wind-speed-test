#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"

#include "WindMonitor.hpp"

#define MAIN_TASK_PRIORITY ( tskIDLE_PRIORITY + 2UL )

const uint SN74HC14N_INTERRUPT_PIN = 15;
const bool CALLBACK_ENABLED = true;
int sn74hc14n_count = 0; // don't need volatile as only the task access it, not the ISR

TaskHandle_t sn74hc14n_task_handle = nullptr;

static WindMonitor wind;

void sn74hc14n_callback(uint gpio, __unused uint32_t events) {
  if (gpio == SN74HC14N_INTERRUPT_PIN) {
      wind.onPulse();
  }
}

void wind_scheduler_task(__unused void *)
{
    const uint32_t INTERVAL_MS = 1000;
    const TickType_t tickDelay = pdMS_TO_TICKS(INTERVAL_MS);

    uint32_t seconds = 0;

    while (true)
    {
        wind.sampleAverage1s();

        // Calculate gust over each 2-second period.
        if ((seconds + 1) % 2 == 0)
        {
            wind.sampleGustInterval(2000);
        }

        // Move to the next minute slot every 60 seconds.
        if ((seconds + 1) % 60 == 0)
        {
            wind.rotateMinute();
        }

        seconds++;

        vTaskDelay(tickDelay);
    }
}

void wind_info_task(__unused void *)
{
    while (true)
    {
        const int32_t average = wind.getRunningAverageMph10();

        const int32_t gust = wind.getHourlyMaxGustMph10();

        printf("Average wind: %ld.%ld mph\n", static_cast<long>(average / 10), static_cast<long>(average % 10));

        printf("Max gust: %ld.%ld mph\n", static_cast<long>(gust / 10), static_cast<long>(gust % 10));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    stdio_init_all();

    gpio_init(SN74HC14N_INTERRUPT_PIN);
    gpio_set_dir(SN74HC14N_INTERRUPT_PIN, GPIO_IN);
    gpio_pull_down(SN74HC14N_INTERRUPT_PIN);
    gpio_set_irq_enabled_with_callback(SN74HC14N_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE,
        CALLBACK_ENABLED, sn74hc14n_callback);

    xTaskCreate(wind_scheduler_task, "WindSchedulerTask", 1024, nullptr, MAIN_TASK_PRIORITY, nullptr);
    xTaskCreate(wind_info_task, "WindInfoTask", 1024, nullptr, MAIN_TASK_PRIORITY, nullptr);

    vTaskStartScheduler();

    return 0;
}

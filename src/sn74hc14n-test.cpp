#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"

#define MAIN_TASK_PRIORITY ( tskIDLE_PRIORITY + 2UL )

const uint SN74HC14N_INTERRUPT_PIN = 14;
const bool CALLBACK_ENABLED = true;
int sn74hc14n_count = 0; // don't need volatile as only the task access it, not the ISR

TaskHandle_t sn74hc14n_task_handle = nullptr;

void sn74hc14n_callback(uint gpio, __unused uint32_t events) {
  if (gpio == SN74HC14N_INTERRUPT_PIN) {
      BaseType_t higher_priority_task_woken = pdFALSE;
      vTaskNotifyGiveFromISR(sn74hc14n_task_handle, &higher_priority_task_woken);
      portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}

// vTaskNotifyGiveFromISR() increments the task's notification value each time the interrupt occurs.
// So if three interrupts happen before the task gets CPU time,
// FreeRTOS can remember that there were three notifications. But:
//   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
// returns the number of notifications that accumulated and then clears the count.
// So take advantage of that.
// Suppose SN74HC14N_INTERRUPT_PIN gets:
//   interrupt
//   interrupt
//   interrupt
// before the task runs, the notification value becomes 3. Then:
//   uint32_t pulses = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
// gives:
//   pulses == 3
// so:
//   sn74hc14nCount += pulses;
// doesn't lose those pulses.
void sn74hc14n_task(void *pvParameters) {
    while (true) {
        uint32_t pulses = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        sn74hc14n_count += pulses;

        printf("SN74HC14N : %d\n", sn74hc14n_count);
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

    xTaskCreate(sn74hc14n_task, "SN74HC14N", 1024, nullptr, MAIN_TASK_PRIORITY, &sn74hc14n_task_handle);

    vTaskStartScheduler();

    return 0;
}

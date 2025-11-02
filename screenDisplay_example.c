#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include "tkjhat/sdk.h"

// ...existing code...

static QueueHandle_t btn_queue;

void Screen() {
    init_display();
    write_text("Hello PICO!");
}

// IRQ callback: keep it fast, post gpio number to queue from ISR
void ButtonFxn(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (btn_queue) {
        uint32_t g = gpio;
        xQueueSendFromISR(btn_queue, &g, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Task that handles button events and performs display updates in thread context
static void display_task(void *arg) {
    (void)arg;
    uint32_t gpio;
    for (;;) {
        if (xQueueReceive(btn_queue, &gpio, portMAX_DELAY) == pdTRUE) {
            if (gpio == BUTTON1) {
                // do display work in task context (not in IRQ)
                clear_display();
                write_text("  2");
            } else if (gpio == BUTTON2) {
                // do display reinit in task context
                stop_display();
                init_display();
                write_text("  1");
            }
        }
    }
}

int main() {
    stdio_init_all();
    init_hat_sdk();

    // initialize display and show intro
    Screen();

    init_led();

    // initialize buttons using SDK helpers (they configure pin & pull)
    init_button1();
    init_button2();

    // create queue and task
    btn_queue = xQueueCreate(4, sizeof(uint32_t));
    xTaskCreate(display_task, "display", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);

    // attach IRQs for button *press* (buttons are active-high -> rising edge)
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, ButtonFxn);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, ButtonFxn);

    vTaskStartScheduler();

    // Should never reach here
    for (;;) tight_loop_contents();
}
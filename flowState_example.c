#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

enum modes {mode_0= 0, mode_1= 1, mode_2= 2};
    enum modes current_mode = mode_0;
enum states {WAITING = 3, DATA_READY = 4};
    enum states current_state = WAITING;

void imu_task(void *pvParameters) {
    while (1) {
        if (current_mode == mode_1) {
            if (current_state == WAITING) {
                printf("IMU Task Running in Mode 1\n");
                current_state = DATA_READY;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void choosing_symbol_task(void *pvParameters) {
    while (1) {
        if (current_mode == mode_1) {
            if (current_state == DATA_READY) {
                printf("Choosing Symbol Task Running in Mode 1\n");
                current_state = WAITING;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void movement_task(void *pvParameters) {
    while (1) {
        if (current_mode == mode_2) {
            if (current_state == WAITING) {
                printf("Movement Task Running in Mode 2\n");
                current_state = DATA_READY;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void choosing_symbol_task_2(void *pvParameters) {
    while (1) {
        if (current_mode == mode_2) {
            if (current_state == DATA_READY) {
                printf("Choosing Symbol Task Running in Mode 2\n");
                current_state = WAITING; 
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void debugging_task(void *pvParameters) {
    while (1) {
        printf("Current Mode: %d, Current State: %d\n", current_mode, current_state);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void buttonfxn(uint gpio, uint32_t events) {
    if (gpio == BUTTON1) {
        current_mode = mode_1;
        printf("Button 1 Pressed: Switched to Mode 1\n");
    } else if (gpio == BUTTON2) {
        current_mode = mode_2;
        printf("Button 2 Pressed: Switched to Mode 2\n");
    }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    init_hat_sdk();

    init_sw1();
    init_sw2();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &buttonfxn);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_FALL, true, &buttonfxn);
    // Create tasks
    TaskHandle_t debugging,choosetask1,choosetask2,imutask, movementtask = NULL;
    xTaskCreate(imu_task, "IMU Task", 2048, NULL, 2, &imutask);
    xTaskCreate(movement_task, "Movement Task", 2048, NULL, 2, &movementtask);
    xTaskCreate(choosing_symbol_task, "Choosing Symbol Task 1", 2048, NULL, 2, &choosetask1);
    xTaskCreate(choosing_symbol_task_2, "Choosing Symbol Task 2", 2048, NULL, 2, &choosetask2);
    xTaskCreate(debugging_task, "Debugging Task", 2048, NULL, 2, &debugging);

    // Start the scheduler
    vTaskStartScheduler();
}


#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

#define INPUT_BUFFER_SIZE 256

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

static void receive_task(void *arg){
    (void)arg;
    char line[INPUT_BUFFER_SIZE];
    size_t index = 0;
    
    while (1){
        //OPTION 1
        // Using getchar_timeout_us https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#group_pico_stdio_1ga5d24f1a711eba3e0084b6310f6478c1a
        // take one char per time and store it in line array, until reeceived the \n
        // The application should instead play a sound, or blink a LED. 
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT){// I have received a character
            if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
            if (c == '1'){
                current_mode = mode_1;
            }
            else if (c == '2'){
                current_mode = mode_2;
            }
            else if (c == '\n'){
                // terminate and process the collected line
                line[index] = '\0'; 
                printf("__[RX]:\"%s\"__\n", line); //Print as debug in the output
                index = 0;
                vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
            }
            else if(index < INPUT_BUFFER_SIZE - 1){
                line[index++] = (char)c;
            }
            else { //Overflow: print and restart the buffer with the new character. 
                line[INPUT_BUFFER_SIZE - 1] = '\0';
                printf("__[RX]:\"%s\"__\n", line);
                index = 0; 
                line[index++] = (char)c; 
            }
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }
        //OPTION 2. Use the whole buffer. 
        /*absolute_time_t next = delayed_by_us(get_absolute_time,500);//Wait 500 us
        int read = stdio_get_until(line,INPUT_BUFFER_SIZE,next);
        if (read == PICO_ERROR_TIMEOUT){
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }
        else {
            line[read] = '\0'; //Last character is 0
            printf("__[RX] \"%s\"\n__", line);
            vTaskDelay(pdMS_TO_TICKS(50));
        }*/
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
    TaskHandle_t receivetask,debugging,choosetask1,choosetask2,imutask, movementtask = NULL;
    xTaskCreate(imu_task, "IMU Task", 2048, NULL, 2, &imutask);
    xTaskCreate(movement_task, "Movement Task", 2048, NULL, 2, &movementtask);
    xTaskCreate(choosing_symbol_task, "Choosing Symbol Task 1", 2048, NULL, 2, &choosetask1);
    xTaskCreate(choosing_symbol_task_2, "Choosing Symbol Task 2", 2048, NULL, 2, &choosetask2);
    //xTaskCreate(debugging_task, "Debugging Task", 2048, NULL, 2, &debugging);
    xTaskCreate(receive_task, "Receive Task", 2048, NULL, 2, &receivetask);

    // Start the scheduler
    vTaskStartScheduler();
}



#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "tkjhat/ssd1306.h"

#define DEFAULT_STACK_SIZE 2048
#define BUFFER_SIZE 50

char buf[BUFFER_SIZE];
int spacecount = 0;
int i = 0;

void ButtonFxn(uint gpio, uint32_t eventMasks){
    if(gpio == BUTTON1 && eventMasks == GPIO_IRQ_EDGE_FALL){
        printf("Button 1 Pressed!\n");
        buf[i] = '.';
        i += 1;
        spacecount ++;
        if (spacecount == 3){
            for (int j=0; j<sizeof(buf); j++){
                printf("%c\n",buf[j]);
            }
            spacecount = 0;
        }
        vTaskDelay(pdTICKS_TO_MS(500));
    }
    if(gpio == BUTTON2 && eventMasks == GPIO_IRQ_EDGE_FALL){
        printf("Button 2 Pressed!\n");
    }
}

int main(){
    stdio_init_all();
    init_hat_sdk();
    init_button1();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &ButtonFxn);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_FALL, true);

    vTaskStartScheduler();
}
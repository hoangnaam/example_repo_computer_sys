#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

static QueueHandle_t btn_queue;
float ax, ay, az, gx, gy, gz, t;
char *symbol;
enum state {mode_0 = 0, mode_1 = 1, mode_2 = 2};
enum state current_mode = mode_0;

void imu_task(void *pvParameters);

void check_values_task(float in_ax, float in_ay, float in_az, float in_gx, float in_gy, float in_gz, float in_t) {
    if (fabs(in_ax) > 0.85 ) {
        symbol = ".";
    }
    else if (fabs(in_ay) > 0.85 ) {
        symbol = "-";
    }
    else if (fabs(in_az) > 0.85 ) {
        symbol = " ";
    }
}


void buttonfxn(uint gpio, uint32_t events) {
    // Just a placeholder for button press functionality
    float in_ax, in_ay, in_az, in_gx, in_gy, in_gz, in_t;
    in_ax = ax;
    in_ay = ay;
    in_az = az;
    in_gx = gx;
    in_gy = gy;
    in_gz = gz;
    in_t = t;
    
    check_values_task(in_ax, in_ay, in_az, in_gx, in_gy, in_gz, in_t);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (btn_queue) {
        uint32_t g = gpio;
        xQueueSendFromISR(btn_queue, &g, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    vTaskDelay(pdMS_TO_TICKS(300));
}

static void display_task(void *pvParameters) {
    (void)pvParameters;
    uint32_t gpio;
    init_display();
    while (1) {
        if (xQueueReceive(btn_queue, &gpio, portMAX_DELAY) == pdTRUE) {
            // Process button press event
            write_text(symbol);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void imu_task(void *pvParameters) {
    (void)pvParameters;
    // Setting up the sensor. 
        if (init_ICM42670() == 0) {
            printf("ICM-42670P initialized successfully!\n");
            if (ICM42670_start_with_default_values() != 0){
                printf("ICM-42670P could not initialize accelerometer or gyroscope");
            }
            /*int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
            printf ("Enable gyro: %d\n",_enablegyro);
            int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
            printf ("Gyro return:  %d\n", _gyro);
            int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
            printf ("Accel return:  %d\n", _accel);*/
        } else {
            printf("Failed to initialize ICM-42670P.\n");
        }
        // Start collection data here. Infinite loop. 
        while (1)
        {
            if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                printf("Ax: %.2f Ay: %.2f Az: %.2f Gx: %.2f Gy: %.2f Gz: %.2f T: %.2f\n", ax, ay, az, gx, gy, gz, t);
            } else {
                printf("Failed to read imu data\n");
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_led();
    init_sw1();
    init_sw2();
    printf("Start acceleration test\n");

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &buttonfxn);
    TaskHandle_t hIMUTask, hdisplay = NULL;

    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);
    xTaskCreate(display_task, "DisplayTask", 2048, NULL, 2, &hdisplay);
    // Start the FreeRTOS scheduler
    printf("Starting scheduler\n");
    vTaskStartScheduler();

    return 0;
}

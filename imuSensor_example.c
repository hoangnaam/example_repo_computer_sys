
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

float ax, ay, az, gx, gy, gz, t;
int symbol;
enum state {mode_0 = 0, mode_1 = 1, mode_2 = 2};
enum state current_mode = mode_0;

void imu_task(void *pvParameters);

void check_values_task(float in_ax, float in_ay, float in_az, float in_gx, float in_gy, float in_gz, float in_t) {
    if (fabs(in_ax) > 0.85 ) {
        symbol = 0x2E; // '.'
        

    }
    else if (fabs(in_ay) > 0.85 ) {
        symbol = 0x2D; // '-'
        
    }
    else if (fabs(in_az) > 0.85 ) {
        symbol = 0x20; // ' ' (space)
    }
}

void print_symbol_task(void *pvParameters) {
    (void)pvParameters;
    while (1) {
        if (symbol != 0) {
            printf("Symbol: %c\n", (char)symbol);
            symbol = 0; // Reset symbol after printing
        }
        vTaskDelay(pdMS_TO_TICKS(100));
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
    vTaskDelay(pdMS_TO_TICKS(100));
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
                continue;
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
    TaskHandle_t hprinting, hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);
    xTaskCreate(print_symbol_task, "PrintSymbolTask", 2048, NULL, 2, &hprinting);
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}

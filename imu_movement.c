
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

float ax_global, ay_global, az_global, gx_global, gy_global, gz_global, t_global;
int symbol;
float gyro_buf[3];

void imu_task(void *pvParameters);
void checking_max(void *arg);
void printing_task();

void imu_task(void *pvParameters) {
    (void)pvParameters;
    // Setting up the sensor. 
        if (init_ICM42670() == 0) {
            printf("ICM-42670P initialized successfully!\n");
            if (ICM42670_start_with_default_values() != 0){
                printf("ICM-42670P could not initialize accelerometer or gyroscope");
            }
            int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
            printf ("Enable gyro: %d\n",_enablegyro);
            int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
            printf ("Gyro return:  %d\n", _gyro);
            int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
            printf ("Accel return:  %d\n", _accel);
        } else {
            printf("Failed to initialize ICM-42670P.\n");
        }
        // Start collection data here. Infinite loop. 
        while (1)
        {

            if (ICM42670_read_sensor_data(&ax_global, &ay_global, &az_global, &gx_global, &gy_global, &gz_global, &t_global) == 0) {
                // printf("Acc: ax= %.3f ay=%.3f az=%.3f|Gyro [dps]: gx=%.3f gy=%.3f gz=%.3f \n", 
                //     ax_global, ay_global, az_global, gx_global, gy_global, gz_global, t_global);
                gyro_buf[0] = gx_global;
                gyro_buf[1] = gy_global;
                gyro_buf[2] = gz_global;
            } else {
                printf("Failed to read imu data\n");
            }
            vTaskDelay(pdMS_TO_TICKS(375));
        }
}

void checking_max(void *arg){
    while(1){
        float max_gx = fabs(gyro_buf[0]);
        float max_gy = fabs(gyro_buf[1]);
        float max_gz = fabs(gyro_buf[2]);
        float max_num = 0.0f;
        int axis = 0;
        for (int i = 0; i<3; i++){
            if (fabs(gyro_buf[i]) > max_num){
                max_num = fabs(gyro_buf[i]);
                axis = i;
            }
        }
        if (fabs(gyro_buf[axis]) >= 175){
            switch (axis){
                case 0:
                    symbol = 0x2D; // '.'
                    break;
                case 1:
                    symbol = 0x20; // ' '
                    break;
                case 2:
                    symbol = 0x2E; // '-'
                    break;
                default:
                    symbol = 0x3F;
                    break;
            }
            if (symbol != 0x3F){
                printing_task();
                sleep_ms(300);    
            }
        }
        max_num = 0;
        axis = 0;
        vTaskDelay(pdMS_TO_TICKS(375));
    }
}

void printing_task(){

    printf("Symbol: %c\n", symbol);

}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    printf("Start acceleration test\n");

    TaskHandle_t hchecktask, hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);
    xTaskCreate(checking_max, "CheckMaxTask", 2048, NULL, 2, &hchecktask);
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}
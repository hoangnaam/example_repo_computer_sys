
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

float ax_global, ay_global, az_global, gx_global, gy_global, gz_global, t_global;
int symbol;
float acc_buff[3];

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
            int _accel = ICM42670_startAccel(200, ICM42670_ACCEL_FSR_DEFAULT);
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
                float modulus = sqrt(ax_global*ax_global + ay_global*ay_global);
                acc_buff[0] = modulus;
                acc_buff[1] = az_global;
                acc_buff[2] = gy_global;                    
            } else {
                printf("Failed to read imu data\n");
            }
            vTaskDelay(pdMS_TO_TICKS(375));
        }
}

void checking_max(void *arg){
    while(1){
        float local_modulus = fabs(acc_buff[0]);
        float local_az = fabs(acc_buff[1]);
        float local_gy = fabs(acc_buff[2]);
        float max_num = 0.0f;
        int axis = 0;
        for (int i = 0; i<2; i++){
            if (fabs(acc_buff[i]) > max_num) {
                max_num = fabs(acc_buff[i]);
                axis = i;
            }
        }
        if (local_gy >= 170) {
            symbol = 0x20;
            printing_task();
        }
        else if(local_modulus > 2){
            symbol = 0x2E; // '.'
            printing_task();
        }
        else if(local_az > 1.3){
            symbol = 0x2D; // '-'
            printing_task();
        }
        max_num = 0;
        axis = 0;
        sensor_flag = true;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void printing_task(){

    printf("Symbol: %c\n", symbol);
    printf("Wait 2 seconds for next detection...\n");
    symbol = 0;
    sleep_ms(2500);
    printf("Make the gesture now!\n");

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

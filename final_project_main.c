
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <math.h>

float ax_global, ay_global, az_global, gx_global, gy_global, gz_global, t_global;
char * symbol;
float acc_buff[3];
bool button_pressed = false;
enum modes { mode_0 = 0, mode_1};
enum modes current_mode = mode_0;

#define INPUT_BUFFER_SIZE 256

void imu_task(void *pvParameters);
void checking_max(void *arg);
void printing_task();

void ButtonFxn(uint gpio, uint32_t events) {
    button_pressed = true;
}

void check_acceleration(void *arg) {
    while(1) {
        if (current_mode == mode_1) {
            if (button_pressed) {
                float absX = fabs(ax_global);
                float absY = fabs(ay_global);
                float absZ = fabs(az_global);
                
                if (absX > 0.8) {
                    printf("Symbols:%c\n", 0x20);
                } 
                else if (absY > 0.8) {
                    printf("Symbols:%c\n", 0x2D);
                } 
                else if (absZ > 0.8) {
                    printf("Symbols:%c\n", 0x2E);
                }
                button_pressed = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void receive_task(void *arg){
    (void)arg;
    char line[INPUT_BUFFER_SIZE];
    size_t index = 0;
    
    while (1){
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT){// I have received a character
            if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
            if (c == '1'){
                current_mode = mode_0;
            }
            else if (c == '2'){
                current_mode = mode_1;
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
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
void imu_task(void *pvParameters) {
    (void)pvParameters;
        // Start collection data here. Infinite loop. 
        while (1)
        {
            if (current_mode == mode_0 || current_mode == mode_1) {
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
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
}

void checking_max(void *arg){
    while(1){
        if (current_mode == mode_0) {
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
            if (local_gy >= 170 && local_modulus < 1.2){
                symbol = " "; // ' ' (space)
                printing_task();
            }
            else if(local_modulus > 1.3 && local_az < 1){
                symbol = "."; // '.'
                printing_task();
            }
            else if(local_az > 1.3 && local_modulus < 0.8){
                symbol = "-"; // '-'
                printing_task();
            }
            max_num = 0;
            axis = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void Screen() {
    if (current_mode == mode_0) {
        clear_display();
        if (symbol == " ") {
            write_text("Space");
        }
        else {
            write_text(symbol);
        }
    }
}

void printing_task(){
    if (current_mode == mode_0) {
        printf("Symbol: %s\n", symbol);
        printf("Wait 2 seconds for next detection...\n");
        //Screen();
        sleep_ms(2500);
        printf("Make the gesture now!\n");
    }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/
    init_hat_sdk();
    init_display();
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
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    printf("Start acceleration test\n");

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &ButtonFxn);
    TaskHandle_t hreceive, hbutton, hchecktask, hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);
    xTaskCreate(checking_max, "CheckMaxTask", 2048, NULL, 2, &hchecktask);
    xTaskCreate(check_acceleration, "ButtonTask", 2048, NULL, 2, &hbutton);
    xTaskCreate(receive_task, "ReceiveTask", 2048, NULL, 2, &hreceive);
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}

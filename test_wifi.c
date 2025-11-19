#include <stdio.h>
#include "pico/stdlib.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

int main() {
    stdio_init_all();
    sleep_ms(5000); // Wait for USB serial to initialize

    // Initialize the CYW43 Wi-Fi chip (non-threaded version)
    if (cyw43_arch_init()) {
        printf("Failed to initialize WiFi chip!\n");
        return -1;
    }

    printf("Connecting to WiFi...\n");

    // Connect to Wi-Fi (blocking)
    if (cyw43_arch_wifi_connect_timeout_ms("Zaf", "$oulTerminator",
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connection failed!\n");
    } else {
        printf("Pico W is connected to WiFi!\n");
    }

    // Keep running
    while (true) {
        sleep_ms(1000);
        printf("Pico W is running...\n");
    }

    // Cleanup (never reached here)
    cyw43_arch_deinit();
    return 0;
}


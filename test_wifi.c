stdio_init_all();
    printf("Starting WiFi connection example\n");
    while (1)
    {
        /* code */
    if(cyw43_arch_init()) {
        printf("Wireless init failed\n");
    }
    else {
        // Enabling "Station"-mode, where we can connect to wireless networks
        cyw43_arch_enable_sta_mode();
        // Connecting to the open panoulu-network (no password needed)
        // We try to connect for 30 seconds before printing an error message
        if(cyw43_arch_wifi_connect_timeout_ms("Zaf", "$oulTerminator", CYW43_AUTH_OPEN, 30*1000)) {
            printf("Failed to connect\n");
        }
        else {
            printf("Connected to panoulu\n");
        }
    }
    }

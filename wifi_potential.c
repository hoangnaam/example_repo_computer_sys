#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "semphr.h"

// --- lwIP headers ---
#include "lwip/opt.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

// --- Critical fix: undefine lwIP's macro before including Pico SDK ---
#undef poll

#include "pico/cyw43_arch.h"

#define TCP_TRANSPORT_WAIT_MS 10000
#define INPUT_BUFFER_SIZE     512

typedef struct {
    ip_addr_t        host;     // Resolved IP
    uint16_t         port;     // Port number
    int              sock;     // Socket file descriptor
    SemaphoreHandle_t dns_sem; // DNS completion semaphore
} TCPTransport;

/* --------------------- DNS CALLBACK (internal) --------------------- */

static void dns_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    TCPTransport *t = (TCPTransport *)callback_arg;

    if (ipaddr != NULL) {
        t->host = *ipaddr;
    }
    /* Just signal that DNS is done (success or failure). */
    xSemaphoreGive(t->dns_sem);
}

/* --------------------- 1. DNS LOOKUP --------------------- */

void tcp_dns_lookup(TCPTransport *t, const char *hostname)
{
    err_t res;

    /* Assume t->dns_sem is already created */
    /* Clear any previous give */
    xSemaphoreTake(t->dns_sem, 0);

    res = dns_gethostbyname(hostname, &t->host, dns_cb, t); // don't understand the dns_cb and t

    if (res == ERR_OK) {
        /* Address was resolved immediately, t->host is valid now */
        return;
    }

    if (res == ERR_INPROGRESS) {
        /* Wait for the callback to signal completion */
        xSemaphoreTake(t->dns_sem, pdMS_TO_TICKS(TCP_TRANSPORT_WAIT_MS)); // don't understand what is xSemaphoreTake
        return;
    }

    /* Any other res means error; no extra handling as requested */
    return;
}

/* --------------------- 2. OPEN SOCKET --------------------- */

void tcp_open_socket(TCPTransport *t)
{
    struct sockaddr_in serv_addr; //Is this struct undefined?

    t->sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr)); // why 0?
    serv_addr.sin_family = AF_INET;             //what this line and the next line do?
    serv_addr.sin_port   = htons(t->port);

    /* Same trick as in the C++ code you showed */
    memcpy(&serv_addr.sin_addr.s_addr, &t->host, sizeof(t->host)); //what is copied and what is the first argument?

    connect(t->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); // is it we need to connect to the ip address? What is this function doing? (connect to where?)
}

/* --------------------- 3. HTTPS GET (send only) --------------------- */

void tcp_https_get(TCPTransport *t, const char *hostname, const char *path)
{
    char request[INPUT_BUFFER_SIZE];

    /* This builds a plain HTTP request.
       NOTE: Real HTTPS requires a TLS layer on top of this socket.
       Here we just send the HTTP GET bytes. */

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, hostname); //is it for now, the program sends nothig?

    write(t->sock, request, strlen(request)); // what is t->sock, I saw you used a lot before
}

/* --------------------- 4. SOCKET RECEIVE --------------------- */

void tcp_read_response(TCPTransport *t)
{
    char buffer[INPUT_BUFFER_SIZE];
    int n;

    printf("Reading response...\n");

    while ((n = read(t->sock, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);   // print to serial
    }

    printf("\n--- End of response ---\n");
}

/* --------------------- 5. SOCKET CLOSE --------------------- */

void tcp_transport_close(TCPTransport *t)
{
    close(t->sock);
}

/* --------------------- MAIN EXAMPLE --------------------- */

int connecting_wifi(){
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
}

int main(void)
{
    /* Pico / stdio init */
    stdio_init_all();  // You need to define somewhere if not using stdio

    connecting_wifi();

    TCPTransport transport;
    const char *hostname = "neverssl.com";
    const char *path     = "/"; //is it you can change "/" to anything else?

    transport.port    = 80;  /* HTTPS port */
    transport.dns_sem = xSemaphoreCreateBinary();  // Requires FreeRTOS running

    /* 1. DNS lookup */
    tcp_dns_lookup(&transport, hostname);

    /* 2. Open socket */
    tcp_open_socket(&transport);

    /* 3. HTTPS GET (send only) */
    tcp_https_get(&transport, hostname, path);

    /* 4. HTTPS READ – you said “forget this now”, so not implemented */
    tcp_read_response(&transport);
    /* 5. Close socket */
    tcp_transport_close(&transport);

    /* If this is inside a FreeRTOS task, you probably return or delete the task instead. */
    // vTaskDelete(NULL);       // You need to define somewhere in a task context

    while (1) {
        tight_loop_contents(); // Or vTaskDelay(...) if using FreeRTOS
    }

    return 0;
}

# Raspberry-Pi-Pico-LIN
LIN library for UART (to be used with transceiver)

For example [TLIN](https://www.ti.com/store/ti/en/p/product/?p=TLIN2029DRQ1&utm_source=google&utm_medium=cpc&utm_campaign=asc-null-null-OPN_EN-cpc-store-google-wwe&utm_content=Device&ds_k=TLIN2029DRQ1&DCM=yes&gclid=CjwKCAjwm7mEBhBsEiwA_of-TAKlG2J9qPjvqYHOgww9pG8lpJY5LOO7HmK3Xh_q1XpcVLn3b2mDDhoC980QAvD_BwE&gclsrc=aw.ds) 

# Master Code Example
```cpp
#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lin.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main()
{
    stdio_init_all();

    // Initialize LIN
    lin_initialize(uart0, 12, 1, 9600);

    // Send Buffer
    uint8_t data[] = {0xC, 0xD};

    // Receive Buffer
    uint8_t rx_data[] = {0x0, 0x0};

    // Frame to send as frame (with data)
    lin_frame* frame = new lin_frame();

    // Set params
    frame->id = 0x10;
    frame->enhanced_checksum = true;
    frame->data = data;
    frame->length = lin_get_data_size(frame->id);

    // Header to receive data
    lin_frame* header = new lin_frame();

    // Set params
    header->id = 0x5;
    header->enhanced_checksum = true;
    header->data = rx_data;
    header->length = lin_get_data_size(frame->id);

    while(true)
    {
        // Send header
        lin_send_header(header);
        sleep_ms(2000);
        printf("Data: %.02X %.02X\r\n", header->data[0], header->data[1]);
        printf("Checksum: %.02X\r\n", header->checksum);

        // Send frame
        lin_send_frame(frame);
        sleep_ms(2000);
    }

    return 0;
}


#pragma clang diagnostic pop
```

# Slave Code Example
```cpp
#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lin.h"


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
    stdio_init_all();

    // Initalize LIN
    lin_initialize(uart0, 0, 13, 9600);

    // Create Frame to RX
    lin_frame* frame = new lin_frame();

    // Send Bytes
    uint8_t data_send[] = {0x0C, 0x0D};

    // Receive buffer
    uint8_t data_rx[] = {0x0, 0x0, 0x0, 0x0};


    while(true){
        // Load receive buffer
        frame->data = data_rx;

        // Receive frame
        lin_read_frame_blocking(frame);

        // Grab ID and print it
        uint8_t sid = frame->id & 0b00111111;
        printf("ID: %02X\r\n", sid);

        // Validate  ID
        if(sid == 0x5) {
            // Respond to frame
            frame->data = data_send;
            frame->length = lin_get_data_size(sid);
            frame->enhanced_checksum = true;
            lin_send_response(frame);
        }
        else if(sid == 0x10)
        {
            // Print data from master
            printf("Data: %.02X %.02X\r\n", frame->data[0], frame->data[1]);
            printf("Checksum: %.02X\r\n", frame->checksum);
        }
    }

    return 0;
}

#pragma clang diagnostic pop
```


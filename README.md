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
    lin_initialize(uart0, 0, 1, 9600);

    // Data array storage, predefined with default values
    uint8_t data[] = {0xA, 0xB, 0xC, 0xD, 0xD, 0xC, 0xB, 0xA};

    // Create LIN Frame
    lin_frame* frame = new lin_frame();
    frame->id = 5; // ID = 0x5
    frame->length = lin_get_data_size(frame->id); // Data Length for ID acc. to LIN 2.0 - 2 bytes
    frame->data = data; // Set data array

    frame->enhanced_checksum = true; // Enable enchanced checksum


    while(true)
    {
        printf("Sending frame...\r\n");
        lin_send_header(frame); // Send frame header
        printf("Sent...\r\n");
        printf("Data: %02X %02X\r\n", frame->data[0], frame->data[1]); // Debug 2B of data
        printf("Checksum: %02X\r\n", frame->checksum); // Print checksum

        sleep_ms(2500);

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

    // Initialize LIN
    lin_initialize(uart0, 0, 1, 9600);

    // Create frame to download data
    lin_frame* frame = new lin_frame();
    
    // Data arrays to be sent as responses
    uint8_t data[] = {0xA, 0xB, 0xC, 0xD, 0xD, 0xC, 0xB, 0xA};
    uint8_t data_null[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    while(true){
        // Read frame (blocks thread)
        lin_read_frame_blocking(frame);
    
        // Convert PID back to ID
        uint8_t sid = frame->id & 0b00111111;
        
        // Print ID on console
        putchar(sid);
        
        // If ID == 0x5 then send frame with data, otherwise data_null
        if(sid == 0x5) {
            // Set data
            frame->data = data;
            
            // Set length
            frame->length = lin_get_data_size(sid);
            
            // Enable enchanced checksum (LIN 2.0)
            frame->enhanced_checksum = true;
            
            // Send response
            lin_send_response(frame);
        }
        else{
            // Set data            
            frame->data = data_null;
            
            // Set length
            frame->length = lin_get_data_size(sid);
            
            // Enable enchanced checksum
            frame->enhanced_checksum = true;
            
            // Send response
            lin_send_response(frame);
        }
    }

    return 0;
}

#pragma clang diagnostic pop
```

# Known bugs
1. Slave cannot read data from master... Will be implemented using timeout.
2. Sometimes UART gets bugged and drops some data from slave - thus data being read is bugged. (eg. checksum is as data and data as checksum).

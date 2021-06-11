//
// Created by nov11 on 02.05.2021.
//

#ifndef PICOUARTLIN_LIN_H
#define PICOUARTLIN_LIN_H

#include "pico/stdlib.h"
#include <hardware/uart.h>

struct lin_frame{
    uint8_t id;
    uint8_t* data;
    uint8_t length;
    bool enhanced_checksum;

    // Checksum only in received frames
    uint8_t checksum;
};

void lin_initialize(uart_inst_t* uartInst, uint tx, uint rx, uint baudRate);
void lin_send_frame(lin_frame* frame);
void lin_send_response(lin_frame* response);
bool lin_read_frame_blocking(lin_frame* frame);
void lin_send_header(lin_frame* frame);
uint8_t lin_get_data_size(uint8_t pid);
uint8_t lin_calculate_parity(uint8_t id);

#endif //PICOUARTLIN_LIN_H

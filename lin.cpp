//
// Created by nov11 on 02.05.2021.
//

#include <cstdio>
#include "lin.h"

#define LIN_BREAK_WIDTH 14
#define LIN_SYNC 0x55

uint baud;
uint txPin;
uint rxPin;

uint8_t lin_sync = LIN_SYNC;
uart_inst_t* uart;

uint64_t get_lin_time(){
    double micros_d = (double) 1000000 / baud;
    uint64_t micros = (uint64_t) micros_d;

    return micros;
}

void lin_initialize(uart_inst_t *uartInst, uint tx, uint rx, uint baudRate) {
    // Initialize GPIO
    gpio_init(tx);
    gpio_init(rx);
    gpio_set_dir(tx, true);
    gpio_set_dir(rx, false);
    gpio_pull_up(tx);
    gpio_pull_up(rx);
    gpio_put(tx, true);

    // As UART
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_function(rx, GPIO_FUNC_UART);

    // Set PIN values
    txPin = tx;
    rxPin = rx;

    // Store BAUD value
    baud = baudRate;

    // Init UART
    uart_init(uartInst, baudRate);
    uart_set_format(uartInst, 8, 1, UART_PARITY_NONE);

    uart = uartInst;
    // READY
}

uint8_t calculate_checksum(uint8_t id, uint8_t* data, uint8_t len, bool enhanced_checksum){
    uint16_t sum = 0;
    for(int q = 0; q < len; q++){
        sum += data[q];
        if(sum > 256)
            sum -= 255;
    }
    // IF LIN2, then also consider ID
    if(enhanced_checksum) {
        sum += id;
        if (sum > 256)
            sum -= 255;
    }

    // Invert sum
    uint8_t sum8 = (uint8_t) sum;
    return ~sum8;
}

bool get_bit(uint8_t input, uint8_t n){
    return (input >> n) & 1;
}

uint8_t set_bit(uint8_t input, uint8_t n, bool value){
    if(value)
        input |= 1 << n;
    else
        input &= ~(1 << n);

    return input;
}

uint8_t lin_get_data_size(uint8_t pid){
    // Reset parity bits
    uint8_t id = set_bit(pid, 7, 0);
    id = set_bit(id, 6, 0);

    if(id <= 0x1F) return 2;
    if(id <= 0x2F) return 4;
    if(id <= 0x3F) return 8;

    return 0;
}

uint8_t lin_calculate_parity(uint8_t id){
    bool p0 = get_bit(id, 0) xor get_bit(id, 1) xor get_bit(id, 2) xor get_bit(id, 4);
    bool p1 = not (get_bit(id, 1) xor get_bit(id, 3) xor get_bit(id, 4) xor get_bit(id, 5));

    id = set_bit(id, 6, p0);
    id = set_bit(id, 7, p1);

    return id;
}


void lin_send_frame(lin_frame* frame)
{
    // Calculate settings
    uint64_t lin_delay = get_lin_time();
    uint8_t id = lin_calculate_parity(frame->id);

    // Break
    gpio_set_function(0, GPIO_FUNC_SIO);
    gpio_put(0, false); // Send 0 for 14 TLIN
    sleep_us(LIN_BREAK_WIDTH * lin_delay);
    gpio_put(0, true);
    sleep_us(lin_delay);

    // Frame
    gpio_set_function(0, GPIO_FUNC_UART);
    uart_write_blocking(uart, &lin_sync,1);
    uart_write_blocking(uart, &id,1);
    uart_write_blocking(uart, frame->data, frame->length);
    uint8_t checksum = calculate_checksum(id, frame->data, frame->length, frame->enhanced_checksum);
    uart_write_blocking(uart, &checksum,1);
}

void lin_send_header(lin_frame* frame)
{
    uint64_t lin_delay = get_lin_time();
    uint8_t id = lin_calculate_parity(frame->id);

    // Break
    gpio_set_function(0, GPIO_FUNC_SIO);
    gpio_put(0, false); // Send 0 for 14 TLIN
    sleep_us(LIN_BREAK_WIDTH * lin_delay);
    gpio_put(0, true);
    sleep_us(lin_delay);

    // Frame
    gpio_set_function(0, GPIO_FUNC_UART);
    uart_write_blocking(uart, &lin_sync,1);
    uart_write_blocking(uart, &id,1);

    // Read response data
    uart_read_blocking(uart, frame->data, frame->length);

    // Read checksum
    uart_read_blocking(uart, &(frame->checksum), 1);
}

void lin_send_response(lin_frame* frame)
{
    // Calculate PID
    uint8_t id = lin_calculate_parity(frame->id);

    // Update settings
    gpio_set_function(0, GPIO_FUNC_UART);

    // Send response
    uart_write_blocking(uart, frame->data, frame->length);
    uint8_t checksum = calculate_checksum(id, frame->data, frame->length, frame->enhanced_checksum);
    uart_write_blocking(uart, &checksum,1);
}

void lin_read_frame_blocking(lin_frame *frame)
{
    // Get timing
    uint64_t lin_delay = get_lin_time(); // Timing
    gpio_set_function(rxPin, GPIO_FUNC_SIO);

    uint8_t counter = 0;

    // Check gpio pin
    while(true) {
        // Check RX pin for BREAK (13 units by default)
        if (!gpio_get(rxPin)) {
            counter++; // Add value to counter
            sleep_us(lin_delay); // Wait for next bit
            if (counter >= LIN_BREAK_WIDTH) { // Wait for counter
                break;
            }
        }
        else{
            counter = 0; // If went high then reset
        }
    }
    gpio_put(rxPin, true);

    // Switch to UART
    gpio_set_function(rxPin, GPIO_FUNC_UART);

    // Temporary variables
    uint8_t sync;
    uint8_t id;
    uint8_t delay;

    // Read sync byte
    uart_read_blocking(uart, &delay, 1);
    uart_read_blocking(uart, &sync, 1);
    // Read protected ID
    uart_read_blocking(uart, &id, 1);

    // Set frame params
    frame->id = id;

    // Header read complete
}



/**
 * @file crc.c
 * @author Federico Turco ()
 * @brief 
 * @version 0.1
 * @date 2022-02-15
 * 
 * @copyright Copyright (c) Turco Federico 2022
 * 
 */

#include <stdint.h>
#include <stdio.h>

#include "config.h"


ssize_t addCrc16(uint8_t *buffer, ssize_t len){

    uint16_t crc = 0xFFFF;

    for (int pos = 0; pos < len; pos++)
    {
        crc ^= buffer[pos];    // XOR

        for (int i = 8; i != 0; i--)
        {
            // Passo ogni byte del pacchetto
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {             
                crc >>= 1;
            }
        }
    }

    buffer[len]     = crc & 0xFF;        // LSB
    buffer[len + 1] = crc >> 8;          // MSB

    len += 2;
    return len;
}

int8_t checkCrc16(uint8_t *buffer, ssize_t len){

    uint16_t crc = 0xFFFF;

    for (int16_t pos = 0; pos < (len - 2); pos++)
    {
        crc ^= buffer[pos];    // XOR

        for (int16_t i = 8; i != 0; i--)
        {
            // Passo ogni byte del pacchetto
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {             
                crc >>= 1;
            }
        }
    }

    if(!(buffer[len - 2] == (crc & 0xFF) && buffer[len - 1] == (crc >> 8))){
        printMillis();
        printf("ERROR: Invalid CRC, read: [%2x,%2x], calc: [%2x,%2x]\n", buffer[len -2], buffer[len -1], crc & 0xFF, crc >> 8);
    };

    return !(buffer[len - 2] == (crc & 0xFF) && buffer[len - 1] == (crc >> 8));
}


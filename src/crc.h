
#ifndef CRC_H
#define CRC_H

ssize_t addCrc16(uint8_t *buffer, ssize_t len);
int8_t checkCrc16(uint8_t *buffer, ssize_t len);

#endif
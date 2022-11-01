/**
 * @file config.c
 * @author Federico Turco ()
 * @brief 
 * @version 1.0
 * @date 2022-02-15
 * 
 * @copyright Copyright (c) Turco Federico 2022
 * 
 */
 
#ifndef CONFIG_H
#define CONFIG_H

#include <termios.h>

// TCP
typedef struct{
    char address[20];
    int port;
    long timeout;
} tcp_head;

// RTU
typedef struct{
    char device[20];
    int baud;
    char configuration[4];
    long timeout;
    int tty_VTIME;
    int tty_VMIN;
} rtu_head;

// File di configurazione
typedef struct{
    uint8_t verbose;
    tcp_head tcp;
    rtu_head rtu;
} config;



void printMillis(void);
uint64_t millis(void);
void readConfig(config *config);
int configureSerial(config *config, struct termios *tty);
int configureSocket(config *config);


#endif
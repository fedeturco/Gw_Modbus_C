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

// Standard libs
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

// Socket
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Serial
#include <fcntl.h>
#include <termios.h>

#include "config.h"


void printMillis(void)
{
    struct timespec spec;
    long millis;

    clock_gettime(CLOCK_REALTIME, &spec);

    millis = spec.tv_sec;
    printf("[%12lu", millis);

    millis = spec.tv_nsec / 1.0e6;
    printf(".%3lu] ", millis);
}

uint64_t millis(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    uint64_t millis = (uint64_t)(spec.tv_sec) * 1.0e3 + (uint64_t)(spec.tv_nsec) / 1.0e6;
    return millis;
}

void readConfig(config *config)
{
    char line[256];
    int linenum=0;

    // Apro il file di configurazione
    FILE *file_ = fopen("/etc/gwModbus/gwModbus.ini", "r");
    
    // Default values
    config->rtu.tty_VTIME = 1;
    config->rtu.tty_VMIN = 0;

    printMillis();
    printf("Reading configuration file\n");

    while(fgets(line, 256, file_) != NULL)
    {
            char key[256], sep[10], value[256];

            linenum++;
            if(line[0] == '#' || line[0] == '[') 
                continue;

            if(sscanf(line, "%s %s %s", key, sep, value) == 3)
            {
                // Verbose
                if(strcmp(key, "verbose") == 0){

                    config->verbose = atoi(value);

                    if(config->verbose > 2)
                    printf("Found key verbose\n");
                }

                // TCP

                // Address
                if(strcmp(key, "tcp_address") == 0){

                    if(config->verbose > 2)
                    printf("Found key tcp_address\n");

                    memcpy(&config->tcp.address, &value, sizeof(config->tcp.address));
                }

                // Port
                if(strcmp(key, "tcp_port") == 0){

                    if(config->verbose > 2)
                    printf("Found key tcp_port\n");

                    config->tcp.port = atoi(value);
                }

                // Timeout
                if(strcmp(key, "tcp_timeout") == 0){

                    if(config->verbose > 2)
                    printf("Found key tcp_timeout\n");

                    config->tcp.timeout = atol(value);
                }

                // Device
                if(strcmp(key, "ser_device") == 0){

                    if(config->verbose > 2)
                    printf("Found key ser_device\n");

                    memcpy(&config->rtu.device, &value, sizeof(config->rtu.device));
                }

                // Baudrate
                if(strcmp(key, "ser_baud") == 0){
                    
                    if(config->verbose > 2)
                    printf("Found key ser_baud\n");

                    config->rtu.baud = atoi(value);
                }

                // Configuration
                if(strcmp(key, "ser_configuration") == 0){

                    if(config->verbose > 2)
                    printf("Found key ser_configuration\n");

                    memcpy(&config->rtu.configuration, &value, sizeof(config->rtu.configuration));
                }

                // Timeout
                if(strcmp(key, "ser_timeout") == 0){

                    if(config->verbose > 2)
                    printf("Found key ser_timeout\n");

                    config->rtu.timeout = atol(value);
                }

                // tty_VTIME
                if(strcmp(key, "tty_VTIME") == 0){

                    if(config->verbose > 2)
                    printf("Found key tty_VTIME\n");

                    config->rtu.tty_VTIME = atoi(value);
                }

                // tty_VMIN
                if(strcmp(key, "tty_VMIN") == 0){

                    if(config->verbose > 2)
                    printf("Found key tty_VMIN\n");

                    config->rtu.tty_VMIN = atoi(value);
                }

            if(config->verbose){
                printMillis();
                printf("Line %3d  -  Key: %-15s Value: %-15s\n", linenum, key, value);
            }
        }
        else if(sscanf(line, "%s %s %s", key, sep, value) == 1)
        {
            if(config->verbose){
                printMillis();
                printf("%s\n", value);
            }
        }
        else
        {
            continue;
        }
    }
    printf("\n");

    fclose(file_);
}

int configureSerial(config *config, struct termios *p_tty)
{
    if(config->verbose){
        printMillis();
        printf("Serial configuration:\n\n");
    }
    
    // Apro seriale
    int serialPort = open(config->rtu.device, O_RDWR);

    // Leggo configurazione esistente e eventuali errori
    if(tcgetattr(serialPort, p_tty) != 0) {

        printMillis();
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    // Parity
    if(config->rtu.configuration[1] == 'N' || config->rtu.configuration[1] == 'n')
    {
        p_tty->c_cflag &= ~PARENB;         // DISABLE PARITY

        if(config->verbose){
            printMillis();
            printf("Parity: No parity\n");
        }
    }
    else if(config->rtu.configuration[1] == 'E' || config->rtu.configuration[1] == 'e')
    {
        p_tty->c_cflag &= ~PARODD;         // DISABLE ODD PARYTY (EVEN)
        p_tty->c_cflag |= PARENB;          // ENABLE PARITY

        if(config->verbose){
            printMillis();
            printf("Parity: Even\n");
        }
    }
    else if(config->rtu.configuration[1] == 'O' || config->rtu.configuration[1] == 'o')
    {
        p_tty->c_cflag |= PARODD;          // ENABLE ODD PARYTY
        p_tty->c_cflag |= PARENB;          // ENABLE PARITY

        if(config->verbose){
            printMillis();
            printf("Parity: Odd\n");
        }
    }
    else
    {
        printMillis();
        printf("ERROR: Invalid parity selected\n");
        exit(EXIT_FAILURE);
    }

    p_tty->c_cflag &= ~CSTOPB;         // Clear stop field, only one stop bit used in communication (most common)
    p_tty->c_cflag &= ~CSIZE;          // Clear all bits that set the data size 

    // Bytesize
    if(config->rtu.configuration[0] == '8')
    {
        p_tty->c_cflag |= CS8;
        
        if(config->verbose){
            printMillis();
            printf("Bytesize: 8\n");
        }
    }
    else if(config->rtu.configuration[0] == '7')
    {
        p_tty->c_cflag |= CS7;

        if(config->verbose){
            printMillis();
            printf("Bytesize: 7\n");
        }
    }
    else
    {
        printMillis();
        printf("ERROR: Invalid bytesize\n");
        exit(EXIT_FAILURE);
    }

    // Definitions prese da https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
    p_tty->c_cflag &= ~CRTSCTS;        // Disable RTS/CTS hardware flow control (most common)
    p_tty->c_cflag |= CREAD | CLOCAL;  // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    p_tty->c_lflag &= ~ICANON;
    p_tty->c_lflag &= ~ECHO;       // Disable echo
    p_tty->c_lflag &= ~ECHOE;      // Disable erasure
    p_tty->c_lflag &= ~ECHONL;     // Disable new-line echo
    p_tty->c_lflag &= ~ISIG;       // Disable interpretation of INTR, QUIT and SUSP
    p_tty->c_iflag &= ~(IXON | IXOFF | IXANY);                             // Turn off s/w flow ctrl
    p_tty->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);    // Disable any special handling of received bytes

    p_tty->c_oflag &= ~OPOST;      // Prevent special interpretation of output bytes (e.g. newline chars)
    p_tty->c_oflag &= ~ONLCR;      // Prevent conversion of newline to carriage return/line feed

    // VMIN = 0 and VTIME > 0 -> Pure time read, triggers after VTIME elapsed
    // VMIN > 0 and VTIME = 0 -> Pure counted read, triggers only when characters size (VMIN) is reached
    // VMIN > 0 and VTIME > 0 -> Trigger only if len > 0 with or timeout of VTIME elapsed

    p_tty->c_cc[VTIME] = config->rtu.tty_VTIME;
    p_tty->c_cc[VMIN] = config->rtu.tty_VMIN;
    
    if(config->verbose){
        printMillis();
        printf("Timeout: %li\n", config->rtu.timeout);
    }

    // Set baudrate
    switch(config->rtu.baud)
    {
        case 4800:
            cfsetispeed(p_tty, B4800);
            cfsetospeed(p_tty, B4800);

            if(config->verbose){
                printMillis();
                printf("Baudrate: 4800\n");
            }
            break;

        case 9600:
            cfsetispeed(p_tty, B9600);
            cfsetospeed(p_tty, B9600);
            
            if(config->verbose){
                printMillis();
                printf("Baudrate: 9600\n");
            }
            break;

        case 19200:
            cfsetispeed(p_tty, B19200);
            cfsetospeed(p_tty, B19200);

            if(config->verbose){
                printMillis();
                printf("Baudrate: 19200\n");
            }
            break;

        case 38400:
            cfsetispeed(p_tty, B38400);
            cfsetospeed(p_tty, B38400);
            
            if(config->verbose){
                printMillis();
                printf("Baudrate: 38400\n");
            }
            break;

        case 57600:
            cfsetispeed(p_tty, B57600);
            cfsetospeed(p_tty, B57600);
            
            if(config->verbose){
                printMillis();
                printf("Baudrate: 57600\n");
            }
            break;

        case 115200:
            cfsetispeed(p_tty, B115200);
            cfsetospeed(p_tty, B115200);
            
            if(config->verbose){
                printMillis();
                printf("Baudrate: 115200\n");
            }
            break;

        default:
            printMillis();
            printf("ERROR: Invalid baudrate\n");
            exit(EXIT_FAILURE);
    }

    if(config->verbose){
        printf("\n");
        printMillis();
        printf("VTIME: %i\n", config->rtu.tty_VTIME);
        printMillis();
        printf("VMIN:  %i\n", config->rtu.tty_VMIN);
    }
    
    // Applico tty settings
    if (tcsetattr(serialPort, TCSANOW, p_tty) != 0) {
        printMillis();
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));

        exit(EXIT_FAILURE);
    }

    if(config->verbose)
        printf("\n");

    return serialPort;
}

int configureSocket(config *config)
{
    int server_sockfd;
    int enable = 1;
    struct sockaddr_in server_address;

    struct protoent *protoent;
    protoent = getprotobyname("tcp");

    if (protoent == NULL) {
        exit(EXIT_FAILURE);
    }

    server_sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);

    // Check che la socket sia valida
    if (server_sockfd == -1) {
        perror("Socket error: ");
        exit(EXIT_FAILURE);
    }

    // Set sock option - REUSEADDR
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Set sock option - SO_RCVTIMEO
    struct timeval tv;
    tv.tv_sec = config->tcp.timeout / 1000;
    tv.tv_usec = (config->tcp.timeout - (config->tcp.timeout * 1000)) * 1000;

    if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
        perror("setsockopt(SO_RCVTIMEO) failed");
        exit(EXIT_FAILURE);
    }

    // Definizione server
    server_address.sin_family = AF_INET;
    inet_aton(config->tcp.address, &server_address.sin_addr);
    server_address.sin_port = htons(config->tcp.port);

    if (bind(server_sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) 
    {
        perror("Bind error: ");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sockfd, 5) == -1) {
        perror("Listen error: ");
        exit(EXIT_FAILURE);
    }

    return server_sockfd;
}

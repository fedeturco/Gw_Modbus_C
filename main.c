
// Copyright (c) 2022 Federico Turco

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// Standard libs
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <time.h>

// Socket
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Serial
#include <fcntl.h>      // Contains file controls like O_RDWR
#include <errno.h>      // Error integer and strerror() function
#include <termios.h>    // Contains POSIX terminal control definitions
#include <unistd.h>     // write(), read(), close()

// Def. buffer size
#define BUFSIZE_MODBUS  256
#define BUFSIZE_TCP     256

#define version         "1.0"

// TCP
typedef struct{
    char address[20];
    int port;
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

// Global vars
config settings;
struct termios tty;

void printMillis(){
    struct timespec spec;
    long millis;

    clock_gettime(CLOCK_REALTIME, &spec);

    millis = spec.tv_sec;
    printf("[%12lu", millis);

    millis = spec.tv_nsec / 1.0e6;
    printf(".%3lu] ", millis);
}

long millis(){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long millis = spec.tv_sec * 1.0e3 + spec.tv_nsec / 1.0e6;

    // debug
    if(settings.verbose > 2){
        printf("millis: %li\n", millis);
    }

    return millis;
}

void readConfig(config *config)
{
    char line[256];
    int linenum=0;

    FILE *file_ = fopen("/etc/gwModbus/gwModbus.ini", "r");
    
    // Default values
    config->rtu.tty_VTIME = 10;
    config->rtu.tty_VMIN = 5;

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

                    if(config->verbose > 2)
                    printf("Found key verbose\n");

                    config->verbose = atoi(value);
                }

                // Indirizzo
                if(strcmp(key, "address") == 0){

                    if(config->verbose > 2)
                    printf("Found key address\n");

                    memcpy(&config->tcp.address, &value, sizeof(config->tcp.address));
                }

                // Porta
                if(strcmp(key, "port") == 0){

                    if(config->verbose > 2)
                    printf("Found key port\n");

                    config->tcp.port = atoi(value);
                }

                // Device
                if(strcmp(key, "device") == 0){

                    if(config->verbose > 2)
                    printf("Found key device\n");

                    memcpy(&config->rtu.device, &value, sizeof(config->rtu.device));
                }

                // Baudrate
                if(strcmp(key, "baud") == 0){
                    
                    if(config->verbose > 2)
                    printf("Found key baud\n");

                    config->rtu.baud = atoi(value);
                }

                // Configuration
                if(strcmp(key, "configuration") == 0){

                    if(config->verbose > 2)
                    printf("Found key configuration\n");

                    memcpy(&config->rtu.configuration, &value, sizeof(config->rtu.configuration));
                }

                // Timeout
                if(strcmp(key, "timeout") == 0){

                    if(config->verbose > 2)
                    printf("Found key timeout\n");

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

            printMillis();
            printf("Line %3d  -  Key: %-15s Value: %-15s", linenum, key, value);
        }
        else if(sscanf(line, "%s %s %s", key, sep, value) == 1)
        {
            printMillis();
            printf("%s\n", value);
        }
        else
        {
            if(config->verbose > 2){
                printMillis();
                printf("ERROR: Syntax error, line %2d", linenum);
            }
            continue;
        }

        printf("\n");
    }
    printf("\n");
}

int configureSerial(config *config){

    printMillis();
    printf("Serial configuration:\n\n");
    
    // Apro seriale
    int serialPort = open(config->rtu.device, O_RDWR);

    // Leggo configurazione esistente e eventuali errori
    if(tcgetattr(serialPort, &tty) != 0) {

        printMillis();
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    // Parity
    if(config->rtu.configuration[1] == 'N' || config->rtu.configuration[1] == 'n')
    {
        tty.c_cflag &= ~PARENB;         // DISABLE PARITY
        printMillis();
        printf("Parity: No parity\n");
    }
    else if(config->rtu.configuration[1] == 'E' || config->rtu.configuration[1] == 'e')
    {
        tty.c_cflag &= ~PARODD;         // DISABLE ODD PARYTY (EVEN)
        tty.c_cflag |= PARENB;          // ENABLE PARITY
        printMillis();
        printf("Parity: Even\n");
    }
    else if(config->rtu.configuration[1] == 'O' || config->rtu.configuration[1] == 'o')
    {
        tty.c_cflag |= PARODD;          // ENABLE ODD PARYTY
        tty.c_cflag |= PARENB;          // ENABLE PARITY
        printMillis();
        printf("Parity: Odd\n");
    }
    else
    {
        printMillis();
        printf("ERROR: Invalid parity selected\n");
        exit(EXIT_FAILURE);
    }

    tty.c_cflag &= ~CSTOPB;         // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE;          // Clear all bits that set the data size 

    // Bytesize
    if(config->rtu.configuration[0] == '8')
    {
        tty.c_cflag |= CS8;
        
        printMillis();
        printf("Bytesize: 8\n");
    }
    else if(config->rtu.configuration[0] == '7')
    {
        tty.c_cflag |= CS7;

        printMillis();
        printf("Bytesize: 7\n");
    }
    else
    {
        printMillis();
        printf("ERROR: Invalid bytesize\n");
        exit(EXIT_FAILURE);
    }

    // Definitions prese da https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
    tty.c_cflag &= ~CRTSCTS;        // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL;  // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;       // Disable echo
    tty.c_lflag &= ~ECHOE;      // Disable erasure
    tty.c_lflag &= ~ECHONL;     // Disable new-line echo
    tty.c_lflag &= ~ISIG;       // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                             // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);    // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST;      // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;      // Prevent conversion of newline to carriage return/line feed

    // VMIN = 0 and VTIME > 0 -> Pure time read, triggers after VTIME elapsed
    // VMIN > 0 and VTIME = 0 -> Pure counted read, triggers only when characters size (VMIN) is reached
    // VMIN > 0 and VTIME > 0 -> Trigger only if len > 0 with or timeout of VTIME elapsed

    tty.c_cc[VTIME] = config->rtu.tty_VTIME;
    tty.c_cc[VMIN] = config->rtu.tty_VMIN;
    
    printMillis();
    printf("Timeout: %li\n", config->rtu.timeout);

    // Set baudrate
    switch(config->rtu.baud)
    {
        case 4800:
            cfsetispeed(&tty, B4800);
            cfsetospeed(&tty, B4800);

            printMillis();
            printf("Baudrate: 4800\n");
            break;

        case 9600:
            cfsetispeed(&tty, B9600);
            cfsetospeed(&tty, B9600);
            
            printMillis();
            printf("Baudrate: 9600\n");
            break;

        case 19200:
            cfsetispeed(&tty, B19200);
            cfsetospeed(&tty, B19200);

            printMillis();
            printf("Baudrate: 19200\n");
            break;

        case 38400:
            cfsetispeed(&tty, B38400);
            cfsetospeed(&tty, B38400);
            
            printMillis();
            printf("Baudrate: 38400\n");
            break;

        case 57600:
            cfsetispeed(&tty, B57600);
            cfsetospeed(&tty, B57600);
            
            printMillis();
            printf("Baudrate: 57600\n");
            break;

        case 115200:
            cfsetispeed(&tty, B115200);
            cfsetospeed(&tty, B115200);
            
            printMillis();
            printf("Baudrate: 115200\n");
            break;

        default:
            printMillis();
            printf("ERROR: Invalid baudrate\n");
            exit(EXIT_FAILURE);
    }

    printf("\n");

    printMillis();
    printf("VTIME: %i\n", config->rtu.tty_VTIME);

    printMillis();
    printf("VMIN:  %i\n", config->rtu.tty_VMIN);
    
    // Applico tty settings
    if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
        printMillis();
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));

        exit(EXIT_FAILURE);
    }

    printf("\n");

    return serialPort;
}

int configureSocket(config *config){

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

    // Set sock option
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Definizioe server
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

    for (int pos = 0; pos < (len - 2); pos++)
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

    if(!(buffer[len - 2] == (crc & 0xFF) && buffer[len - 1] == (crc >> 8))){
        printMillis();
        printf("ERROR: Invalid CRC, read: [%2x,%2x], calc: [%2x,%2x]\n", buffer[len -2], buffer[len -1], crc & 0xFF, crc >> 8);
    };

    return !(buffer[len - 2] == (crc & 0xFF) && buffer[len - 1] == (crc >> 8));
}




int main(){
    printf("\n");
    printf("------------------------------------\n");
    printf("------ GW Modbus TCP <-> RTU  ------\n");
    printf("------------------------------------\n");
    printf("\n");
    printf("Version: %s\n", version);
    printf("\n");

    // Leggo la configurazione dal file .ini
    readConfig(&settings);

    // Configuro la seriale
    int serialPort = configureSerial(&settings);

    // Configuro socket
    int socket = configureSocket(&settings);

    // Info
    printMillis();
    printf("Starting server at %s:%i\n\n", settings.tcp.address, settings.tcp.port);

    while (1) {
        // Definizioni client
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        // Connessione in ingresso
        int client_sockfd = accept(socket, (struct sockaddr*)&client_address,&client_len);

        // Leggo TCP in ingresso
        uint8_t request[BUFSIZE_MODBUS];
        ssize_t nBytes_read = read(client_sockfd, request, BUFSIZE_TCP);

        // Scarto pacchetti troppo corti
        if(nBytes_read < 8){
            continue;
        }

        struct in_addr ipAddr = client_address.sin_addr;

        char clientPort[6]; 
        char clientName[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, clientName, INET_ADDRSTRLEN);

        // Info connessione in ingresso
        printMillis();
        printf("Accepted connection from %s:%i\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Output console
        if(settings.verbose > 1){
            printMillis();
            printf("<- RX TCP [%3zu]: ", nBytes_read);

            for(int i = 0; i < nBytes_read; i++){
                printf("%02x ", request[i]);
            }

            printf("\n");
        }

        // Controllo protocol identifier che sia 00 00
        if(request[2] != 0 || request[3] != 0){
            printf("Protocol identifier non valido [%2x %2x], per ModBus TCP deve essere 0\n", request[2], request[3]);
            close(client_sockfd);
            continue;
        }

        ssize_t messageLen = (request[4] << 8) + request[5];
        ssize_t responseLen = 0;

        if((messageLen + 6) != nBytes_read){
            printMillis();
            printf("ERROR: incoming buffer length [%3zu] != MosBus packet length + 6 [%3zu]\n", nBytes_read, messageLen);
        }

        // FC01, FC02
        if(request[7] == 0x01 || request[7] == 0x02){
            responseLen = ((((request[10]) << 8) + (request[11])) / 8) + ((((request[10]) << 8) + (request[11])) % 8 != 0) + 5; // 1 slave id, 1 FC, 1 len, 2 byte CRC
        }

        // FC03, FC04
        else if(request[7] == 0x03 || request[7] == 0x04){
            responseLen = ((((request[10]) << 8) + (request[11])) * 2) + 5; // 1 slave id, 1 FC, 1 len, 2 byte CRC
        }

        // FC05, FC06 
        else if(request[7] == 0x05 || request[7] == 0x06){
            responseLen = 8;     // Risposta a lunghezza fissa di 8 bytes
        }

        // FC15, FC16
        else if(request[7] == 0x0F || request[7] == 0x10){
            responseLen = 8;     // Risposta a lunghezza fissa di 8 bytes
        }

        // FC08
        else if(request[7] == 0x08){
            responseLen = 6;     // Risposta a lunghezza fissa di 6 bytes
        }

        // Unknown Function Code
        else{
            printMillis();
            printf("Unknow function code FC %2i", request[7]);
            close(client_sockfd);
            continue;
        }

        // Creo un nuovo buffer con il pacchetto da inviare sulla 485
        uint8_t toSend485[BUFSIZE_MODBUS];
        ssize_t nBytes_write = nBytes_read - 6;
        memcpy(&toSend485[0], &request[6], (nBytes_read - 6));

        // Aggiungo il CRC ModBus
        nBytes_write = addCrc16(&toSend485[0], nBytes_write);

        // Output console
        if(settings.verbose > 1){
            printMillis();
            printf("-> TX RTU [%3zu]: ", nBytes_write);

            for(int i = 0; i < nBytes_write; i++){
                printf("%02x ", toSend485[i]);
            }
            
            printf("\n");
        }

        // Serial.flush
        tcflush(serialPort, TCIOFLUSH);

        // Invio il pacchetto sulla seriale
        write(serialPort, toSend485, nBytes_write);

        // Creo un nuovo buffer con il pacchetto ricevuto dalla 485
        uint8_t received485[BUFSIZE_MODBUS];

        // Leggo risposta 485
        nBytes_read = 0;

        long startMillis = millis();
        
        // Continuo a spazzolare il buffer fino a che raggiungo la lunghezza che mi aspetto o vado in timeout
        while(nBytes_read < responseLen){
            ssize_t currRead = read(serialPort, &received485[nBytes_read], responseLen - nBytes_read);
            nBytes_read += currRead;

            if(settings.verbose > 2){
                printf("currRead: %zu\n", currRead);
            }

            if((millis() - startMillis) > settings.rtu.timeout){
                if(settings.verbose > 0){
                    printMillis();
                    printf("Timed out\n");
                }
                break;
            }
        }

        // Timeout risposta
        if(nBytes_read == 0){
            close(client_sockfd);
            continue;
        }

        // Output console
        if(settings.verbose > 1){
            printMillis();
            printf("<- RX RTU [%3zu]: ", nBytes_read);
            
            for(int i = 0; i < nBytes_read; i++){
                printf("%02x ", received485[i]);
            }
            
            printf("\n");
        }

        // Check CRC
        checkCrc16(received485, nBytes_read);

        // Creo un nuovo buffer con il pacchetto di risposta TCP
        uint8_t response[256];

        // I primi 6 byte del pacchetto (header) sono uguali alla request
        memcpy(&response[0], &request[0], 6);


        if(nBytes_read > 2){
            memcpy(&response[6], &received485[0], nBytes_read - 2);
            nBytes_write = nBytes_read + 4;

            // Output console
            if(settings.verbose > 1){
                printMillis();
                printf("-> TX TCP [%3zu]: ", nBytes_write);

                for(int i = 0; i < nBytes_write; i++){
                    printf("%02x ", response[i]);
                }

                printf("\n");
            }

            // Invio il pacchetto TCP
            write(client_sockfd, response, nBytes_write);
        }else{
            printMillis();
            printf("Not enough bytes received from RTU\n");
        }

        // Chiudo la socket
        close(client_sockfd);
    }

    return EXIT_SUCCESS;
}
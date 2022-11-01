/**
 * @file main.c
 * @author Federico Turco ()
 * @brief 
 * @version 1.0
 * @date 2022-02-15
 * 
 * @copyright Copyright (c) Turco Federico 2022
 * 
 */

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
#include "crc.h"

#include "main.h"



// Global vars
config settings;
struct termios tty;


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
    int serialPort = configureSerial(&settings, &tty);

    // Flush buffer input/output
    tcflush(serialPort, TCIOFLUSH);

    // Info
    if(settings.verbose){
        printMillis();
        printf("Starting server at %s:%i\n", settings.tcp.address, settings.tcp.port);
    }

    // Configuro socket
    int socket = configureSocket(&settings);

    if(settings.verbose){
        printMillis();
        printf("Ok, Running\n\n");
    }

    while (1) {
        // Definizioni client
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        // Connessione in ingresso
        int client_sockfd = accept(socket, (struct sockaddr*)&client_address,&client_len);

        if(client_sockfd != -1)
        {
            // Buffer
            uint8_t buf_0[BUFSIZE_MODBUS];
            uint8_t buf_1[BUFSIZE_MODBUS];
            uint8_t headerTCP[6];

            ssize_t nBytes = read(client_sockfd, buf_0, BUFSIZE_TCP);

            // Errore lettura
            if(nBytes == -1)
            {
                close(client_sockfd);
                continue;
            }

            // Scarto pacchetti troppo corti
            if(nBytes < 8)
            {
                close(client_sockfd);
                continue;
            }

            memcpy(headerTCP, buf_0, 0);

            struct in_addr ipAddr = client_address.sin_addr;

            char clientPort[6]; 
            char clientName[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_address.sin_addr, clientName, INET_ADDRSTRLEN);

            // Info connessione in ingresso
            if(settings.verbose > 1){
                printMillis();
                printf("Accepted connection from %s:%i\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            }

            // Output console
            if(settings.verbose > 1){
                printMillis();
                printf("<- RX TCP [%3zu]: ", nBytes);

                for(int i = 0; i < nBytes; i++){
                    printf("%02x ", buf_0[i]);
                }

                printf("\n");
            }

            // Controllo protocol identifier che sia 00 00
            if(buf_0[2] != 0 || buf_0[3] != 0){
                printf("Protocol identifier not valid [%2x %2x], for ModBus TCP it should be 0\n", buf_0[2], buf_0[3]);
                close(client_sockfd);
                continue;
            }

            ssize_t messageLen = (buf_0[4] << 8) + buf_0[5];
            ssize_t responseLen = 0;

            if((messageLen + 6) != nBytes){
                printMillis();
                printf("ERROR: incoming buffer length [%3zu] != MosBus packet length + 6 [%3zu]\n", nBytes, messageLen);
            }

            // FC01, FC02
            if(buf_0[7] == 0x01 || buf_0[7] == 0x02){
                responseLen = ((((buf_0[10]) << 8) + (buf_0[11])) / 8) + ((((buf_0[10]) << 8) + (buf_0[11])) % 8 != 0) + 5; // 1 slave id, 1 FC, 1 len, 2 byte CRC
            }

            // FC03, FC04
            else if(buf_0[7] == 0x03 || buf_0[7] == 0x04){
                responseLen = ((((buf_0[10]) << 8) + (buf_0[11])) * 2) + 5; // 1 slave id, 1 FC, 1 len, 2 byte CRC
            }

            // FC05, FC06 
            else if(buf_0[7] == 0x05 || buf_0[7] == 0x06){
                responseLen = 8;     // Risposta a lunghezza fissa di 8 bytes
            }

            // FC15, FC16
            else if(buf_0[7] == 0x0F || buf_0[7] == 0x10){
                responseLen = 8;     // Risposta a lunghezza fissa di 8 bytes
            }

            // FC08
            else if(buf_0[7] == 0x08){
                responseLen = 6;     // Risposta a lunghezza fissa di 6 bytes
            }

            // Unknown Function Code
            else{
                printMillis();
                printf("Unknow function code FC %2i", buf_0[7]);
                close(client_sockfd);
                continue;
            }

            // Creo un nuovo buffer con il pacchetto da inviare sulla 485
            nBytes = nBytes - 6;
            memcpy(&buf_1[0], &buf_0[6], nBytes);

            // Aggiungo il CRC ModBus
            nBytes = addCrc16(&buf_1[0], nBytes);

            // Output console
            if(settings.verbose > 1){
                printMillis();
                printf("-> TX RTU [%3zu]: ", nBytes);

                for(int i = 0; i < nBytes; i++){
                    printf("%02x ", buf_1[i]);
                }
                
                printf("\n");
            }

            // Serial.flush
            tcflush(serialPort, TCIFLUSH);

            // Invio il pacchetto sulla seriale
            write(serialPort, buf_1, nBytes);

            // Leggo risposta 485
            nBytes = 0;

            uint64_t startMillis = millis();
            
            // Continuo a spazzolare il buffer fino a che raggiungo la lunghezza attesa o vado in timeout
            while(nBytes < responseLen){
                ssize_t currRead = read(serialPort, &buf_0[nBytes], responseLen - nBytes);
                nBytes += currRead;

                if(settings.verbose > 2){
                    printf("currRead: %zu\n", currRead);
                }

                if((millis() - startMillis) > settings.rtu.timeout){
                    if(settings.verbose){
                        printMillis();
                        printf("Timed out\n");
                    }
                    break;
                }

                if(startMillis > millis()){
                    if(settings.verbose){
                        printMillis();
                        printf("Read aborted (millis() overflow)\n");
                    }
                    break;
                }
            }

            // Timeout risposta
            if(nBytes == 0){
                close(client_sockfd);
                continue;
            }

            // Output console
            if(settings.verbose > 1){
                printMillis();
                printf("<- RX RTU [%3zu]: ", nBytes);
                
                for(int i = 0; i < nBytes; i++){
                    printf("%02x ", buf_0[i]);
                }
                
                printf("\n");
            }

            // Check CRC
            if(checkCrc16(buf_0, nBytes)){
                close(client_sockfd);
                continue;
            }

            // I primi 6 byte del pacchetto (header) sono uguali alla request
            memcpy(&buf_1, &headerTCP, 6);

            if(nBytes > 2){
                memcpy(&buf_1[6], &buf_0, nBytes - 2);
                nBytes = nBytes + 4;

                // Output console
                if(settings.verbose > 1){
                    printMillis();
                    printf("-> TX TCP [%3zu]: ", nBytes);

                    for(int i = 0; i < nBytes; i++){
                        printf("%02x ", buf_1[i]);
                    }

                    printf("\n");
                }

                // Invio il pacchetto TCP
                write(client_sockfd, buf_1, nBytes);
            }else{
                printMillis();
                printf("Not enough bytes received from RTU\n");
            }

            // Chiudo la socket
            close(client_sockfd);
        }
    }

    return EXIT_SUCCESS;
}
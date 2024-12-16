#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#define write(sock, buf, len) send(sock, buf, len, 0)
#define read(sock, buf, len) recv(sock, buf, len, 0)

#define PORT 3500
#define SERVER_ADDR "your ip" 

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in serverAddr;
    char buffer[256];
    int n;

    // Inizializza Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Errore nell'inizializzazione di Winsock\n");
        return 1;
    }

    // Create a new socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Errore nella creazione del socket\n");
        WSACleanup();
        return 1;
    }

    // Init address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr); 

    // Try connection to server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Errore nella connessione al server\n");
        close(sockfd);
        WSACleanup();
        return 1;
    }

    // Send 'receiver' flag 
    char *receiverTag = "receiver";
    write(sockfd, receiverTag, strlen(receiverTag));

    while (1) {
        memset(buffer, 0, 256);  // Clean buffer

        n = read(sockfd, buffer, 255);
        printf("-----\n");
        printf("Ho letto %d caratteri.\n", n);

        if (n < 0) {
            printf("Errore nella lettura dei dati dal server\n");
            close(sockfd);
            WSACleanup();
            return 1;
        }

        // If the server sent a serial number (and not a closed connection), print it out
        if (n > 0) {
            printf("Numero di serie ricevuto dal server: %s\n", buffer);
        } else {
            // If the server has closed the connection, don't get out of the loop, but keep waiting
            printf("Nessun seriale disponibile, in attesa di nuovi dati...\n");
        }
        printf("-----\n");
        Sleep(1000);  // 1000 ms = 1 secondo
    }

    close(sockfd);
    WSACleanup();

    return 0;
}

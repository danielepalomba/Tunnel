#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 3500
#define MAX_CLIENTS 2  // Numero massimo di client (1 giver e 1 receiver)

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void clean_buffer(char *buffer) {
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[len - 1] = '\0';
        len--;
    }
}

int main() {
    int serverFd, clientFd, client_add_len, n;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[256];

    /*The receiver remains connected until it decides to disconnect, so its connection must be maintained and managed.*/    
    int receiverFd = -1; // File descriptor for the client receiver
    int activeReceiver = 0; // Receiver connection state

    // Server socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        error("Errore nella creazione della socket");
    }

    // Init server address 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT); 
    serv_addr.sin_addr.s_addr = INADDR_ANY; //allow all IP's

    // Bind SOCKET->ADDRESS
    if (bind(serverFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Errore nel binding della socket");
    }

    // Put the server listening
    listen(serverFd, MAX_CLIENTS);

    printf("Server in ascolto sulla porta %d...\n", PORT);

    // Main cycle of the server
    while (1) {
        client_add_len = sizeof(cli_addr); //client address len
        clientFd = accept(serverFd, (struct sockaddr *) &cli_addr, &client_add_len); //accept connection

        if (clientFd < 0) {
            perror("Errore nell'accettazione della connessione");
            continue;
        }

        // Clear the buffer & read the client msg
        bzero(buffer, 256);
        n = read(clientFd, buffer, 255);
        if (n < 0) {
            perror("Errore nella lettura dal socket");
            close(clientFd);
            continue;
        }

        buffer[n] = '\0'; // end of line
        clean_buffer(buffer);

        // if it's a receiver, manage it
        if (strcmp(buffer, "receiver") == 0) {
            if (activeReceiver == 1) {
                // If there is already a receiver connected, close the connection of the old receiver
                printf("Un receiver ha tentato di connettersi. Sostituendo il vecchio receiver.\n");
                close(receiverFd);
                activeReceiver = 0;  // Set inactive
            }

            // New receiver
            printf("Receiver connesso!\n");
            receiverFd = clientFd; 
            activeReceiver = 1;
            write(clientFd, "Connesso come receiver.\n", 24);
        } 
        // if it's a giver
        else if (strcmp(buffer, "giver") == 0) {
            bzero(buffer, 256);
            n = read(clientFd, buffer, 255);
            if (n < 0) {
                perror("Errore nella lettura del seriale dal giver.");
                write(clientFd, "Errore nella ricezione del seriale.\n", 35);
                close(clientFd);
                continue;
            }

            buffer[n] = '\0';
            clean_buffer(buffer);
            printf("Giver ha inviato un seriale: %s\n", buffer);

            // If a receiver is connected, it sends the serial to the receiver
            if (activeReceiver && receiverFd != -1) {
                n = write(receiverFd, buffer, strlen(buffer));
                if (n < 0) {
                    perror("Errore nell'invio del seriale al receiver.");
                    activeReceiver = 0; // Flag as inactive!
                    close(receiverFd); //release receiver
                    receiverFd = -1;
                } else {
                    printf("Seriale inviato al receiver con successo.\n");
                    write(clientFd, "Seriale inviato con successo al receiver.\n", 40);
                }
            } else {
                printf("Nessun receiver connesso, seriale '%s' scartato.\n", buffer);
                write(clientFd, "Errore: nessun receiver connesso.\n", 34);
            }

            // Close connection giver after managed the message
            close(clientFd);
        }
        // Unknown type
        else {
            printf("Tipo di client sconosciuto: %s. Connessione chiusa.\n", buffer);
            write(clientFd, "Errore: tipo di client sconosciuto.\n", 36);
            close(clientFd);
        }

        // Periodic check of receiver connection
        if (activeReceiver) {
            char testBuffer[1];
            ssize_t test = recv(receiverFd, testBuffer, sizeof(testBuffer), MSG_PEEK | MSG_DONTWAIT);
            if (test <= 0) {
                if (errno != EWOULDBLOCK && errno != EINTR) {
                    printf("Receiver disconnesso.\n");
                    activeReceiver = 0;
                    close(receiverFd);
                    receiverFd = -1;
                }
            }
        }
    }

    close(serverFd);
    return 0;
}

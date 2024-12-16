#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#define write(sock, buf, len) send(sock, buf, len, 0)
#define read(sock, buf, len) recv(sock, buf, len, 0)
#define SERVER_ADDR "your ip" 

void clean_string(char *str) {
    size_t len = strlen(str);
    size_t i = 0, j = 0;

    // Remove leading and trailing spaces
    while (isspace(str[i])) i++;
    while (len > 0 && isspace(str[len - 1])) len--;

    // Copy the cleaned string
    while (i < len) {
        str[j++] = str[i++];
    }
    str[j] = '\0';
}

int main() {
    int sockfd;
    char buffer[1024];
    struct sockaddr_in serv_addr;
    char serial_number[128] = {0};

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error initializing Winsock.\n");
        return 1;
    }

    // Use CreateProcess and ReadFile to execute PowerShell command and read output
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        printf("Error creating pipe.\n");
        return 1;
    }

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;

    if (!CreateProcess(NULL, "powershell.exe -Command \"Get-CimInstance -ClassName Win32_BIOS | Select-Object -ExpandProperty SerialNumber\"", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        printf("Error starting PowerShell process.\n");
        return 1;
    }

    CloseHandle(hWrite);

    DWORD bytesRead;
while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL)) {
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        if (buffer[bytesRead - 1] == '\n') {
            buffer[bytesRead - 1] = '\0';
        }
        strncpy(serial_number, buffer, sizeof(serial_number) - 1);
        break;
    }
}


    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Clean the serial number string
    clean_string(serial_number);

    // Verify the result
    if (strlen(serial_number) == 0 || strcmp(serial_number, "default string") == 0) {
        printf("Error: Serial number not found.\n");
        return 1;
    }

    printf("Serial number: %s\n", serial_number);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Errore nella creazione del socket.\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(3500); // Porta del server
    inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr); 

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Errore nella connessione al server.\n");
        close(sockfd);
        return 1;
    }
    
    if (write(sockfd, "giver", 5) < 0) {
     printf("Errore nell'invio del tag.\n");
     close(sockfd);
     return 1;
   }

   /*necessary to prevent overlapping strings in the buffer, especially
    the server's computing power is low
   */
    Sleep(100);

    if (write(sockfd, serial_number, strlen(serial_number)) < 0) {
     printf("Errore nell'invio del numero di serie.\n");
     close(sockfd);
     return 1;
   }

    close(sockfd);

    WSACleanup();

    return 0;
}

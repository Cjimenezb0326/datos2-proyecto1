#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>

// Necesario para enlazar con Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

int wmain()
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    sockaddr_in clientService;
    const char* sendbuf = "GET_MEMORY_STATUS";
    char recvbuf[512];
    int recvbuflen = 512;

    // Inicializar Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Crear un socket
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        wprintf(L"Socket creation failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configurar la dirección del servidor
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(27015);
    if (inet_pton(AF_INET, "127.0.0.1", &clientService.sin_addr) != 1) {
        wprintf(L"Invalid address\n");
        WSACleanup();
        return 1;
    }

    // Conectar al servidor
    iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"Connection failed: %ld\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    wprintf(L"Connected to server.\n");

    // Enviar datos al servidor
    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"Send failed: %ld\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    wprintf(L"Bytes sent: %d\n", iResult);

    // Recibir respuesta del servidor
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
        printf("Bytes received: %d\nMessage: %.*s\n", iResult, iResult, recvbuf);
    else if (iResult == 0)
        wprintf(L"Connection closed\n");
    else
        wprintf(L"Recv failed: %ld\n", WSAGetLastError());

    // Cerrar conexión
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

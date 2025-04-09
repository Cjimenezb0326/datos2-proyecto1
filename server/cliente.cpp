#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

void sendCommand(SOCKET ConnectSocket, const std::string& command) {
    int iResult = send(ConnectSocket, command.c_str(), (int)command.size(), 0);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "send falló con el error: " << WSAGetLastError() << std::endl;
    }
}

std::string receiveResponse(SOCKET ConnectSocket) {
    char recvBuffer[512];
    int iResult = recv(ConnectSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (iResult > 0) {
        recvBuffer[iResult] = '\0';  // Agregar terminador de cadena
        return std::string(recvBuffer);
    }
    else if (iResult == 0) {
        return "Conexión cerrada por el servidor.";
    }
    else {
        std::cerr << "recv falló con error: " << WSAGetLastError() << std::endl;
        return "";
    }
}

int main() {
    // Inicializar Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup falló con el error: " << iResult << std::endl;
        return 1;
    }

    // Crear el socket del cliente
    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Configurar la dirección del servidor
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(27015);  // Puerto del servidor
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);  // IP del servidor (localhost)

    // Conectarse al servidor
    iResult = connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Error al conectar con el servidor: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Conectado al servidor!" << std::endl;

    std::string command;
    while (true) {
        std::cout << "Ingrese un comando (CREATE, SET, GET, IncreaseRefCount, DecreaseRefCount, DUMP, Exit): ";
        std::getline(std::cin, command);

        if (command == "Exit") {
            sendCommand(ConnectSocket, "Exit");
            break;
        }

        // Enviar comando al servidor
        sendCommand(ConnectSocket, command);

        // Recibir y mostrar la respuesta del servidor
        std::string response = receiveResponse(ConnectSocket);
        std::cout << "Respuesta del servidor: " << response << std::endl;

        // El cliente espera otro comando, por lo que el ciclo continúa
    }

    // Cerrar el socket
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

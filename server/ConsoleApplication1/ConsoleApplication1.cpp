#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

int listenPort = 27015;
size_t memSize = 10 * 1024 * 1024;
std::string dumpFolder = "./dumps";
void* memory = nullptr;
size_t nextId = 1; // Para asignar IDs únicos

// Función para generar el nombre del archivo de volcado con fecha y hora
std::string generateDumpFilename() {
    std::time_t t = std::time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);
    char filename[100];
    snprintf(filename, sizeof(filename), "%04d-%02d-%02d_%02d-%02d-%02d.dump",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return dumpFolder + "/" + filename;
}

// Función para enviar respuestas al cliente
void sendResponse(SOCKET ClientSocket, const std::string& response) {
    int iResult = send(ClientSocket, response.c_str(), (int)response.size(), 0);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "send falló con el error: " << WSAGetLastError() << std::endl;
    }
}

// Función para manejar la comunicación con un cliente
void handleClient(SOCKET ClientSocket) {
    char buffer[1024];
    int iResult;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        iResult = recv(ClientSocket, buffer, sizeof(buffer), 0);
        if (iResult <= 0) break;

        // Mostrar lo que se recibe del cliente
        std::cout << "Comando recibido: " << buffer << std::endl;

        std::istringstream command(buffer);
        std::string action;
        command >> action;

        if (action == "CREATE") {
            size_t size;
            std::string type;
            command >> size >> type;
            std::cout << "Comando CREATE recibido: tamaño = " << size << ", tipo = " << type << std::endl;

            // Aquí puedes crear un espacio de memoria en la memoria reservada
            size_t id = nextId++;  // Asignar un ID único para este bloque de memoria
            std::string response = "ID: " + std::to_string(id);  // Responder con el ID generado
            sendResponse(ClientSocket, response);
        }
        else if (action == "SET") {
            size_t id;
            std::string value;
            command >> id >> value;
            std::cout << "Comando SET recibido: id = " << id << ", valor = " << value << std::endl;
            // Aquí deberías almacenar el valor en el bloque de memoria identificado por 'id'
            sendResponse(ClientSocket, "OK");
        }
        else if (action == "GET") {
            size_t id;
            command >> id;
            std::cout << "Comando GET recibido: id = " << id << std::endl;
            // Aquí deberías devolver el valor almacenado en el bloque de memoria identificado por 'id'
            sendResponse(ClientSocket, "Valor del bloque de memoria");
        }
        else if (action == "INCREASE") {
            size_t id;
            command >> id;
            std::cout << "Comando INCREASE recibido: id = " << id << std::endl;
            // Aquí deberías incrementar el contador de referencias para el bloque de memoria 'id'
            sendResponse(ClientSocket, "RefCount incrementado");
        }
        else if (action == "DECREASE") {
            size_t id;
            command >> id;
            std::cout << "Comando DECREASE recibido: id = " << id << std::endl;
            // Aquí deberías disminuir el contador de referencias para el bloque de memoria 'id'
            sendResponse(ClientSocket, "RefCount decrementado");
        }
        else if (action == "DUMP") {
            std::ofstream dumpFile(generateDumpFilename(), std::ios::binary);
            dumpFile.write((char*)memory, memSize);
            std::cout << "Comando DUMP recibido, generando archivo de volcado." << std::endl;
            sendResponse(ClientSocket, "Volcado de memoria generado");
        }
        else {
            std::cout << "Comando no reconocido: " << action << std::endl;
            sendResponse(ClientSocket, "Comando no reconocido");
        }
    }

    closesocket(ClientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(listenPort);
    bind(ListenSocket, (SOCKADDR*)&service, sizeof(service));
    listen(ListenSocket, SOMAXCONN);

    memory = malloc(memSize);  // Reservar memoria inicial
    if (memory == nullptr) {
        std::cerr << "Error al reservar memoria" << std::endl;
        return 1;
    }

    std::cout << "Servidor escuchando en el puerto " << listenPort << std::endl;

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket != INVALID_SOCKET) {
            handleClient(ClientSocket);
        }
    }

    free(memory);
    WSACleanup();
    return 0;
}

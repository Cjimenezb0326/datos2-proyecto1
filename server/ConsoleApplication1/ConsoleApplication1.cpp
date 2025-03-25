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

// Necesitas vincular con Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")
// Variables globales para los parámetros, definidos directamente en el código
int listenPort = 27015;      // Puerto de escucha
size_t memSize = 10 * 1024 * 1024;  // Tamaño de la memoria (10 MB por defecto)
std::string dumpFolder = "./dumps";  // Carpeta para los dumps

// Función para generar el nombre del archivo de dump basado en la fecha y hora actual
std::string generateDumpFilename() {
    // Obtener la hora actual
    std::time_t t = std::time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);

    // Formato de fecha y hora: YYYY-MM-DD_HH-MM-SS-SSS
    char filename[100];
    snprintf(filename, sizeof(filename), "%04d-%02d-%02d_%02d-%02d-%02d-%03d.dump",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        0); // Aquí puedes agregar milisegundos si lo deseas

    return dumpFolder + "/" + filename;
}

int main() {
    // Validar que el tamaño de la memoria sea mayor que 0
    if (memSize == 0) {
        wprintf(L"El tamaño de la memoria (memsize) debe ser mayor a 0\n");
        return 1;
    }

    // Inicializar Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup falló con el error: %d\n", iResult);
        return 1;
    }

    //----------------------
    // Crear el socket para escuchar conexiones
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        wprintf(L"Error al crear el socket: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //----------------------
    // Preparar la dirección del servidor
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;  // Acepta conexiones desde cualquier IP
    service.sin_port = htons(listenPort);  // Puerto de escucha

    //----------------------
    // Enlazar el socket con la dirección y puerto
    iResult = bind(ListenSocket, (SOCKADDR*)&service, sizeof(service));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"bind falló con el error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    //----------------------
    // Escuchar conexiones entrantes
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"listen falló con el error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    wprintf(L"Servidor escuchando en el puerto %d...\n", listenPort);

    //----------------------
    // Aceptar una conexión entrante
    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        wprintf(L"accept falló con el error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    wprintf(L"Conexión aceptada desde un cliente.\n");

    //----------------------
    // Reservar memoria
    void* memory = malloc(memSize);
    if (!memory) {
        wprintf(L"No se pudo reservar la memoria solicitada de %zu bytes\n", memSize);
        return 1;
    }

    //----------------------
    // Enviar un mensaje al cliente
    const char* sendMessage = "Hola desde el servidor!";
    iResult = send(ClientSocket, sendMessage, (int)strlen(sendMessage), 0);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send falló con el error: %ld\n", WSAGetLastError());
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    wprintf(L"Mensaje enviado al cliente.\n");

    //----------------------
    // Generar el archivo de dump
    std::string dumpFilename = generateDumpFilename();
    std::ofstream dumpFile(dumpFilename);
    dumpFile << "Estado de la memoria..." << std::endl;
    dumpFile << "Tamaño de la memoria: " << memSize << " bytes" << std::endl;
    dumpFile.close();

    wprintf(L"Dump generado: %S\n", dumpFilename.c_str());  // Usar %S para strings estándar

    //----------------------
    // Liberar memoria
    free(memory);

    //----------------------
    // Cerrar las conexiones
    iResult = closesocket(ClientSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket falló con el error: %ld\n", WSAGetLastError());
    }

    iResult = closesocket(ListenSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket falló con el error: %ld\n", WSAGetLastError());
    }

    //----------------------
    // Finalizar Winsock
    WSACleanup();

    return 0;
}


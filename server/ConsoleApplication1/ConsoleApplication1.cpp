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
#include <map>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

int listenPort = 27015;
size_t memSize = 10 * 1024 * 1024; // 10 MB
std::string dumpFolder = "./dumps";
void* memory = nullptr;
size_t nextId = 1; // Para asignar IDs únicos

// Estructura de bloque de memoria
struct MemoryBlock {
    size_t id;
    std::string type;
    size_t size;
    char* ptr;
    std::string value;
    int refCount;
};

// Mapa para almacenar los bloques de memoria por ID
std::map<size_t, MemoryBlock> memoryBlocks;
char* memoryCursor = nullptr;

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
#include <iostream>

// Función para desfragmentar la memoria
void defragment() {
    char* newCursor = static_cast<char*>(memory);
    bool fragmented = false;  // Flag para saber si hubo movimientos

    for (auto& pair : memoryBlocks) {
        if (pair.second.refCount > 0) {
            if (pair.second.ptr != newCursor) {
                std::cout << "Desfragmentando: Moviendo bloque desde " 
                          << static_cast<void*>(pair.second.ptr) 
                          << " a " << static_cast<void*>(newCursor) << std::endl;
                memmove(newCursor, pair.second.ptr, pair.second.size);
                pair.second.ptr = newCursor;
                fragmented = true;
            }
            newCursor += pair.second.size;
        }
    }

    memoryCursor = newCursor;

    if (!fragmented) {
        std::cout << "No se realizaron movimientos de desfragmentacion." << std::endl;
    }
}

// Función para limpiar la memoria no utilizada
void garbage_collector() {
    bool deletedAnyBlock = false;  // Flag para saber si se eliminaron bloques

    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ) {
        if (it->second.refCount <= 0) {  // Si no hay referencias al bloque
            std::cout << "Eliminando bloque con puntero " 
                      << static_cast<void*>(it->second.ptr) 
                      << " porque su refCount es " << it->second.refCount << std::endl;
            it = memoryBlocks.erase(it);  // Elimina el bloque de la lista
            deletedAnyBlock = true;
        }
        else {
            ++it;
        }
    }

    if (!deletedAnyBlock) {
        std::cout << "No se eliminaron bloques de memoria." << std::endl;
    }
}

void defragAndClean() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));  
        std::cout << "Iniciando recoleccion de basura..." << std::endl;
        garbage_collector();
        std::cout << "Iniciando desfragmentacion de memoria..." << std::endl;
        defragment();
    }
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

        std::cout << "Comando recibido: " << buffer << std::endl;

        std::istringstream command(buffer);
        std::string action;
        command >> action;

        if (action == "CREATE") {
            size_t size;
            std::string type;
            command >> size >> type;

            if ((memoryCursor + size) > ((char*)memory + memSize)) {
                sendResponse(ClientSocket, "ERROR: Memoria insuficiente");
                continue;
            }

            size_t id = nextId++;
            MemoryBlock block;
            block.id = id;
            block.type = type;
            block.size = size;
            block.ptr = memoryCursor;
            block.refCount = 1;

            memoryBlocks[id] = block;
            memoryCursor += size;

            std::string response = "ID: " + std::to_string(id);
            sendResponse(ClientSocket, response);
        }
        else if (action == "SET") {
            size_t id;
            std::string value;
            command >> id >> value;

            if (memoryBlocks.find(id) != memoryBlocks.end()) {
                MemoryBlock& block = memoryBlocks[id];
                if (value.size() > block.size) {
                    sendResponse(ClientSocket, "ERROR: Valor excede tamaño del bloque");
                    continue;
                }

                memcpy(block.ptr, value.c_str(), value.size());
                block.value = value;
                sendResponse(ClientSocket, "OK");
            }
            else {
                sendResponse(ClientSocket, "ERROR: ID no encontrado");
            }
        }
        else if (action == "GET") {
            size_t id;
            command >> id;

            if (memoryBlocks.find(id) != memoryBlocks.end()) {
                MemoryBlock& block = memoryBlocks[id];
                std::string value(block.ptr, block.size);
                sendResponse(ClientSocket, "Valor: " + value);
            }
            else {
                sendResponse(ClientSocket, "ERROR: ID no encontrado");
            }
        }
        else if (action == "INCREASE") {
            size_t id;
            command >> id;
            if (memoryBlocks.find(id) != memoryBlocks.end()) {
                memoryBlocks[id].refCount++;
                sendResponse(ClientSocket, "RefCount: " + std::to_string(memoryBlocks[id].refCount));
            }
            else {
                sendResponse(ClientSocket, "ERROR: ID no encontrado");
            }
        }
        else if (action == "DECREASE") {
            size_t id;
            command >> id;
            if (memoryBlocks.find(id) != memoryBlocks.end()) {
                memoryBlocks[id].refCount--;
                sendResponse(ClientSocket, "RefCount: " + std::to_string(memoryBlocks[id].refCount));
            }
            else {
                sendResponse(ClientSocket, "ERROR: ID no encontrado");
            }
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

    memoryCursor = static_cast<char*>(memory);

    std::cout << "Servidor escuchando en el puerto " << listenPort << std::endl;

    std::thread defrag_garbage_collector_Thread(defragAndClean);
    defrag_garbage_collector_Thread.detach();

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

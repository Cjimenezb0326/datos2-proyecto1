#ifndef MPOINTER_H
#define MPOINTER_H

#include <iostream>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdexcept>
#include <map>
#include <sstream>
#include <typeinfo>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

template <class T>
class MPointer {
private:
    size_t id;
    static SOCKET ConnectSocket;
    static bool initialized;
    static std::mutex socketMutex;
    static std::map<size_t, int> refCounts;

    MPointer(size_t blockId) : id(blockId) {}

    static void sendCommand(const std::string& command) {
        std::lock_guard<std::mutex> lock(socketMutex);
        int iResult = send(ConnectSocket, command.c_str(), (int)command.size(), 0);
        if (iResult == SOCKET_ERROR) {
            throw std::runtime_error("Error al enviar comando: " + std::to_string(WSAGetLastError()));
        }
    }

    static std::string receiveResponse() {
        std::lock_guard<std::mutex> lock(socketMutex);
        char recvBuffer[512];
        int iResult = recv(ConnectSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (iResult > 0) {
            recvBuffer[iResult] = '\0';
            return std::string(recvBuffer);
        }
        else if (iResult == 0) {
            throw std::runtime_error("Conexión cerrada por el servidor");
        }
        else {
            throw std::runtime_error("Error al recibir respuesta: " + std::to_string(WSAGetLastError()));
        }
    }

public:
    MPointer() : id(0) {}
    static void Init(SOCKET existingSocket = INVALID_SOCKET, int port = 27015) {
        if (initialized) return;

        if (existingSocket != INVALID_SOCKET) {
            ConnectSocket = existingSocket;
            initialized = true;
            return;
        }

        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            throw std::runtime_error("WSAStartup falló: " + std::to_string(iResult));
        }

        ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ConnectSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Error al crear socket: " + std::to_string(WSAGetLastError()));
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

        iResult = connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            WSACleanup();
            throw std::runtime_error("Error al conectar: " + std::to_string(WSAGetLastError()));
        }

        initialized = true;
    }

    static MPointer<T> New(size_t size = sizeof(T), const std::string& type = typeid(T).name()) {
        if (!initialized) {
            throw std::runtime_error("MPointer no inicializado");
        }

        std::string command = "CREATE " + std::to_string(size) + " " + type;
        sendCommand(command);
        std::string response = receiveResponse();

        if (response.find("ID: ") == 0) {
            size_t newId = std::stoul(response.substr(4));
            {
                std::lock_guard<std::mutex> lock(socketMutex);
                refCounts[newId] = 1;
            }
            return MPointer<T>(newId);
        }
        throw std::runtime_error("Error al crear bloque: " + response);
    }

    MPointer(const MPointer& other) : id(other.id) {
        if (id != 0) {
            std::lock_guard<std::mutex> lock(socketMutex);
            refCounts[id]++;
            sendCommand("INCREASE " + std::to_string(id));
            receiveResponse();
        }
    }

    MPointer& operator=(const MPointer& other) {
        if (this != &other) {
            if (id != 0) {
                std::lock_guard<std::mutex> lock(socketMutex);
                if (--refCounts[id] == 0) {
                    sendCommand("DECREASE " + std::to_string(id));
                    receiveResponse();
                    refCounts.erase(id);
                }
            }

            id = other.id;
            if (id != 0) {
                std::lock_guard<std::mutex> lock(socketMutex);
                refCounts[id]++;
                sendCommand("INCREASE " + std::to_string(id));
                receiveResponse();
            }
        }
        return *this;
    }

    T operator*() const {
        if (id == 0) throw std::runtime_error("Dereferencia de MPointer nulo");

        sendCommand("GET " + std::to_string(id));
        std::string response = receiveResponse();

        if (response.find("Valor: ") == 0) {
            std::istringstream iss(response.substr(7));
            T value;
            if constexpr (std::is_same_v<T, std::string>) {
                return response.substr(7);
            }
            else {
                iss >> value;
                return value;
            }
        }
        throw std::runtime_error("Error al obtener valor: " + response);
    }

    void operator=(const T& value) {
        if (id == 0) throw std::runtime_error("Asignación a MPointer nulo");

        std::ostringstream oss;
        oss << value;
        sendCommand("SET " + std::to_string(id) + " " + oss.str());
        std::string response = receiveResponse();
        if (response != "OK") {
            throw std::runtime_error("Error al establecer valor: " + response);
        }
    }

    size_t getId() const { return id; }

    ~MPointer() {
        if (id != 0 && initialized) {
            try {
                std::lock_guard<std::mutex> lock(socketMutex);
                if (--refCounts[id] == 0) {
                    sendCommand("DECREASE " + std::to_string(id));
                    receiveResponse();
                    refCounts.erase(id);
                }
            }
            catch (...) {}
        }
    }

    static void Close() {
        if (initialized) {
            closesocket(ConnectSocket);
            WSACleanup();
            initialized = false;
        }
    }
};

template <class T> SOCKET MPointer<T>::ConnectSocket = INVALID_SOCKET;
template <class T> bool MPointer<T>::initialized = false;
template <class T> std::mutex MPointer<T>::socketMutex;
template <class T> std::map<size_t, int> MPointer<T>::refCounts;

#endif // MPOINTER_H
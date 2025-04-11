#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "MPointer.h"

#pragma comment(lib, "ws2_32.lib")

void sendCommand(SOCKET socket, const std::string& command) {
    int iResult = send(socket, command.c_str(), (int)command.size(), 0);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "send falló con el error: " << WSAGetLastError() << std::endl;
    }
}

std::string receiveResponse(SOCKET socket) {
    char recvBuffer[512];
    int iResult = recv(socket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (iResult > 0) {
        recvBuffer[iResult] = '\0';
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

void basicCommandsMenu(SOCKET socket) {
    std::string command;
    while (true) {
        std::cout << "\n--- Comandos Basicos ---\n";
        std::cout << "1. CREATE <tamano> <tipo>\n";
        std::cout << "2. SET <id> <valor>\n";
        std::cout << "3. GET <id>\n";
        std::cout << "4. INCREASE <id>\n";
        std::cout << "5. DECREASE <id>\n";
        std::cout << "6. LISTA\n"; // Nueva opción para LISTA
        std::cout << "7. Volver al menu principal\n";
        std::cout << "Seleccione una opcion: ";

        std::getline(std::cin, command);

        if (command == "7") break;

        if (command == "1") {
            std::cout << "Ingrese tamano y tipo: ";
            std::getline(std::cin, command);
            command = "CREATE " + command;
        }
        else if (command == "2") {
            std::cout << "Ingrese ID y valor: ";
            std::getline(std::cin, command);
            command = "SET " + command;
        }
        else if (command == "3") {
            std::cout << "Ingrese ID: ";
            std::getline(std::cin, command);
            command = "GET " + command;
        }
        else if (command == "4") {
            std::cout << "Ingrese ID: ";
            std::getline(std::cin, command);
            command = "INCREASE " + command;
        }
        else if (command == "5") {
            std::cout << "Ingrese ID: ";
            std::getline(std::cin, command);
            command = "DECREASE " + command;
        }
        else if (command == "6") { // Manejo del comando LISTA
            command = "LISTA";
        }

        sendCommand(socket, command);
        std::string response = receiveResponse(socket);
        std::cout << "Respuesta: " << response << std::endl;
    }
}

void mpointerMenu() {
    MPointer<int> currentPtr;
    std::string option;
    while (true) {
        std::cout << "\n--- Menu MPointer ---\n";
        std::cout << "1. Crear nuevo MPointer\n";
        std::cout << "2. Asignar valor a MPointer\n";
        std::cout << "3. Obtener valor de MPointer\n";
        std::cout << "4. Copiar MPointer\n";
        std::cout << "5. Volver al menu principal\n";
        std::cout << "Seleccione una opcion: ";

        std::getline(std::cin, option);

        if (option == "5") break;

        try {
            if (option == "1") {
                std::cout << "Ingrese tamano y tipo: ";
                std::string input;
                std::getline(std::cin, input);

                std::istringstream iss(input);
                size_t size;
                std::string type;

                if (iss >> size >> type) {
                    currentPtr = MPointer<int>::New(size, type);
                    std::cout << "MPointer creado." << std::endl;
                }
                else {
                    std::cout << "Entrada invalida. Intente de nuevo.\n";
                }
            }
            else if (option == "2") {
                if (currentPtr.getId() == 0) {
                    std::cout << "Primero debe crear un Mpointer\n";
                }
                else {
                    std::cout << "Ingrese un valor: ";
                    int value;
                    std::cin >> value;
                    std::cin.ignore();
                    currentPtr = value;
                    std::cout << "Valor asignado correctamente.\n";
                }
            }
            else if (option == "3") {
                if (currentPtr.getId() == 0) {
                    std::cout << "Primero debe crear un Mpointer\n";
                }
                else {
                    int value = *currentPtr;
                    std::cout << "Valor: " << value << std::endl;
                }
            }
            else if (option == "4") {
                if (currentPtr.getId() == 0) {
                    std::cout << "Primero debe crear un Mpointer\n";
                }
                else {
                    MPointer<int> newPtr = currentPtr;
                    std::cout << "MPointer copiado.\n";
                }
            }
            else {
                std::cout << "Opcion no valida. Intente nuevamente.\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

void mpointerMenu1() {
    std::string option;
    while (true) {
        std::cout << "\n--- Menu MPointer ---\n";
        std::cout << "1. Crear nuevo MPointer\n";
        std::cout << "2. Volver al menu principal\n";
        std::cout << "Seleccione una opcion: ";

        std::getline(std::cin, option);

        if (option == "2") break;

        try {
            if (option == "1") {
                std::cout << "Ingrese tamano y tipo (ej: 4 int, 8 double, 16 string): ";
                std::string input;
                std::getline(std::cin, input);

                std::istringstream iss(input);
                size_t size;
                std::string type;

                if (iss >> size >> type) {
                    if (type == "int") {
                        MPointer<int> intPtr = MPointer<int>::New(size, type);
                    }
                    else if (type == "double") {
                        MPointer<double> doublePtr = MPointer<double>::New(size, type);
                    }
                    else if (type == "string") {
                        MPointer<std::string> stringPtr = MPointer<std::string>::New(size, type);
                    }
                    else {
                        std::cout << "Tipo no valido.\n";
                    }
                    std::cout << "MPointer creado.\n";
                }
                else {
                    std::cout << "Entrada invalida. Intente de nuevo.\n";
                }
            }
            else {
                std::cout << "Opcion no valida. Intente nuevamente.\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
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
    serverAddr.sin_port = htons(27015);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Conectarse al servidor
    iResult = connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Error al conectar con el servidor: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Conectado al servidor!" << std::endl;

    // Inicializar MPointer
    MPointer<int>::Init(ConnectSocket); 

    std::string mainOption;
    while (true) {
        std::cout << "\n--- Menu Principal ---\n";
        std::cout << "1. Comandos basicos (CREATE, SET, GET, etc.)\n";
        std::cout << "2. Menu MPointer\n";
        std::cout << "3. Salir\n";
        std::cout << "Seleccione una opcion: ";

        std::getline(std::cin, mainOption);

        if (mainOption == "1") {
            basicCommandsMenu(ConnectSocket);
        }
        else if (mainOption == "2") {
            mpointerMenu();
        }
        else if (mainOption == "3") {
            break;
        }
        else {
            std::cout << "Opcion no valida. Intente nuevamente.\n";
        }
    }

    // Cerrar el socket y limpiar
    closesocket(ConnectSocket);
    MPointer<int>::Close();
    WSACleanup();

    return 0;
}
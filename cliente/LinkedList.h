#include "MPointer.h"
#include <iostream>

template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        MPointer<Node> next;  // Se inicializará correctamente

        Node(const T& val) : data(val), next(nullptr) {}  // Ahora acepta nullptr
    };

    MPointer<Node> head;

public:
    LinkedList() : head(nullptr) {}

    void append(const T& value) {
        MPointer<Node> newNode = MPointer<Node>::New();
        *newNode = Node(value);

        if (head.getId() == 0) {  
            head = newNode;
        }
        else {
            MPointer<Node> current = head;
            while ((*current).next.getId() != 0) {  
                current = (*current).next;
            }
            (*current).next = newNode;
        }
    }

    void print() {
        MPointer<Node> current = head;
        while (current.getId() != 0) { 
            std::cout << (*current).data << " ";
            current = (*current).next;
        }
        std::cout << std::endl;
    }

    
    ~LinkedList() {
        MPointer<Node> current = head;
        while (current.getId() != 0) {
            MPointer<Node> next = (*current).next;
            current = next;  
        }
    }
};
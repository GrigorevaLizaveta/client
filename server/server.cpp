#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

const int BUFFER_SIZE = 1024;

void save_file(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Error creating/opening file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    file.write(content.c_str(), content.size());
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[1]);

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "[!] Error creating server socket." << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    // Bind the socket to the specified address and port
    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "[!] Error binding server socket." << std::endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        std::cerr << "[!] Error listening for incoming connections." << std::endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << server_port << std::endl;

    while (true) {
        // Accept a connection
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == -1) {
            std::cerr << "[!] Error accepting connection." << std::endl;
            continue;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Receive data from the client
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            std::cerr << "[!] Error receiving data from client." << std::endl;
            close(client_socket);
            continue;
        }

        // Extract filename and content from the received data
        std::string received_data(buffer, bytes_received);
        size_t pos = received_data.find('|');

        if (pos != std::string::npos) {
            std::string filename = received_data.substr(0, pos);
            std::string content = received_data.substr(pos + 1);

            // Save the received file
            save_file(filename, content);

            std::cout << "File received and saved: " << filename << std::endl;
        }
        else {
            std::cerr << "[!] Invalid data format received." << std::endl;
        }

        // Close the client socket
        close(client_socket);
    }

    // Close the server socket (this part will never be reached in this example)
    close(server_socket);

    return 0;
}
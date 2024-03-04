#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

void send_file(const std::string& file_path, const std::string& server_address, int server_port) {
    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Error opening file: " << file_path << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    file.close();

    size_t pos = file_path.find_last_of('/');
    std::string filename = (pos != std::string::npos) ? file_path.substr(pos + 1) : file_path;

    std::string data = filename + "|" + content;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "[!] Error creating client socket." << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_address_struct{};
    server_address_struct.sin_family = AF_INET;
    server_address_struct.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_address.c_str(), &(server_address_struct.sin_addr)) <= 0) {
        std::cerr << "[!] Invalid server address: " << server_address << std::endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_address_struct), sizeof(server_address_struct)) == -1) {
        std::cerr << "[!] Error connecting to server." << std::endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (send(client_socket, data.c_str(), data.size(), 0) == -1) {
        std::cerr << "[!] Error sending data to server." << std::endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    close(client_socket);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <file_path> <server_ip> <server_port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string file_path = argv[1];
    std::string server_address = argv[2];
    int server_port = atoi(argv[3]);

    send_file(file_path, server_address, server_port);

    return 0;
}
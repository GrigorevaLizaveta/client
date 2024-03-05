#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <mutex>  
#include <atomic> 

constexpr int MAX_THREADS = 100;
constexpr int MAX_BUFFER_SIZE = 1024;

std::mutex file_mutex;
std::vector<std::thread> thread_pool;
std::atomic_bool is_running(true);
int server_socket;

void handle_client(int client_socket, const std::string& save_path) {
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0) {
        std::cerr << "[!] Error receiving data from client." << std::endl;
        close(client_socket);
        return;
    }

    std::string received_data(buffer, bytes_received);
    size_t split_pos = received_data.find('|');
    std::string filename = received_data.substr(0, split_pos);
    std::string content = received_data.substr(split_pos + 1);

    std::string file_path = save_path + "/" + filename;

    {
        std::lock_guard<std::mutex> lock(file_mutex);

        std::ofstream file(file_path, std::ios::out | std::ios::binary);
        file.write(content.c_str(), content.size());
    }

    close(client_socket);
}

void signal_handler(int signum) {
    std::cout << "[*] Shutting down the server..." << std::endl;
    is_running.store(false);

    for (auto& thread : thread_pool) {
        thread.join();
    }

    close(server_socket);
    exit(EXIT_SUCCESS);
}

void server_thread(int port, int max_threads, const std::string& save_path) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "[!] Error creating server socket." << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "[!] Error binding server socket." << std::endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        std::cerr << "[!] Error listening on server socket." << std::endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    std::cout << "[*] Listening on 0.0.0.0:" << port << std::endl;

    auto sig_handler = signal_handler;
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    while (is_running.load()) {
        int client_socket = accept(server_socket, nullptr, nullptr);

        if (client_socket == -1) {
            std::cerr << "[!] Error accepting client connection." << std::endl;
            continue;
        }

        if (thread_pool.size() < static_cast<size_t>(max_threads) && is_running.load()) {
            thread_pool.emplace_back(handle_client, client_socket, save_path);
        }
        else {
            std::cerr << "[!] Maximum number of threads reached. Connection refused." << std::endl;
            close(client_socket);
        }
    }

    for (auto& thread : thread_pool) {
        thread.join();
    }
}

int main(int argc, char* argv[]) {
    int port = 12346;
    int max_threads = MAX_THREADS;
    std::string save_path = "./received_files";

    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    if (argc >= 3) {
        max_threads = atoi(argv[2]);
    }

    if (argc >= 4) {
        save_path = argv[3];
    }

    if (access(save_path.c_str(), F_OK) != 0) {
        if (mkdir(save_path.c_str(), 0777) == -1) {
            std::cerr << "[!] Error creating directory: " << save_path << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    server_thread(port, max_threads, save_path);

    return 0;
}
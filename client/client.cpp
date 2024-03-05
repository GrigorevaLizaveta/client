#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

// Отправка файла на сервер (& - оператор, возвращающий адрес операнда в памяти)
void send_file_to_server(const string& file_path, const string& server_address, int server_port) {
    ifstream file(file_path, ios::in | ios::binary); //ios - базовый класс для форматированного ввода и вывода данных.
   //Проверка открытия файла
    if (!file.is_open()) {
        cerr << "[!] Error opening file: " << file_path << endl;
        exit(EXIT_FAILURE);
    }
    //Чтение файла в строку content 
    string content((istreambuf_iterator<char>(file)), (istreambuf_iterator<char>())); // istreatbuf... - объект входного итератора
    file.close();

    //Извлечение имени файла 
    size_t pos = file_path.find_last_of('/');
    //Проверка есть ли '/' и вычленение пути к файлу
    string filename = (pos != string::npos) ? file_path.substr(pos + 1) : file_path;

    string data = filename + "|" + content;

    //Создание сокета для клиента (AF_INET - семейства адресов IPv4, SOCK_STREAM - создание потокового сокета (протокол TCP), протокол по умолчанию)
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "[!] Error creating client socket." << endl;
        exit(EXIT_FAILURE);
    }

    //структура sockaddr_in для представления адреса сервера
    sockaddr_in server_address_struct{};
    server_address_struct.sin_family = AF_INET;//Устанавливает семейство адресов для IPv4.
    server_address_struct.sin_port = htons(server_port);  //Установка порта и конвертация
   
    if (inet_pton(AF_INET, server_address.c_str(), &(server_address_struct.sin_addr)) <= 0) {
        cerr << "[!] Invalid server address: " << server_address << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    //соединение с сервером
    if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_address_struct), sizeof(server_address_struct)) == -1) {
        cerr << "[!] Error connecting to server." << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    //Отправка данных на сервер
    if (send(client_socket, data.c_str(), data.size(), 0) == -1) {
        cerr << "[!] Error sending data to server." << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    close(client_socket);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <file_path> <server_ip> <server_port>" << endl;
        exit(EXIT_FAILURE);
    }

    string file_path = argv[1];
    string server_address = argv[2];
    int server_port = atoi(argv[3]);

    send_file_to_server(file_path, server_address, server_port);

    return 0;
}

#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

const int BUFFER_SIZE = 1024;

//сохранение файла на сервере  (& - оператор, возвращающий адрес операнда в памяти)
void save_file_on_server(const string& filename, const string& content) {
    ofstream file(filename, ios::out | ios::binary);//ios - базовый класс для форматированного ввода и вывода данных.
    //Проверка на открытие файла
    if (!file.is_open()) {
        cerr << "[!] Error creating/opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }
    //запись в файл content
    file.write(content.c_str(), content.size());
    file.close();
}

//создание серверного сокета, привязка к порту и адресу, ожидание входящих соединений и обработка
//*-указатель на первый символ массива
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <server_port>" << endl;
        exit(EXIT_FAILURE); //Принудительное завершение
    }

    int server_port = atoi(argv[1]); //atoi - конвертация строки в числовой вид

    //Создание сокета  (AF_INET - семейства адресов IPv4, SOCK_STREAM - создание потокового сокета (протокол TCP), протокол по умолчанию)
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr << "[!] Error creating server socket." << endl;
        exit(EXIT_FAILURE);
    }

    //структура sockaddr_in для представления адреса сервера
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET; //Устанавливает семейство адресов для IPv4.
    server_address.sin_addr.s_addr = INADDR_ANY; // Указывает, что сервер должен слушать все доступные сетевые интерфейсы
    server_address.sin_port = htons(server_port); //Установка порта и конвертация

    // bind - привязка сокета к конкретному адресу и порту.
    //Приводит указатель к структуре sockaddr*, которая используется в bind.
    //(bind хочет указатель на структуру sockaddr, но server_address является sockaddr_in)
    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        cerr << "[!] Error binding server socket." << endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    //  Установка сокета в режим прослушивания входящих соединений (макс 5 в очереди)
    if (listen(server_socket, 5) == -1) {
        cerr << "[!] Error listening for incoming connections." << endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    cout << "Server listening on port " << server_port << endl;


    //Прием и обработка соединений
    while (true) {
        // Ожидание соединения
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == -1) {
            cerr << "[!] Error accepting connection." << endl;
            continue;
        }

        //создание буфера
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Получение данных от клиента
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            cerr << "[!] Error receiving data from client." << endl;
            close(client_socket);
            continue;
        }

        // Обработка полученных данных
        string received_data(buffer, bytes_received); //буфер в строку
        size_t pos = received_data.find('|');

        if (pos != std::string::npos) {
            std::string filename = received_data.substr(0, pos);
            std::string content = received_data.substr(pos + 1);

            // сохранение данных в файл
            save_file_on_server(filename, content);

            cout << "File received and saved: " << filename << endl;
        }
        else {
            cerr << "[!] Invalid data format received." << endl;
        }

        // Закрытие сокета
        close(client_socket);
    }

    close(server_socket);

    return 0;
}
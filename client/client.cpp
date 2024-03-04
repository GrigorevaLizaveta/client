#include <iostream>      // Ввод-вывод
#include <cstdlib>       // Преобразование строки в число
#include <cstring>       // Работа со строками
#include <cerrno>        // Обработка ошибок через errno
#include <unistd.h>      // Закрытие файловых дескрипторов
#include <sys/types.h>   // Типы данных
#include <sys/socket.h>  // Системные вызовы для работы с сокетами
#include <netinet/in.h>  // Структуры для представления сетевых адресов
#include <arpa/inet.h>   // Преобразование IP-адресов

constexpr int MAX_BUFFER_SIZE = 1024;  // Максимальный размер буфера для чтения файла

int main(int argc, char* argv[]) {
    // Проверяем количество аргументов командной строки
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_address> <port> <file_path>\n";
        return EXIT_FAILURE;
    }

    // Получаем аргументы командной строки: адрес сервера, порт, путь к файлу
    const char* serverAddress = argv[1];
    int port = atoi(argv[2]);
    const char* filePath = argv[3];

    // Создаем сокет клиента
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error in socket");  // Выводим сообщение об ошибке
        return EXIT_FAILURE;
    }

    // Заполняем структуру для адреса сервера
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverAddress, &serverAddr.sin_addr);

    // Устанавливаем соединение с сервером
    if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("Error in connect");  // Выводим сообщение об ошибке
        return EXIT_FAILURE;
    }

    // Открываем файл на чтение
    FILE* file = fopen(filePath, "r");
    if (file == nullptr) {
        perror("Error opening file");  // Выводим сообщение об ошибке
        close(clientSocket);
        return EXIT_FAILURE;
    }

    // Буфер для чтения данных из файла
    char buffer[MAX_BUFFER_SIZE];
    size_t bytesRead;

    // Читаем данные из файла и отправляем их по сокету
    while ((bytesRead = fread(buffer, 1, MAX_BUFFER_SIZE, file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) < 0) {
            perror("Error in send");  // Выводим сообщение об ошибке
            fclose(file);
            close(clientSocket);
            return EXIT_FAILURE;
        }
    }

    // Закрываем файл и сокет
    fclose(file);
    close(clientSocket);
    return EXIT_SUCCESS;
}
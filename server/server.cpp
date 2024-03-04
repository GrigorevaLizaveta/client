#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

constexpr int MAX_THREADS = 10; // Максимальное количество потоков
constexpr int MAX_BUFFER_SIZE = 1024; // Максимальный размер буфера для данных

// Структура для передачи аргументов в функцию потока
struct ThreadArgs {
    int clientSocket; // Сокет клиента
    const char* savePath; // Путь для сохранения данных
};

// Функция обработки клиента в отдельном потоке
void* handleClient(void* arg) {
    ThreadArgs* threadArgs = reinterpret_cast<ThreadArgs*>(arg); // Преобразование аргументов

    char buffer[MAX_BUFFER_SIZE]; // Буфер для данных
    ssize_t bytesReceived; // Количество принятых байт

    // Прием данных от клиента
    bytesReceived = recv(threadArgs->clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    if (bytesReceived < 0) { // Проверка на ошибку приема
        perror("Error in recv");
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }

    // Сохранение принятых данных в файл
    FILE* file = fopen(threadArgs->savePath, "w");
    if (file == nullptr) { // Проверка на ошибку открытия файла
        perror("Error opening file");
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }
    size_t bytesWritten = fwrite(buffer, 1, bytesReceived, file); // Запись данных в файл
    if (bytesWritten != static_cast<size_t>(bytesReceived)) { // Проверка на ошибку записи
        perror("Error writing to file");
        fclose(file);
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }
    fclose(file); // Закрытие файла

    // Вывод сообщения об успешной записи файла
    if (bytesWritten > 0) {
        std::cout << "File successfully written: " << threadArgs->savePath << std::endl;
    }
    else {
        std::cout << "Failed to write file: " << threadArgs->savePath << std::endl;
    }

    close(threadArgs->clientSocket); // Закрытие сокета клиента
    delete threadArgs; // Освобождение памяти для аргументов
    pthread_exit(NULL); // Выход из потока
}

int main(int argc, char* argv[]) {
    // Проверка на количество аргументов командной строки
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <port> <save_path> <max_threads>\n";
        return EXIT_FAILURE;
    }

    // Получение параметров из аргументов командной строки
    int port = atoi(argv[1]); // Порт
    const char* savePath = argv[2]; // Путь для сохранения данных
    int maxThreads = atoi(argv[3]); // Максимальное количество потоков

    int serverSocket, clientSocket; // Серверный и клиентский сокеты
    struct sockaddr_in serverAddr, clientAddr; // Структуры адресов сервера и клиента
    socklen_t clientLen = sizeof(clientAddr); // Размер структуры клиента

    // Создание сокета сервера
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) { // Проверка на ошибку создания сокета
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // Настройка адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Привязка сокета к адресу
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("Error binding socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    // Прослушивание сокета
    listen(serverSocket, 5);

    pthread_t threads[MAX_THREADS]; // Массив потоков
    int threadCount = 0; // Счетчик потоков

    // Бесконечный цикл принятия клиентов
    while (true) {
        // Принятие соединения от клиента
        clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
        // Проверка на ошибку принятия соединения
        if (clientSocket < 0) {
            perror("Error accepting connection");
            continue; // Продолжение цикла для ожидания следующего соединения
        }

        // Создание структуры аргументов для потока
        ThreadArgs* args = new ThreadArgs;
        args->clientSocket = clientSocket; // Передача клиентского сокета
        args->savePath = savePath; // Передача пути для сохранения данных

        // Создание нового потока для обработки клиента
        pthread_create(&threads[threadCount++ % maxThreads], NULL, handleClient, args);
    }

    close(serverSocket); // Закрытие серверного сокета
    return EXIT_SUCCESS; // Выход с успехом
}
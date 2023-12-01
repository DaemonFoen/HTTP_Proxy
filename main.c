#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 8192

void* handle_client(void* client_socket_raw) {
    int client_socket = (int)client_socket_raw;

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        perror("Ошибка в запросе клиента");
        close(client_socket);
        pthread_exit(NULL);
    }

    char* host_start = strstr(buffer, "Host: ");
    if (!host_start) {
        close(client_socket);
        pthread_exit(NULL);
    }
    host_start += 6;
    char* host_end = strchr(host_start, '\r');
    if (!host_end) {
        close(client_socket);
        pthread_exit(NULL);
    }
    *host_end = '\0';

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Ошибка при создании сокета сервера");
        close(client_socket);
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    struct hostent* server_host = gethostbyname(host_start);
    if (!server_host) {
        close(client_socket);
        close(server_socket);
        pthread_exit(NULL);
    }
    memcpy(&server_addr.sin_addr, server_host->h_addr, server_host->h_length);

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка при подключении к серверу");
        close(client_socket);
        close(server_socket);
        pthread_exit(NULL);
    }

    if (send(server_socket, buffer, bytes_received, 0) < 0) {
        perror("Ошибка при отправке данных на сервер");
        close(client_socket);
        close(server_socket);
        pthread_exit(NULL);
    }

    while ((bytes_received = recv(server_socket, buffer, sizeof(buffer), 0)) > 0) {
        if (send(client_socket, buffer, bytes_received, 0) < 0) {
            perror("Ошибка при отправке данных клиенту");
            break;
        }
    }

    close(client_socket);
    close(server_socket);

    pthread_exit(NULL);
}


void sigpipe_handler(int signo) {
    printf("broken pipe");
}

int main() {
    signal(SIGPIPE, sigpipe_handler);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Ошибка при создании сокета сервера");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка при привязке сокета сервера к адресу и порту");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Ошибка при прослушивании подключений");
        exit(EXIT_FAILURE);
    }

    printf("Заработал");
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket;
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Ошибка при принятии подключения");
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *) client_socket) != 0) {
            perror("Ошибка при создании потока");
            close(client_socket);
        } else {
            pthread_detach(tid);
        }
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Servidor usando: %s <puerto>\n", argv[0]);
        return 1;
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Crear el socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error al crear el socket");
        return 1;
    }

    // Configurar la dirección del servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(atoi(argv[1]));

    // Vincular el socket a la dirección del servidor
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error al vincular el socket");
        return 1;
    }

    // Escuchar por conexiones entrantes
    if (listen(serverSocket, 1) == -1) {
        perror("Error al escuchar por conexiones entrantes");
        return 1;
    }

    printf("Esperando conexiones entrantes...\n");

    // Aceptar una conexión entrante
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (clientSocket == -1) {
        perror("Error al aceptar la conexión");
        return 1;
    }

    printf("Cliente conectado\n");

    // Crear un pipe  para la comunicación entre el servidor y el comando ejecutado
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Error al crear la tubería");
        return 1;
    }

    // Crear un proceso hijo para ejecutar el comando
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error al crear el proceso hijo");
        return 1;
    }

    if (pid == 0) {
        // Cerrar el extremo de lectura de la tubería en el proceso hijo
        close(pipefd[0]);

        // Redirigir la salida estándar hacia el extremo de escritura de la tubería
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Leer el comando del cliente
        char command[BUFFER_SIZE];
        ssize_t bytesRead = recv(clientSocket, command, sizeof(command), 0);
        if (bytesRead == -1) {
            perror("Error al recibir el comando");
            return 1;
        }

        // Ejecutar el comando en el sistema local
        if (execl("/bin/bash", "bash", "-c", command, NULL) == -1) {
            perror("Error al ejecutar el comando");
            return 1;
        }

        exit(0);
    } else {
        // Cerrar el extremo de escritura de la tubería en el proceso padre
        close(pipefd[1]);

        // Leer la salida del comando desde el extremo de lectura de la tubería
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            // Enviar la salida al cliente
            ssize_t bytesSent = send(clientSocket, buffer, bytesRead, 0);
            if (bytesSent == -1) {
                perror("Error al enviar la salida");
                return 1;
            }
        }

        // Cerrar el extremo de lectura de la tubería en el proceso padre
        close(pipefd[0]);

        // Esperar a que el proceso hijo termine
        int status;
        waitpid(pid, &status, 0);
    }

    // Cerrar los descriptores de archivo
    close(clientSocket);
    close(serverSocket);

    return 0;
}

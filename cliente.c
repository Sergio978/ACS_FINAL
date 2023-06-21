#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("El uso correcto es: %s <dirección IP> <puerto>\n", argv[0]);
        return 1;
    }

    int clientSocket;
    struct sockaddr_in serverAddress;

    // Crear el socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error al crear el socket");
        return 1;
    }

    // Configurar la dirección del servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(atoi(argv[2]));

    // Conectar con el servidor
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error al conectar con el servidor");
        return 1;
    }

    // Leer el comando a ejecutar
    char command[BUFFER_SIZE];
    printf("Ingrese el comando a ejecutar (o bien salir/exit para desconectar): ");
    fgets(command, sizeof(command), stdin);

    // Enviar el comando al servidor
    ssize_t bytesSent = send(clientSocket, command, strlen(command), 0);
    if (bytesSent == -1) {
        perror("Error al enviar el comando");
        return 1;
    }

    // Verificar si el comando es "salir" o "exit"
    if (strcmp(command, "salir\n") == 0 || strcmp(command, "exit\n") == 0) {
        printf("Desconectando del servidor...\n");
        close(clientSocket);
        return 0;
    }

    // Recibir y mostrar la salida del servidor
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        printf("%.*s", (int)bytesRead, buffer);
    }

    if (bytesRead == -1) {
        perror("Error al recibir la salida");
        return 1;
    }

    // Cerrar el descriptor de archivo
    close(clientSocket);

    return 0;
}

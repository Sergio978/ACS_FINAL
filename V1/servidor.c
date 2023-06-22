#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#define LENGTH 20000

int main(int argc, char *argv[])
{
  int numbytes;
  char buf[100];

  int server_fd, cliente_fd;

  struct sockaddr_in servidor;
  struct sockaddr_in cliente;

  int sin_size_servidor;
  int sin_size_cliente;

  // Creación del socket del servidor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  // Configuración del socket para permitir reutilizar el puerto inmediatamente después de cerrar el servidor
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
  {
    perror("Server-setsockopt() error!");
    exit(1);
  }
  else
    printf("Server-setsockopt is OK...\n");

  // Configuración de los parámetros del servidor
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons( atoi(argv[1]) );  // Convertimos el número de puerto a network byte order
  servidor.sin_addr.s_addr = INADDR_ANY;  // El servidor escuchará en todas las interfaces de red
  memset(&(servidor.sin_zero), '\0', 8);  // Poner a cero el resto de la estructura

  sin_size_servidor = sizeof( servidor );

  // Asociamos el socket del servidor con el puerto
  if (bind(server_fd, (struct sockaddr *)&servidor, sin_size_servidor) == -1)
  {
    perror("bind");
    exit(1);
  }

  // Esperamos por conexiones de clientes
  if (listen(server_fd, 1) == -1)
  {
    perror("listen");
    exit(1);
  }

  sin_size_cliente = sizeof( cliente );

  // Aceptamos una conexión de cliente
  if ((cliente_fd = accept(server_fd, (struct sockaddr *)&cliente, &sin_size_cliente)) == -1)
  {
    perror("accept");
    exit(1);
  }
  printf("server: conexion cliente desde %s\n", inet_ntoa(cliente.sin_addr));

  while(1){
    // Recibimos el comando del cliente
    if ((numbytes=recv(cliente_fd, buf, 100-1, 0)) == -1) {
      perror("recv");
      exit(1);
    }

    buf[numbytes] = '\0';

    if (strcmp(buf,"exit") == 0)
      break;

    

    // Creamos un proceso hijo para ejecutar el comando
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed");
      exit(1);
    }
    if (pid == 0) {  // Proceso hijo
      // Abrimos el archivo "a.txt"
      int fd = open("a.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (fd == -1) {
        perror("open failed");
        exit(1);
      }
      // Redirigimos la salida estándar al archivo
      dup2(fd, STDOUT_FILENO);
      close(fd);
      // Ejecutamos el comando
      char* args[4] = {"/bin/sh", "-c", buf,NULL};
      if (execvp(args[0], args) < 0) {
          // Comando no válido
          perror("exec failed");
          exit(1);
      }
    }
    // Proceso padre espera al hijo para asegurarse de que el archivo "a.txt" esté completo
    waitpid(pid, NULL, 0);

    // ...
    // Leemos el archivo a.txt y lo enviamos al cliente
    char* fs_name = "a.txt";
    char sdbuf[LENGTH];
    printf("[Server] Enviando salida al Cliente...\n");
    FILE* fs = fopen(fs_name, "r");
    if (fs == NULL)
    {
      printf("ERROR: File %s not found on server.\n", fs_name);
      exit(1);
    }

    bzero(sdbuf, LENGTH);
    int fs_block_sz;
    int file_empty = 1;  // Bandera para indicar si el archivo está vacío
    while ((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
    {
      file_empty = 0;  // Actualizamos la bandera si se encuentra contenido en el archivo
      if (send(cliente_fd, sdbuf, fs_block_sz, 0) < 0)
      {
        printf("ERROR: al enviar la salida del comando al cliente\n");
        exit(1);
      }
      bzero(sdbuf, LENGTH);
    }
    fclose(fs);

    if (file_empty)
    {
      // Si el archivo está vacío, enviamos un mensaje especial al cliente
      char empty_message[] = "Salida de comando no encontrada";
      if (send(cliente_fd, empty_message, strlen(empty_message), 0) < 0)
      {
        printf("ERROR: al enviar el mensaje de salida vacía al cliente\n");
        exit(1);
      }
    }
    else
    {
      printf("Ok sent to client!\n");
    }
    // ...


    // Eliminamos el archivo a.txt
    if (remove(fs_name) == -1) {
      perror("remove");
    }
  }

  close(cliente_fd);

  close(server_fd);
  shutdown(server_fd, SHUT_RDWR);

  exit(0);

  return 0;
}

#include "headerFiles.h"

#define path "/home/wanakin/Projetos/C-C++/http-server"
#define port_number 8080
#define bf_size 1024
#define connection_n 10

int thread_count = 0;
sem_t mutex;

int main(int argc, char *argv[])
{
    char buffer[256]; // Buffer de dados
    char *ptr = buffer;
    int maxLen = sizeof(buffer);
    int len = 0; // numero de bytes a serem recebidos
    int n = 0;   // numero de bytes em cada recv

    sem_init(&mutex, 0, 1); // inicializa o semaforo
    int socket_desc, new_socket, tamanho, *new_sock;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0); // cria o socket // AFINET = endereco local // SOCK_STREAM = tipo de socket para tcp

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_number);

    bind(socket_desc, (struct sockaddr *)&server, sizeof(server)); // binda o servidor a um port especifico (deve ser conhecido pelo cliente)

    listen(socket_desc, 20); // deixa o servidor escutando (pronto para aceitar conexoes)

    puts("Esperando por conexoes...");
    tamanho = sizeof(struct sockaddr_in);
    for (;;){
        // Aceita conexões do cliente
        if (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&tamanho) > 0)
        {
            perror("Error: accepting failed!");
            exit(1);
        }
        // Seção de transferência de dados
        while ((n = recv(new_socket, ptr, maxLen, 0)) > 0)
        {
            ptr += n;
            maxLen -= n;
            len += n;
        }
        send(new_socket, buffer, len, 0);
        // Fecha o socket
        close(new_socket);
    }

    return 0;
}
#include "headerFiles.h"

#define PATH "/home/wanakin/Projetos/C-C++/http-server"                // caminho do repositorio
#define PORT_NO 8080 // numero da porta
#define BUFFER_SIZE 1024 // tamanho do buffer
#define CONNECTION_NUMBER 10 // numero de conexoes simultaneas

int thread_count = 0; // faz a contagem das threads simultaneas ativas no momento
sem_t mutex; // semaforo pra controlar a condicao de corrida

void html_handler(int socket, char *file_name) // handler de arquivos html
{
    char *buffer; 
    char *full_path = (char *)malloc((strlen(PATH) + strlen(file_name)) * sizeof(char)); // alocacao do caminho completo
    FILE *fp; // handler de arquivos

    strcpy(full_path, PATH); // joga o caminho do diretorio raiz dentro do full path
    strcat(full_path, file_name); // concatena o caminho com o nome do arquivo

    fp = fopen(full_path, "r"); // abre o arquivo modo leitura
    if (fp != NULL) // achou o arquivo
    {
        puts("achei o arquivo.");

        fseek(fp, 0, SEEK_END); // encontra o tamanho do arquivo
        long bytes_read = ftell(fp); // salvando o tamanho do arquivo
        fseek(fp, 0, SEEK_SET);
        send(socket, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n", 44, 0); // manda o header com mensagem de sucesso
        buffer = (char *)malloc(bytes_read * sizeof(char));  // aloca o numero de bytes lidos e guarda no buffer
        fread(buffer, bytes_read, 1, fp);
        write (socket, buffer, bytes_read); // envia o conteudo do html pro cliente
        free(buffer);
        fclose(fp);
    }
    else // caso nao for encontrado o arquivo
    {
        write(socket, "HTTP/1.0 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>404 File Not Found</body></html>", strlen("HTTP/1.0 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>404 File Not Found</body></html>"));
    }

    free(full_path); // libera a memoria alocada pro caminho completo
}

void *connection_handler(void *socket_desc) // recebe o endereco do socket
{
    int request;
    char client_reply[BUFFER_SIZE], *request_lines[3];
    char *file_name;
    char *extension;

    // recebe o socket descritor
    int sock = *((int *)socket_desc);

    // recebe a request
    request = recv(sock, client_reply, BUFFER_SIZE, 0);

    sem_wait(&mutex);
    thread_count++; 

    if(thread_count > 10) // mantem dentro do limite de threads, caso o limite seja ultrapassado entra aqui
    {
        char *message = "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>System is busy right now.</body></html>";
        write(sock, message, strlen(message));
        thread_count--; 
        sem_post(&mutex);
        free(socket_desc);
        shutdown(sock, SHUT_RDWR);
        close(sock);
        sock = -1;
        pthread_exit(NULL);
    }
    sem_post(&mutex);

    if (request < 0) // erro na funcao recv
    {
        puts("erro no recv");
    }
    else if (request == 0) // socket fechado. cliente desconectado
    {
        puts("cliente desconectado");
    }
    else // recv deu bom
    {
        printf("%s", client_reply);
        request_lines[0] = strtok(client_reply, " \t\n");
        if (strncmp(request_lines[0], "GET\0", 4) == 0)
        {
            // analisa o cabecalho do request
            request_lines[1] = strtok(NULL, " \t");
            request_lines[2] = strtok(NULL, " \t\n");

            if (strncmp(request_lines[2], "HTTP/1.0", 8) != 0) // da ruim se nao for http/1.0
            {
                char *message = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>400 Bad Request</body></html>";
                write(sock, message, strlen(message));
            }
            else
            {
                char *tokens[2]; // para fazer a analise do arquivo e do tipo

                file_name = (char *)malloc(strlen(request_lines[1]) * sizeof(char));
                strcpy(file_name, request_lines[1]);
                puts(file_name);

                // pega o nome do arquivo e o tipo
                tokens[0] = strtok(file_name, ".");
                tokens[1] = strtok(NULL, "."); 

                if (tokens[0] == NULL || tokens[1] == NULL) // se nao existe uma extensao no arquivo
                {
                    char *message = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n<!doctype html><html><body>400 Bad Request. (You need to request html files)</body></html>";
                    write(sock, message, strlen(message));
                }
                else
                {

                    if (strcmp(tokens[1], "html") != 0) 
                    {
                        char *message = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n<!doctype html><html><body>400 Bad Request. Not Supported File Type (Suppoerted File Types: html and jpeg)</body></html>";
                        write(sock, message, strlen(message));
                    }
                    else
                    {
                        if (strcmp(tokens[1], "html") == 0) // chama o handler de html
                        {
                            sem_wait(&mutex); // cria uma condicao de corrida
                            html_handler(sock, request_lines[1]);
                            sem_post(&mutex);
                        }
                    }

                }
                free(file_name);
            }
        }
    }
    
    free(socket_desc); // libera memoria, desliga a thread e diminui o contador de threads.
    shutdown(sock, SHUT_RDWR);
    close(sock);
    sock = -1;
    sem_wait(&mutex);
    thread_count--;
    sem_post(&mutex);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // inicializando os semaforos
    sem_init(&mutex, 0, 1); 
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    /* if (socket_desc == -1)
    {
        puts("deu erro ao criar o socket leozinho");
        return 1;
    }*/

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT_NO);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) // faz o bind
    {
        puts("falhou o bind");
        return 1;
    }

    listen(socket_desc, 20); //bota o socket para ouvir

    puts("esperando as conexao");
    c = sizeof(struct sockaddr_in);
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))) // espera ate aceitar uma conexao
    {
        puts("conexao aceita\n");

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *)new_sock) < 0) // a cada request, cria uma thread
        {
            puts("nao deu pra cria a thread"); // erro ao criar a thread
            return 1;
        }   
    }

    return 0;
}
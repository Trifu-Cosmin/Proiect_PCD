/*
  Descriere:
  Acest fisier implementeaza clientul normal al aplicatiei.

  Clientul:
  - primeste ca argument calea unui fisier sursa
  - se conecteaza la serverul TCP
  - trimite fisierul catre server
  - primeste rezultatul analizei
  - afiseaza raspunsul in consola
*/

#include <arpa/inet.h>  // Pentru inet_addr
#include <netinet/in.h> // Pentru sockaddr_in
#include <stdio.h>      // Pentru printf, fprintf, fopen, fread
#include <stdlib.h>     // Pentru malloc, free
#include <string.h>     // Pentru strlen, strrchr, memset, sscanf
#include <sys/socket.h> // Pentru socket, connect, send, recv
#include <unistd.h>     // Pentru close

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_LINE 1024

/*
  Trimite tot bufferul prin socket.
*/
static int send_all(int sockfd, const void *buffer, size_t length)
{
    const char *data = (const char *)buffer;
    size_t total_sent = 0;

    while (total_sent < length)
    {
        ssize_t sent_now = send(sockfd, data + total_sent, length - total_sent, 0);

        if (sent_now <= 0)
        {
            return -1;
        }

        total_sent += (size_t)sent_now;
    }

    return 0;
}

/*
  Primeste exact length bytes din socket.
*/
static int recv_all(int sockfd, void *buffer, size_t length)
{
    char *data = (char *)buffer;
    size_t total_received = 0;

    while (total_received < length)
    {
        ssize_t received_now = recv(sockfd, data + total_received, length - total_received, 0);

        if (received_now <= 0)
        {
            return -1;
        }

        total_received += (size_t)received_now;
    }

    return 0;
}

/*
  Citeste o linie din socket pana la '\n'.
*/
static int recv_line(int sockfd, char *buffer, size_t max_len)
{
    size_t index = 0;

    if (max_len == 0U)
    {
        return -1;
    }

    while (index < max_len - 1U)
    {
        char ch = '\0';
        ssize_t received_now = recv(sockfd, &ch, 1, 0);

        if (received_now <= 0)
        {
            return -1;
        }

        if (ch == '\n')
        {
            break;
        }

        buffer[index] = ch;
        index++;
    }

    buffer[index] = '\0';
    return (int)index;
}

/*
  Intoarce numele fisierului dintr-o cale.
*/
static const char *get_basename(const char *path)
{
    const char *last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

/*
  Citeste un fisier in memorie.
*/
static int read_file(const char *path, char **buffer_out, size_t *size_out)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);
    if (file_size < 0)
    {
        fclose(file);
        return -1;
    }

    rewind(file);

    char *buffer = (char *)malloc((size_t)file_size);
    if (buffer == NULL)
    {
        fclose(file);
        return -1;
    }

    size_t read_count = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);

    if (read_count != (size_t)file_size)
    {
        free(buffer);
        return -1;
    }

    *buffer_out = buffer;
    *size_out = (size_t)file_size;

    return 0;
}

/*
  Primeste raspuns RESULT de la server.
*/
static int receive_result(int sockfd)
{
    char line[MAX_LINE];

    if (recv_line(sockfd, line, sizeof(line)) <= 0)
    {
        return -1;
    }

    size_t result_size = 0;

    if (sscanf(line, "RESULT %zu", &result_size) != 1)
    {
        fprintf(stderr, "Raspuns invalid de la server.\n");
        return -1;
    }

    char *result = (char *)malloc(result_size + 1U);
    if (result == NULL)
    {
        return -1;
    }

    if (recv_all(sockfd, result, result_size) != 0)
    {
        free(result);
        return -1;
    }

    result[result_size] = '\0';

    printf("%s", result);

    free(result);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Utilizare: %s <fisier_sursa>\n", argv[0]);
        return 1;
    }

    const char *filepath = argv[1];
    const char *filename = get_basename(filepath);

    char *file_buffer = NULL;
    size_t file_size = 0;

    if (read_file(filepath, &file_buffer, &file_size) != 0)
    {
        fprintf(stderr, "Nu s-a putut citi fisierul: %s\n", filepath);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        free(file_buffer);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        free(file_buffer);
        (void)close(sockfd);
        return 1;
    }

    char header[MAX_LINE];

    int written = snprintf(header, sizeof(header), "UPLOAD %s %zu\n", filename, file_size);
    if (written < 0 || (size_t)written >= sizeof(header))
    {
        fprintf(stderr, "Header prea lung.\n");
        free(file_buffer);
        (void)close(sockfd);
        return 1;
    }

    if (send_all(sockfd, header, strlen(header)) != 0)
    {
        fprintf(stderr, "Eroare la trimiterea headerului.\n");
        free(file_buffer);
        (void)close(sockfd);
        return 1;
    }

    if (send_all(sockfd, file_buffer, file_size) != 0)
    {
        fprintf(stderr, "Eroare la trimiterea fisierului.\n");
        free(file_buffer);
        (void)close(sockfd);
        return 1;
    }

    free(file_buffer);

    if (receive_result(sockfd) != 0)
    {
        fprintf(stderr, "Eroare la primirea rezultatului.\n");
        (void)close(sockfd);
        return 1;
    }

    (void)close(sockfd);
    return 0;
}
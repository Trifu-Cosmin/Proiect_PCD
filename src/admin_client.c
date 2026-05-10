/*
  Descriere:
  Acest fisier implementeaza clientul de administrare.

  Pentru moment, clientul admin trimite comanda STATS catre server si afiseaza:
  - statusul serverului
  - cate fisiere au fost analizate
  - ultimul fisier analizat

  Acest client poate fi extins ulterior cu operatii precum logs, shutdown,
  configurare, lista clienti etc.
*/

#include <arpa/inet.h>  // Pentru inet_addr
#include <netinet/in.h> // Pentru sockaddr_in
#include <stdio.h>      // Pentru printf, fprintf
#include <stdlib.h>     // Pentru malloc, free
#include <string.h>     // Pentru memset, strlen, sscanf
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

int main(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
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
        (void)close(sockfd);
        return 1;
    }

    if (send_all(sockfd, "STATS\n", 6) != 0)
    {
        fprintf(stderr, "Eroare la trimiterea comenzii STATS.\n");
        (void)close(sockfd);
        return 1;
    }

    if (receive_result(sockfd) != 0)
    {
        fprintf(stderr, "Eroare la primirea statisticilor.\n");
        (void)close(sockfd);
        return 1;
    }

    (void)close(sockfd);
    return 0;
}
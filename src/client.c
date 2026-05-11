/*
  Descriere:
  Acest fisier implementeaza clientul normal al aplicatiei.

  Clientul poate:
  - trimite un fisier sursa catre server pentru analiza
  - primi rezultatul analizei
  - descarca un raport salvat pe server

  Moduri de rulare:
    ./client tests/sample.c
    ./client upload tests/sample.c
    ./client download sample_report.txt
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_LINE 1024
#define FILE_CHUNK 4096

/*
  Creeaza un director daca nu exista.
*/
static int ensure_dir(const char *dirname)
{
    if (mkdir(dirname, 0755) == -1)
    {
        if (errno != EEXIST)
        {
            return -1;
        }
    }

    return 0;
}

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
  Creeaza conexiunea TCP catre server.
*/
static int connect_to_server(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
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
        return -1;
    }

    return sockfd;
}

/*
  Citeste un fisier local in memorie.
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

    long file_size_long = ftell(file);
    if (file_size_long < 0)
    {
        fclose(file);
        return -1;
    }

    rewind(file);

    size_t file_size = (size_t)file_size_long;

    char *buffer = (char *)malloc(file_size);
    if (buffer == NULL && file_size > 0U)
    {
        fclose(file);
        return -1;
    }

    size_t read_count = fread(buffer, 1, file_size, file);

    fclose(file);

    if (read_count != file_size)
    {
        free(buffer);
        return -1;
    }

    *buffer_out = buffer;
    *size_out = file_size;

    return 0;
}

/*
  Primeste si afiseaza raspuns RESULT.
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

/*
  Primeste un fisier trimis de server si il salveaza in downloads/.
*/
static int receive_file_response(int sockfd)
{
    char line[MAX_LINE];

    if (recv_line(sockfd, line, sizeof(line)) <= 0)
    {
        return -1;
    }

    if (strncmp(line, "RESULT ", 7) == 0)
    {
        size_t result_size = 0;

        if (sscanf(line, "RESULT %zu", &result_size) != 1)
        {
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
        return 1;
    }

    char filename[MAX_LINE];
    size_t file_size = 0;

    if (sscanf(line, "FILE %1023s %zu", filename, &file_size) != 2)
    {
        fprintf(stderr, "Raspuns FILE invalid.\n");
        return -1;
    }

    if (ensure_dir("downloads") != 0)
    {
        fprintf(stderr, "Nu se poate crea directorul downloads.\n");
        return -1;
    }

    const char *safe_name = get_basename(filename);

    char output_path[MAX_LINE];
    int written = snprintf(output_path, sizeof(output_path), "downloads/%s", safe_name);
    if (written < 0 || (size_t)written >= sizeof(output_path))
    {
        fprintf(stderr, "Nume fisier prea lung.\n");
        return -1;
    }

    FILE *file = fopen(output_path, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Nu se poate salva fisierul descarcat.\n");
        return -1;
    }

    char buffer[FILE_CHUNK];
    size_t remaining = file_size;

    while (remaining > 0U)
    {
        size_t chunk_size = remaining < sizeof(buffer) ? remaining : sizeof(buffer);

        if (recv_all(sockfd, buffer, chunk_size) != 0)
        {
            fclose(file);
            return -1;
        }

        size_t written_count = fwrite(buffer, 1, chunk_size, file);
        if (written_count != chunk_size)
        {
            fclose(file);
            return -1;
        }

        remaining -= chunk_size;
    }

    fclose(file);

    printf("Report downloaded to %s\n", output_path);
    return 0;
}

/*
  Trimite un fisier catre server pentru analiza.
*/
static int upload_file(const char *filepath)
{
    const char *filename = get_basename(filepath);

    char *file_buffer = NULL;
    size_t file_size = 0;

    if (read_file(filepath, &file_buffer, &file_size) != 0)
    {
        fprintf(stderr, "Nu s-a putut citi fisierul: %s\n", filepath);
        return 1;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0)
    {
        free(file_buffer);
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

/*
  Cere serverului descarcarea unui raport.
*/
static int download_report(const char *report_name)
{
    int sockfd = connect_to_server();
    if (sockfd < 0)
    {
        return 1;
    }

    char command[MAX_LINE];

    int written = snprintf(command, sizeof(command), "DOWNLOAD_REPORT %s\n", report_name);
    if (written < 0 || (size_t)written >= sizeof(command))
    {
        fprintf(stderr, "Comanda prea lunga.\n");
        (void)close(sockfd);
        return 1;
    }

    if (send_all(sockfd, command, strlen(command)) != 0)
    {
        fprintf(stderr, "Eroare la trimiterea comenzii DOWNLOAD_REPORT.\n");
        (void)close(sockfd);
        return 1;
    }

    if (receive_file_response(sockfd) != 0)
    {
        fprintf(stderr, "Eroare la descarcarea raportului.\n");
        (void)close(sockfd);
        return 1;
    }

    (void)close(sockfd);
    return 0;
}

/*
  Afiseaza modul de utilizare.
*/
static void print_usage(const char *program_name)
{
    printf("Utilizare:\n");
    printf("  %s <fisier_sursa>\n", program_name);
    printf("  %s upload <fisier_sursa>\n", program_name);
    printf("  %s download <raport.txt>\n", program_name);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        return upload_file(argv[1]);
    }

    if (argc == 3 && strcmp(argv[1], "upload") == 0)
    {
        return upload_file(argv[2]);
    }

    if (argc == 3 && strcmp(argv[1], "download") == 0)
    {
        return download_report(argv[2]);
    }

    print_usage(argv[0]);
    return 1;
}
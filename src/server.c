#define _DEFAULT_SOURCE

/*
  Descriere:
  Acest fisier implementeaza un server TCP minimal pentru proiectul T17.

  Serverul:
  - porneste pe 127.0.0.1:8080
  - accepta conexiuni de la clienti
  - primeste fisiere sursa prin protocol simplu text + payload binar
  - salveaza fisierele in directorul uploads
  - ruleaza analyzer-ul intr-un proces separat folosind fork/exec
  - preia rezultatul analyzer-ului prin pipe
  - trimite rezultatul inapoi clientului

  Serverul mai accepta si comanda STATS, folosita de admin_client.
*/

#include <arpa/inet.h>  // Pentru inet_addr
#include <errno.h>      // Pentru errno, EEXIST
#include <netinet/in.h> // Pentru sockaddr_in
#include <stdio.h>      // Pentru printf, fprintf, snprintf, fopen, fwrite
#include <stdlib.h>     // Pentru malloc, free, strtoull
#include <string.h>     // Pentru strcmp, strncmp, strlen, memset
#include <sys/socket.h> // Pentru socket, bind, listen, accept, send, recv
#include <sys/stat.h>   // Pentru mkdir
#include <sys/types.h>  // Pentru pid_t
#include <sys/wait.h>   // Pentru waitpid
#include <unistd.h>     // Pentru close, pipe, fork, dup2, execl, read

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_LINE 1024
#define MAX_FILENAME 256
#define MAX_PATH 512
#define MAX_RESULT 65536

static unsigned analyzed_files = 0;
static char last_file[MAX_FILENAME] = "none";

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
  Creeaza directorul uploads daca nu exista deja.
*/
static int ensure_uploads_dir(void)
{
    if (mkdir("uploads", 0755) == -1)
    {
        if (errno != EEXIST)
        {
            return -1;
        }
    }

    return 0;
}

/*
  Intoarce doar numele fisierului dintr-o cale.
  Exemplu: tests/sample.c -> sample.c
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
  Ruleaza analyzer-ul intr-un proces copil.
  stdout-ul copilului este redirectionat catre pipe, iar parintele citeste
  rezultatul in result.
*/
static int run_analyzer(const char *filepath, char *result, size_t result_size)
{
    int pipefd[2];

    if (result_size == 0U)
    {
        return -1;
    }

    if (pipe(pipefd) == -1)
    {
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        (void)close(pipefd[0]);
        (void)close(pipefd[1]);
        return -1;
    }

    if (pid == 0)
    {
        (void)close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            _exit(1);
        }

        (void)close(pipefd[1]);

        execl("./analyzer", "analyzer", "-f", filepath, "-v", (char *)NULL);

        _exit(1);
    }

    (void)close(pipefd[1]);

    size_t total_read = 0;

    while (total_read < result_size - 1U)
    {
        ssize_t bytes_read = read(pipefd[0], result + total_read, result_size - 1U - total_read);

        if (bytes_read < 0)
        {
            (void)close(pipefd[0]);
            return -1;
        }

        if (bytes_read == 0)
        {
            break;
        }

        total_read += (size_t)bytes_read;
    }

    result[total_read] = '\0';

    (void)close(pipefd[0]);

    int status = 0;

    if (waitpid(pid, &status, 0) == -1)
    {
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        return -1;
    }

    return 0;
}

/*
  Trimite un raspuns de tip RESULT catre client.
*/
static int send_result_response(int client_fd, const char *text)
{
    char header[MAX_LINE];
    size_t len = strlen(text);

    int written = snprintf(header, sizeof(header), "RESULT %zu\n", len);
    if (written < 0 || (size_t)written >= sizeof(header))
    {
        return -1;
    }

    if (send_all(client_fd, header, strlen(header)) != 0)
    {
        return -1;
    }

    if (send_all(client_fd, text, len) != 0)
    {
        return -1;
    }

    return 0;
}

/*
  Tratare comanda STATS pentru clientul de administrare.
*/
static int handle_stats(int client_fd)
{
    char stats[MAX_RESULT];

    int written = snprintf(stats,
                           sizeof(stats),
                           "Server status: running\nAnalyzed files: %u\nLast file: %s\n",
                           analyzed_files,
                           last_file);

    if (written < 0 || (size_t)written >= sizeof(stats))
    {
        return -1;
    }

    return send_result_response(client_fd, stats);
}

/*
  Tratare comanda UPLOAD.
  Format comanda:
    UPLOAD <filename> <size>
  Dupa linie, clientul trimite exact size bytes.
*/
static int handle_upload(int client_fd, const char *line)
{
    char filename[MAX_FILENAME];
    size_t file_size = 0;

    if (sscanf(line, "UPLOAD %255s %zu", filename, &file_size) != 2)
    {
        return send_result_response(client_fd, "ERROR: invalid UPLOAD command\n");
    }

    if (file_size == 0U)
    {
        return send_result_response(client_fd, "ERROR: empty file\n");
    }

    if (ensure_uploads_dir() != 0)
    {
        return send_result_response(client_fd, "ERROR: cannot create uploads directory\n");
    }

    char *file_buffer = (char *)malloc(file_size);
    if (file_buffer == NULL)
    {
        return send_result_response(client_fd, "ERROR: memory allocation failed\n");
    }

    if (recv_all(client_fd, file_buffer, file_size) != 0)
    {
        free(file_buffer);
        return -1;
    }

    const char *safe_name = get_basename(filename);
    char saved_path[MAX_PATH];

    int written = snprintf(saved_path, sizeof(saved_path), "uploads/%s", safe_name);
    if (written < 0 || (size_t)written >= sizeof(saved_path))
    {
        free(file_buffer);
        return send_result_response(client_fd, "ERROR: filename too long\n");
    }

    FILE *file = fopen(saved_path, "wb");
    if (file == NULL)
    {
        free(file_buffer);
        return send_result_response(client_fd, "ERROR: cannot save uploaded file\n");
    }

    size_t bytes_written = fwrite(file_buffer, 1, file_size, file);
    fclose(file);
    free(file_buffer);

    if (bytes_written != file_size)
    {
        return send_result_response(client_fd, "ERROR: file write failed\n");
    }

    char analysis_result[MAX_RESULT];

    if (run_analyzer(saved_path, analysis_result, sizeof(analysis_result)) != 0)
    {
        return send_result_response(client_fd, "ERROR: analyzer failed\n");
    }

    analyzed_files++;

    written = snprintf(last_file, sizeof(last_file), "%s", safe_name);
    if (written < 0 || (size_t)written >= sizeof(last_file))
    {
        (void)snprintf(last_file, sizeof(last_file), "unknown");
    }

    return send_result_response(client_fd, analysis_result);
}

/*
  Proceseaza o conexiune client.
*/
static void handle_client(int client_fd)
{
    char line[MAX_LINE];

    if (recv_line(client_fd, line, sizeof(line)) <= 0)
    {
        return;
    }

    if (strncmp(line, "UPLOAD ", 7) == 0)
    {
        (void)handle_upload(client_fd, line);
    }
    else if (strcmp(line, "STATS") == 0)
    {
        (void)handle_stats(client_fd);
    }
    else if (strcmp(line, "QUIT") == 0)
    {
        (void)send_result_response(client_fd, "BYE\n");
    }
    else
    {
        (void)send_result_response(client_fd, "ERROR: unknown command\n");
    }
}

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        (void)close(server_fd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        (void)close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        (void)close(server_fd);
        return 1;
    }

    printf("Server running on %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Waiting for clients...\n");

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        printf("Client connected.\n");

        handle_client(client_fd);

        (void)close(client_fd);
        printf("Client disconnected.\n");
    }

    (void)close(server_fd);
    return 0;
}
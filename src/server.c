#define _DEFAULT_SOURCE

/*
  Descriere:
  Acest fisier implementeaza serverul TCP pentru proiectul T17.

  Serverul:
  - porneste pe 127.0.0.1:8080
  - accepta conexiuni de la clienti
  - primeste fisiere sursa prin comanda UPLOAD
  - salveaza fisierele primite in directorul uploads/
  - ruleaza analyzer-ul intr-un proces separat folosind fork, exec si pipe
  - trimite rezultatul analizei inapoi clientului
  - salveaza raportul generat in directorul reports/
  - scrie evenimente importante in logs/server.log
  - raspunde la comenzi admin: STATS, LOGS, LIST_UPLOADS, LIST_REPORTS

  Protocol simplificat:
    UPLOAD <filename> <size>
    STATS
    LOGS
    LIST_UPLOADS
    LIST_REPORTS
*/

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

#define MAX_LINE 1024
#define MAX_FILENAME 256
#define MAX_PATH 512
#define MAX_RESULT 65536

static unsigned analyzed_files = 0;
static char last_file[MAX_FILENAME] = "none";

/*
  Creeaza un director daca nu exista deja.
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
  Scrie un mesaj in logs/server.log.
*/
static void log_message(const char *level, const char *format, ...)
{
    if (ensure_dir("logs") != 0)
    {
        return;
    }

    FILE *file = fopen("logs/server.log", "a");
    if (file == NULL)
    {
        return;
    }

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    if (local_time != NULL)
    {
        fprintf(file,
                "[%04d-%02d-%02d %02d:%02d:%02d] %s ",
                local_time->tm_year + 1900,
                local_time->tm_mon + 1,
                local_time->tm_mday,
                local_time->tm_hour,
                local_time->tm_min,
                local_time->tm_sec,
                level);
    }
    else
    {
        fprintf(file, "[unknown-time] %s ", level);
    }

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fprintf(file, "\n");
    fclose(file);
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
  Creeaza numele fara extensie.
  Exemplu: sample.c -> sample
*/
static void make_file_stem(const char *filename, char *stem, size_t stem_size)
{
    int written = snprintf(stem, stem_size, "%s", filename);

    if (written < 0 || (size_t)written >= stem_size)
    {
        if (stem_size > 0U)
        {
            stem[0] = '\0';
        }
        return;
    }

    char *dot = strrchr(stem, '.');
    if (dot != NULL)
    {
        *dot = '\0';
    }
}

/*
  Scrie un text intr-un fisier.
*/
static int write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        return -1;
    }

    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, file);

    fclose(file);

    if (written != len)
    {
        return -1;
    }

    return 0;
}

/*
  Salveaza rezultatul analizei in reports/<nume>_report.txt.
*/
static int save_report(const char *filename, const char *analysis_result)
{
    if (ensure_dir("reports") != 0)
    {
        return -1;
    }

    char stem[MAX_FILENAME];
    make_file_stem(filename, stem, sizeof(stem));

    if (stem[0] == '\0')
    {
        return -1;
    }

    char report_path[MAX_PATH];

    int written = snprintf(report_path, sizeof(report_path), "reports/%s_report.txt", stem);
    if (written < 0 || (size_t)written >= sizeof(report_path))
    {
        return -1;
    }

    return write_text_file(report_path, analysis_result);
}

/*
  Adauga text intr-un buffer.
*/
static int append_text(char *buffer, size_t buffer_size, size_t *used, const char *format, ...)
{
    if (*used >= buffer_size)
    {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int written = vsnprintf(buffer + *used, buffer_size - *used, format, args);

    va_end(args);

    if (written < 0)
    {
        return -1;
    }

    if ((size_t)written >= buffer_size - *used)
    {
        *used = buffer_size - 1U;
        buffer[*used] = '\0';
        return -1;
    }

    *used += (size_t)written;
    return 0;
}

/*
  Ruleaza analyzer-ul intr-un proces separat.

  Se folosesc:
  - pipe pentru preluarea outputului
  - fork pentru proces copil
  - dup2 pentru redirectionarea stdout
  - execl pentru rularea analyzer-ului
  - waitpid pentru asteptarea copilului
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
  Trimite raspuns de forma:
    RESULT <size>
    <payload>
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
  Raspunde la comanda STATS.
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

    log_message("INFO", "Admin requested STATS");
    return send_result_response(client_fd, stats);
}

/*
  Raspunde la comanda LOGS.
*/
static int handle_logs(int client_fd)
{
    FILE *file = fopen("logs/server.log", "r");
    if (file == NULL)
    {
        return send_result_response(client_fd, "No logs available.\n");
    }

    char logs[MAX_RESULT];
    size_t read_count = fread(logs, 1, sizeof(logs) - 1U, file);

    fclose(file);

    logs[read_count] = '\0';

    log_message("INFO", "Admin requested LOGS");
    return send_result_response(client_fd, logs);
}

/*
  Listeaza fisierele dintr-un director.
*/
static int handle_list_directory(int client_fd, const char *dirname, const char *title)
{
    if (ensure_dir(dirname) != 0)
    {
        return send_result_response(client_fd, "ERROR: cannot access directory\n");
    }

    DIR *dir = opendir(dirname);
    if (dir == NULL)
    {
        return send_result_response(client_fd, "ERROR: cannot open directory\n");
    }

    char response[MAX_RESULT];
    size_t used = 0;
    unsigned count = 0;

    response[0] = '\0';

    (void)append_text(response, sizeof(response), &used, "%s\n", title);

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (entry->d_name[0] == '.')
        {
            continue;
        }

        (void)append_text(response, sizeof(response), &used, "- %s\n", entry->d_name);
        count++;
    }

    closedir(dir);

    if (count == 0U)
    {
        (void)append_text(response, sizeof(response), &used, "(empty)\n");
    }

    log_message("INFO", "Admin requested directory listing: %s", dirname);
    return send_result_response(client_fd, response);
}

/*
  Proceseaza comanda UPLOAD.
*/
static int handle_upload(int client_fd, const char *line)
{
    char filename[MAX_FILENAME];
    size_t file_size = 0;

    if (sscanf(line, "UPLOAD %255s %zu", filename, &file_size) != 2)
    {
        log_message("ERROR", "Invalid UPLOAD command");
        return send_result_response(client_fd, "ERROR: invalid UPLOAD command\n");
    }

    if (file_size == 0U)
    {
        log_message("ERROR", "Upload rejected: empty file");
        return send_result_response(client_fd, "ERROR: empty file\n");
    }

    if (ensure_dir("uploads") != 0)
    {
        log_message("ERROR", "Cannot create uploads directory");
        return send_result_response(client_fd, "ERROR: cannot create uploads directory\n");
    }

    char *file_buffer = (char *)malloc(file_size);
    if (file_buffer == NULL)
    {
        log_message("ERROR", "Memory allocation failed");
        return send_result_response(client_fd, "ERROR: memory allocation failed\n");
    }

    if (recv_all(client_fd, file_buffer, file_size) != 0)
    {
        free(file_buffer);
        log_message("ERROR", "Failed receiving uploaded file");
        return -1;
    }

    const char *safe_name = get_basename(filename);
    char saved_path[MAX_PATH];

    int written = snprintf(saved_path, sizeof(saved_path), "uploads/%s", safe_name);
    if (written < 0 || (size_t)written >= sizeof(saved_path))
    {
        free(file_buffer);
        log_message("ERROR", "Uploaded filename too long");
        return send_result_response(client_fd, "ERROR: filename too long\n");
    }

    FILE *file = fopen(saved_path, "wb");
    if (file == NULL)
    {
        free(file_buffer);
        log_message("ERROR", "Cannot save uploaded file: %s", saved_path);
        return send_result_response(client_fd, "ERROR: cannot save uploaded file\n");
    }

    size_t bytes_written = fwrite(file_buffer, 1, file_size, file);

    fclose(file);
    free(file_buffer);

    if (bytes_written != file_size)
    {
        log_message("ERROR", "File write failed for: %s", saved_path);
        return send_result_response(client_fd, "ERROR: file write failed\n");
    }

    log_message("INFO", "File uploaded: %s (%zu bytes)", safe_name, file_size);

    char analysis_result[MAX_RESULT];

    if (run_analyzer(saved_path, analysis_result, sizeof(analysis_result)) != 0)
    {
        log_message("ERROR", "Analyzer failed for file: %s", saved_path);
        return send_result_response(client_fd, "ERROR: analyzer failed\n");
    }

    if (save_report(safe_name, analysis_result) != 0)
    {
        log_message("ERROR", "Could not save report for file: %s", safe_name);
    }
    else
    {
        log_message("INFO", "Report saved for file: %s", safe_name);
    }

    analyzed_files++;

    written = snprintf(last_file, sizeof(last_file), "%s", safe_name);
    if (written < 0 || (size_t)written >= sizeof(last_file))
    {
        (void)snprintf(last_file, sizeof(last_file), "unknown");
    }

    log_message("INFO", "Analysis completed for file: %s", safe_name);

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
        log_message("ERROR", "Failed to receive command from client");
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
    else if (strcmp(line, "LOGS") == 0)
    {
        (void)handle_logs(client_fd);
    }
    else if (strcmp(line, "LIST_UPLOADS") == 0)
    {
        (void)handle_list_directory(client_fd, "uploads", "Uploaded files:");
    }
    else if (strcmp(line, "LIST_REPORTS") == 0)
    {
        (void)handle_list_directory(client_fd, "reports", "Generated reports:");
    }
    else if (strcmp(line, "QUIT") == 0)
    {
        (void)send_result_response(client_fd, "BYE\n");
    }
    else
    {
        log_message("ERROR", "Unknown command received: %s", line);
        (void)send_result_response(client_fd, "ERROR: unknown command\n");
    }
}

int main(void)
{
    (void)ensure_dir("uploads");
    (void)ensure_dir("reports");
    (void)ensure_dir("logs");

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

    log_message("INFO", "Server started on %s:%d", SERVER_IP, SERVER_PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0)
        {
            perror("accept");
            log_message("ERROR", "accept failed");
            continue;
        }

        printf("Client connected.\n");
        log_message("INFO", "Client connected");

        handle_client(client_fd);

        (void)close(client_fd);

        printf("Client disconnected.\n");
        log_message("INFO", "Client disconnected");
    }

    (void)close(server_fd);
    return 0;
}
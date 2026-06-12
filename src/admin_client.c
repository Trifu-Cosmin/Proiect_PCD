#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define ADMIN_SOCKET_PATH "/tmp/t17_admin.sock"
#define BUFFER_SIZE 4096
#define MAX_LINE 1024

static int send_all(int sockfd, const void *buffer, size_t length)
{
    const char *data = (const char *)buffer;
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t n = send(sockfd, data + sent, length - sent, 0);
        if (n <= 0)
        {
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}

static int recv_all(int sockfd, void *buffer, size_t length)
{
    char *data = (char *)buffer;
    size_t received = 0;

    while (received < length)
    {
        ssize_t n = recv(sockfd, data + received, length - received, 0);
        if (n <= 0)
        {
            return -1;
        }

        received += (size_t)n;
    }

    return 0;
}

static int recv_line(int sockfd, char *buffer, size_t size)
{
    size_t pos = 0;

    while (pos + 1 < size)
    {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);

        if (n <= 0)
        {
            if (pos == 0)
            {
                return -1;
            }

            break;
        }

        if (c == '\n')
        {
            break;
        }

        buffer[pos++] = c;
    }

    buffer[pos] = '\0';
    return (int)pos;
}

static int connect_to_server(void)
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket UNIX");
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sun_family = AF_UNIX;

    int written = snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), "%s", ADMIN_SOCKET_PATH);
    if (written < 0 || (size_t)written >= sizeof(server_addr.sun_path))
    {
        fprintf(stderr, "UNIX socket path too long.\n");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect UNIX");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int read_result_response(int sockfd)
{
    char line[MAX_LINE];

    if (recv_line(sockfd, line, sizeof(line)) <= 0)
    {
        fprintf(stderr, "Failed to receive response header.\n");
        return -1;
    }

    if (strncmp(line, "RESULT ", 7) != 0)
    {
        printf("%s\n", line);
        return 0;
    }

    long size = atol(line + 7);
    if (size < 0)
    {
        fprintf(stderr, "Invalid response size.\n");
        return -1;
    }

    char *body = malloc((size_t)size + 1);
    if (body == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return -1;
    }

    if (recv_all(sockfd, body, (size_t)size) != 0)
    {
        fprintf(stderr, "Failed to receive response body.\n");
        free(body);
        return -1;
    }

    body[size] = '\0';
    printf("\n%s\n", body);

    free(body);
    return 0;
}

static int send_admin_command(const char *command)
{
    int sockfd = connect_to_server();
    if (sockfd < 0)
    {
        return -1;
    }

    const char *login = "LOGIN admin admin123\n";

    if (send_all(sockfd, login, strlen(login)) != 0)
    {
        fprintf(stderr, "Failed to send login.\n");
        close(sockfd);
        return -1;
    }

    if (read_result_response(sockfd) != 0)
    {
        close(sockfd);
        return -1;
    }

    char command_line[MAX_LINE];

    int written = snprintf(command_line, sizeof(command_line), "%s\n", command);
    if (written < 0 || (size_t)written >= sizeof(command_line))
    {
        fprintf(stderr, "Command too long.\n");
        close(sockfd);
        return -1;
    }

    if (send_all(sockfd, command_line, strlen(command_line)) != 0)
    {
        fprintf(stderr, "Failed to send command.\n");
        close(sockfd);
        return -1;
    }

    int result = read_result_response(sockfd);

    close(sockfd);
    return result;
}

static void read_input(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
}

static void show_menu(void)
{
    printf("\n=== Admin Client ===\n");
    printf("1. Server stats\n");
    printf("2. Show server logs\n");
    printf("3. List uploaded files\n");
    printf("4. List reports\n");
    printf("5. Server status\n");
    printf("6. Clear server logs\n");
    printf("7. Delete report\n");
    printf("8. List users\n");
    printf("9. Show analysis jobs\n");
    printf("10. Quit\n");
    printf("Choose option: ");
}

int main(void)
{
    while (1)
    {
        char option_line[32];
        int option;

        show_menu();
        read_input(option_line, sizeof(option_line));

        option = atoi(option_line);

        if (option == 1)
        {
            send_admin_command("STATS");
        }
        else if (option == 2)
        {
            send_admin_command("LOGS");
        }
        else if (option == 3)
        {
            send_admin_command("LIST_UPLOADS");
        }
        else if (option == 4)
        {
            send_admin_command("LIST_REPORTS");
        }
        else if (option == 5)
        {
            send_admin_command("SERVER_STATUS");
        }
        else if (option == 6)
        {
            send_admin_command("CLEAR_LOGS");
        }
        else if (option == 7)
        {
            char report_name[256];
            char command[MAX_LINE];

            printf("Report name: ");
            read_input(report_name, sizeof(report_name));

            if (report_name[0] == '\0')
            {
                printf("Invalid report name.\n");
                continue;
            }

            int written = snprintf(command, sizeof(command), "DELETE_REPORT %s", report_name);
            if (written < 0 || (size_t)written >= sizeof(command))
            {
                printf("Report name too long.\n");
                continue;
            }

            send_admin_command(command);
        }
        else if (option == 8)
        {
            send_admin_command("LIST_USERS");
        }
        else if (option == 9)
        {
            send_admin_command("JOBS");
        }
        else if (option == 10)
        {
            send_admin_command("QUIT");
            break;
        }
        else
        {
            printf("Optiune invalida.\n");
        }
    }

    return 0;
}
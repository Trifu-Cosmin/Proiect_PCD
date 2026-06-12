#define _DEFAULT_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define REPORTS_DIR "reports"
#define EVENT_BUFFER_SIZE 8192

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

static void print_time_prefix(void)
{
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    if (local_time != NULL)
    {
        printf("[%04d-%02d-%02d %02d:%02d:%02d] ",
               local_time->tm_year + 1900,
               local_time->tm_mon + 1,
               local_time->tm_mday,
               local_time->tm_hour,
               local_time->tm_min,
               local_time->tm_sec);
    }
    else
    {
        printf("[unknown-time] ");
    }
}

static const char *get_event_name(uint32_t mask)
{
    if ((mask & IN_CREATE) != 0U)
    {
        return "created";
    }

    if ((mask & IN_CLOSE_WRITE) != 0U)
    {
        return "written";
    }

    if ((mask & IN_MODIFY) != 0U)
    {
        return "modified";
    }

    if ((mask & IN_DELETE) != 0U)
    {
        return "deleted";
    }

    if ((mask & IN_MOVED_FROM) != 0U)
    {
        return "moved_from";
    }

    if ((mask & IN_MOVED_TO) != 0U)
    {
        return "moved_to";
    }

    return "changed";
}

int main(void)
{
    if (ensure_dir(REPORTS_DIR) != 0)
    {
        perror("mkdir reports");
        return 1;
    }

    int inotify_fd = inotify_init();
    if (inotify_fd < 0)
    {
        perror("inotify_init");
        return 1;
    }

    uint32_t mask = IN_CREATE |
                    IN_MODIFY |
                    IN_CLOSE_WRITE |
                    IN_DELETE |
                    IN_MOVED_FROM |
                    IN_MOVED_TO;

    int watch_fd = inotify_add_watch(inotify_fd, REPORTS_DIR, mask);
    if (watch_fd < 0)
    {
        perror("inotify_add_watch");
        close(inotify_fd);
        return 1;
    }

    printf("Watching directory: %s\n", REPORTS_DIR);
    printf("Waiting for report changes...\n");

    while (1)
    {
        char buffer[EVENT_BUFFER_SIZE];

        ssize_t length = read(inotify_fd, buffer, sizeof(buffer));
        if (length < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("read");
            break;
        }

        ssize_t index = 0;

        while (index < length)
        {
            struct inotify_event *event = (struct inotify_event *)(buffer + index);

            if (event->len > 0U && event->name[0] != '.')
            {
                print_time_prefix();

                if ((event->mask & IN_ISDIR) != 0U)
                {
                    printf("directory %s: %s\n", get_event_name(event->mask), event->name);
                }
                else
                {
                    printf("report %s: %s\n", get_event_name(event->mask), event->name);
                }

                fflush(stdout);
            }

            index += (ssize_t)(sizeof(struct inotify_event) + event->len);
        }
    }

    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 80

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info;

int request_count = 0;
int total_received_bytes = 0;
int total_sent_bytes = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg)
{
    client_info *info = (client_info *)arg;
    int client_fd = info->client_fd;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while ((n = read(client_fd, buffer, BUFFER_SIZE)) > 0)
    {
        buffer[n] = '\0'; // Null-terminate the received string

        pthread_mutex_lock(&stats_mutex);
        request_count++;
        total_received_bytes += n;
        pthread_mutex_unlock(&stats_mutex);

        if (strncmp(buffer, "GET /static/", 12) == 0)
        {
            char *file_path = buffer + 4; // Skip "GET "
            file_path[strcspn(file_path, " ")] = '\0'; // Null-terminate at the first space

            char full_path[BUFFER_SIZE];
            snprintf(full_path, BUFFER_SIZE, ".%s", file_path); // Use relative path

            printf("Attempting to serve file: %s\n", full_path);

            int file_fd = open(full_path, O_RDONLY);
            if (file_fd < 0)
            {
                perror("open");
                write(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
            }
            else
            {
                write(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19);
                while ((n = read(file_fd, buffer, BUFFER_SIZE)) > 0)
                {
                    write(client_fd, buffer, n);
                    pthread_mutex_lock(&stats_mutex);
                    total_sent_bytes += n;
                    pthread_mutex_unlock(&stats_mutex);
                }
                close(file_fd);
            }
        }
        else if (strncmp(buffer, "GET /stats", 10) == 0)
        {
            char stats_response[BUFFER_SIZE];
            snprintf(stats_response, BUFFER_SIZE,
                     "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body>"
                     "<h1>Server Stats</h1>"
                     "<p>Requests: %d</p>"
                     "<p>Received Bytes: %d</p>"
                     "<p>Sent Bytes: %d</p>"
                     "</body></html>",
                     request_count, total_received_bytes, total_sent_bytes);
            write(client_fd, stats_response, strlen(stats_response));
        }
        else if (strncmp(buffer, "GET /calc/", 10) == 0)
        {
            int a, b;
            sscanf(buffer, "GET /calc/%d/%d", &a, &b);
            char calc_response[BUFFER_SIZE];
            snprintf(calc_response, BUFFER_SIZE,
                     "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body>"
                     "<h1>Calculation Result</h1>"
                     "<p>%d + %d = %d</p>"
                     "</body></html>",
                     a, b, a + b);
            write(client_fd, calc_response, strlen(calc_response));
        }
        else
        {
            write(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 28);
        }
    }

    if (n < 0)
    {
        error("read");
    }

    close(client_fd);
    free(info);
    return NULL;
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    if (argc == 3 && strcmp(argv[1], "-p") == 0)
    {
        port = atoi(argv[2]);
    }

    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("socket");
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error("bind");
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0)
    {
        error("listen");
    }

    printf("Server listening on port %d\n", port);

    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd");
    }

    while (1)
    {
        client_info *info = malloc(sizeof(client_info));
        socklen_t client_len = sizeof(info->client_addr);

        // Accept a new connection
        if ((info->client_fd = accept(server_fd, (struct sockaddr *)&info->client_addr, &client_len)) < 0)
        {
            error("accept");
        }

        printf("Client connected\n");

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, info);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

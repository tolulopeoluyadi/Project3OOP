#include "server.h"

int chat_serv_sock_fd;
struct node *head = NULL; // Linked list of all users
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER;
int numReaders = 0;

struct room *default_room;

// Function to create a new chat room
struct room *create_room(char *name) {
    struct room *new_room = (struct room *)malloc(sizeof(struct room));
    if (!new_room) {
        perror("malloc failed");
        return NULL;
    }
    strcpy(new_room->name, name);
    new_room->user_list = NULL;
    new_room->next = NULL;
    return new_room;
}

// Function to free a chat room
void free_room(struct room *room) {
    if (!room) return;
    struct node *current = room->user_list;
    while (current) {
        struct node *temp = current;
        current = current->next;
        free(temp);
    }
    free(room);
}

// Function to add a new user to the global user list
void add_user(int socket_fd) {
    struct node *new_user = (struct node *)malloc(sizeof(struct node));
    if (!new_user) {
        perror("malloc failed");
        return;
    }
    new_user->socket = socket_fd;
    snprintf(new_user->username, sizeof(new_user->username), "guest%d", socket_fd);
    new_user->next = head;
    head = new_user;
}

// Function to add a user to a chat room
void join_room(struct room *room, int socket_fd) {
    if (!room) return;
    struct node *new_user = (struct node *)malloc(sizeof(struct node));
    if (!new_user) {
        perror("malloc failed");
        return;
    }
    new_user->socket = socket_fd;
    snprintf(new_user->username, sizeof(new_user->username), "guest%d", socket_fd);
    new_user->next = room->user_list;
    room->user_list = new_user;
}

// Signal handler for SIGINT
void sigintHandler(int sig_num) {
    printf("Shutting down server...\n");
    struct node *current = head;
    while (current) {
        close(current->socket); // Close client sockets
        struct node *temp = current;
        current = current->next;
        free(temp);
    }
    free_room(default_room); // Free default room
    close(chat_serv_sock_fd);
    exit(0);
}

// Thread function for handling client communication
void *client_receive(void *ptr) {
    int client = *(int *)ptr;
    char buffer[MAXBUFF];
    char command[MAXBUFF];
    char argument[MAXBUFF];
    
    send(client, "Welcome to the chat server!\nchat> ", 34, 0);
    
    while (1) {
        int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected: %d\n", client);
            close(client);
            pthread_exit(NULL);
        }

        buffer[bytes_received] = '\0';
        printf("Received from client %d: %s\n", client, buffer);

        sscanf(buffer, "%s %s", command, argument);

        if (strcmp(command, "login") == 0) {
            pthread_mutex_lock(&mutex);
            struct node *current = head;
            while (current) {
                if (current->socket == client) {
                    snprintf(current->username, sizeof(current->username), "%s", argument);
                    break;
                }
                current = current->next;
            }
            pthread_mutex_unlock(&mutex);
            send(client, "Login successful.\nchat> ", 23, 0);
        } else if (strcmp(command, "create") == 0) {
            struct room *new_room = create_room(argument);
            if (new_room) {
                new_room->next = default_room;
                default_room = new_room;
                send(client, "Room created successfully.\nchat> ", 32, 0);
            } else {
                send(client, "Failed to create room.\nchat> ", 28, 0);
            }
        } else if (strcmp(command, "join") == 0) {
            pthread_mutex_lock(&mutex);
            struct room *room = default_room;
            while (room) {
                if (strcmp(room->name, argument) == 0) {
                    join_room(room, client);
                    send(client, "Joined room successfully.\nchat> ", 32, 0);
                    pthread_mutex_unlock(&mutex);
                    return NULL;
                }
                room = room->next;
            }
            pthread_mutex_unlock(&mutex);
            send(client, "Room not found.\nchat> ", 23, 0);
        } else if (strcmp(command, "exit") == 0) {
            close(client);
            pthread_exit(NULL);
        } else {
            strcat(buffer, " (unrecognized command)\nchat> ");
            send(client, buffer, strlen(buffer), 0);
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, sigintHandler);

    // Create the default room (Lobby)
    default_room = create_room(DEFAULT_ROOM);
    if (!default_room) {
        fprintf(stderr, "Failed to create default room.\n");
        exit(EXIT_FAILURE);
    }

    // Open server socket
    chat_serv_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (chat_serv_sock_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(chat_serv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(chat_serv_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(chat_serv_sock_fd, BACKLOG) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int new_client = accept(chat_serv_sock_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_client < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected: %d\n", new_client);

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_receive, (void *)&new_client);
    }

    return 0;
}


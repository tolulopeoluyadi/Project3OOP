#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

// Constants
#define PORT 8888
#define BACKLOG 2
#define MAXBUFF 2096
#define DEFAULT_ROOM "Lobby"

// Node structure for user management
struct node {
    int socket;
    char username[50];
    struct node *next;
};

// Room structure
struct room {
    char name[50];
    struct node *user_list;
    struct room *next;
};

// Global Variables
extern int chat_serv_sock_fd;
extern struct node *head;
extern pthread_mutex_t mutex;
extern pthread_mutex_t rw_lock;
extern int numReaders;
extern struct room *default_room;

// Function Prototypes
struct room *create_room(char *name);
void free_room(struct room *room);
void add_user(int socket_fd);
void join_room(struct room *room, int socket_fd);
void sigintHandler(int sig_num);
void *client_receive(void *ptr);

#endif

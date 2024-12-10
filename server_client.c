#include "server.h"

// Define a room structure
struct room {
    char name[50];
    struct node *users;
    struct room *next;
};

struct room *room_head = NULL; // Head of the room linked list

// Add a new room
void create_room(const char *room_name) {
    struct room *new_room = (struct room *)malloc(sizeof(struct room));
    if (!new_room) {
        perror("Memory allocation for room failed");
        exit(1);
    }
    strcpy(new_room->name, room_name);
    new_room->users = NULL;
    new_room->next = room_head;
    room_head = new_room;
    printf("Room '%s' created.\n", room_name);
}

// Add a user to a room
void join_room(const char *room_name, const char *username) {
    struct room *current = room_head;
    while (current) {
        if (strcmp(current->name, room_name) == 0) {
            struct node *new_user = (struct node *)malloc(sizeof(struct node));
            if (!new_user) {
                perror("Memory allocation for user in room failed");
                exit(1);
            }
            strcpy(new_user->username, username);
            new_user->next = current->users;
            current->users = new_user;
            printf("User '%s' joined room '%s'.\n", username, room_name);
            return;
        }
        current = current->next;
    }
    printf("Room '%s' does not exist.\n", room_name);
}

// Remove a user from a room
void leave_room(const char *room_name, const char *username) {
    struct room *current = room_head;
    while (current) {
        if (strcmp(current->name, room_name) == 0) {
            struct node **user_ptr = &current->users;
            while (*user_ptr) {
                if (strcmp((*user_ptr)->username, username) == 0) {
                    struct node *temp = *user_ptr;
                    *user_ptr = (*user_ptr)->next;
                    free(temp);
                    printf("User '%s' left room '%s'.\n", username, room_name);
                    return;
                }
                user_ptr = &(*user_ptr)->next;
            }
            printf("User '%s' is not in room '%s'.\n", username, room_name);
            return;
        }
        current = current->next;
    }
    printf("Room '%s' does not exist.\n", room_name);
}

// List all rooms
void list_rooms(char *buffer) {
    struct room *current = room_head;
    strcpy(buffer, "Available rooms:\n");
    while (current) {
        strcat(buffer, current->name);
        strcat(buffer, "\n");
        current = current->next;
    }
}

// List all users
void list_users(char *buffer) {
    struct node *current = head;
    strcpy(buffer, "Connected users:\n");
    while (current) {
        strcat(buffer, current->username);
        strcat(buffer, "\n");
        current = current->next;
    }
}

// Rename a user
void rename_user(const char *old_username, const char *new_username) {
    struct node *current = head;
    while (current) {
        if (strcmp(current->username, old_username) == 0) {
            strcpy(current->username, new_username);
            printf("User '%s' renamed to '%s'.\n", old_username, new_username);
            return;
        }
        current = current->next;
    }
    printf("User '%s' not found.\n", old_username);
}

// Logout a user
void logout_user(int client) {
    struct node **user_ptr = &head;
    while (*user_ptr) {
        if ((*user_ptr)->socket == client) {
            struct node *temp = *user_ptr;
            *user_ptr = (*user_ptr)->next;
            printf("User '%s' logged out.\n", temp->username);
            free(temp);
            return;
        }
        user_ptr = &(*user_ptr)->next;
    }
    printf("User with socket %d not found.\n", client);
}

// Broadcast a message
void broadcast_message(int sender_socket, const char *sender_username, const char *message) {
    struct node *current = head;
    char buffer[MAXBUFF];
    sprintf(buffer, "::%s> %s", sender_username, message);

    while (current) {
        if (current->socket != sender_socket) {
            send(current->socket, buffer, strlen(buffer), 0);
        }
        current = current->next;
    }
}

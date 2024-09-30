// Sure, I can help you with that! Below is a simple example of a C program that uses UDP sockets and threads to send and receive messages. This program will have two threads: one for sending messages and another for receiving messages.

// First, you’ll need to include the necessary headers and create the socket:

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUF_SIZE 1024

void *send_messages(void *sockfd);
void *receive_messages(void *sockfd);

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    // Server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Create threads for sending and receiving messages
    pthread_t send_thread, receive_thread;
    pthread_create(&send_thread, NULL, send_messages, (void *)&sockfd);
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&sockfd);

    // Wait for threads to finish
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    close(sockfd);
    return 0;
}

void *send_messages(void *sockfd) {
    int sock = *(int *)sockfd;
    struct sockaddr_in dest_addr;
    char buffer[BUF_SIZE];
    socklen_t addr_len = sizeof(dest_addr);

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = INADDR_ANY;

    while (1) {
        printf("escreva.\n");
        fgets(buffer, BUF_SIZE, stdin);
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&dest_addr, addr_len);
    }
}

void *receive_messages(void *sockfd) {
    int sock = *(int *)sockfd;
    struct sockaddr_in src_addr;
    char buffer[BUF_SIZE];
    socklen_t addr_len = sizeof(src_addr);

    while (1) {
        int n = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&src_addr, &addr_len);
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
    }
}

// This program does the following:

// Creates a UDP socket.
// Spawns two threads: one for sending messages and one for receiving messages.
// The send_messages function reads input from the user and sends it to the specified address.
// The receive_messages function listens for incoming messages and prints them to the console.
// You can compile this program using gcc:

// gcc -pthread -o udp_chat udp_chat.c

// Run the program on multiple machines or multiple terminal windows on the same machine to test sending and receiving messages.

// Feel free to ask if you have any questions or need further assistance!
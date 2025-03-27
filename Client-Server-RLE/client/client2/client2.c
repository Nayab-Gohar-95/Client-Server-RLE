#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SERVER_PORT 1234
#define BUFFER_SIZE 1024

#pragma comment(lib, "ws2_32.lib")

// Run-Length Encoding functions
void encode_rle(const char *input, char *output);
void decode_rle(const char *input, char *output);

void upload_file(SOCKET client_socket);
void download_file(SOCKET client_socket);
void view_files(SOCKET client_socket);

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char client_id[BUFFER_SIZE];
    char choice;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock. Error code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Failed to create socket. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connect failed. Error code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Get client ID
    printf("Enter your client ID: ");
    scanf("%s", client_id);
    send(client_socket, client_id, strlen(client_id), 0);

    // Main loop for user input
    while (1) {
        printf("Enter 'u' to upload, 'd' to download, or 'v' to view files: ");
        scanf(" %c", &choice);
        if (choice == 'u') {
            upload_file(client_socket);
        } else if (choice == 'd') {
            download_file(client_socket);
        } else if (choice == 'v') {
            view_files(client_socket);
        } else {
            printf("Invalid choice.\n");
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}

// Implementing RLE encoding
void encode_rle(const char *input, char *output) {
    int i = 0, j = 0, count;
    while (input[i] != '\0') {
        count = 1;
        while (input[i] == input[i + 1] && count < 9) {
            count++;
            i++;
        }
        output[j++] = input[i];
        output[j++] = '0' + count;
        i++;
    }
    output[j] = '\0';
}

// Implementing RLE decoding
void decode_rle(const char *input, char *output) {
    int i = 0, j = 0, count;
    while (input[i] != '\0') {
        output[j++] = input[i++];
        count = input[i++] - '0';
        for (int k = 1; k < count; k++) {
            output[j++] = output[j - 1];
        }
    }
    output[j] = '\0';
}

void upload_file(SOCKET client_socket) {
    char file_path[BUFFER_SIZE];
    FILE *file;
    char buffer[BUFFER_SIZE];
    char encoded_buffer[BUFFER_SIZE];
    int bytes_read;

    printf("Enter the file path to upload: ");
    scanf("%s", file_path);
    
    // Send upload command
    send(client_socket, "$UPLOAD$", 8, 0);
    send(client_socket, file_path, strlen(file_path), 0);
    
    // Open file for reading
    file = fopen(file_path, "rb");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return;
    }

    // Send file data in encoded format
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // Encode the buffer using RLE
        encode_rle(buffer, encoded_buffer);
        send(client_socket, encoded_buffer, strlen(encoded_buffer), 0);
    }

    // No need to send "$END$" marker
    // send(client_socket, "$END$", 5, 0); // This line is removed

    fclose(file);
    printf("File upload complete.\n");

    // Receive server response
    char response[BUFFER_SIZE];
    int bytes_received = recv(client_socket, response, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Server Response: %s\n", response);
    }
}

void download_file(SOCKET client_socket) {
    char file_path[BUFFER_SIZE];
    FILE *file;
    char buffer[BUFFER_SIZE];
    char decoded_buffer[BUFFER_SIZE];
    int bytes_received;

    printf("Enter the file name to download: ");
    scanf("%s", file_path);
    
    // Send download request
    send(client_socket, "$REQUEST$", 9, 0);
    send(client_socket, file_path, strlen(file_path), 0);

    // Wait for server response to validate if file exists
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0 || strncmp(buffer, "$ERROR$", 7) == 0) {
        printf("File not found on server.\n");
        return;
    }

    // Open file for writing
    file = fopen(file_path, "wb");
    if (file == NULL) {
        printf("Failed to create file.\n");
        return;
    }

    // Receive file data in encoded format
    do {
        if (strncmp(buffer, "$END$", 5) == 0) {
            break;
        }
        // Decode the buffer using RLE
        decode_rle(buffer, decoded_buffer);
        fwrite(decoded_buffer, 1, strlen(decoded_buffer), file);
        
    } while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0);

    fclose(file);
    printf("File download complete.\n");
}

void view_files(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Send view command
    send(client_socket, "$VIEW$", 6, 0);

    // Receive and print file details
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        if (strncmp(buffer, "$END$", 5) == 0) {
            break;
        }
        printf("%s", buffer);
    }
}

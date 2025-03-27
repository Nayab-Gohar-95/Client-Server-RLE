#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>  // for _beginthread()

#define SERVER_PORT 1234
#define BUFFER_SIZE 1024

#pragma comment(lib, "ws2_32.lib")

void handle_upload(SOCKET client_socket, const char *client_folder);
void handle_download(SOCKET client_socket, const char *client_folder);
void handle_view(SOCKET client_socket, const char *client_folder);
void client_handler(void *socket_desc);

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock. Error code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Failed to create socket. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("Listen failed. Error code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %d...\n", SERVER_PORT);

    // Accept clients in a loop
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) != INVALID_SOCKET) {
        printf("Client connected.\n");
        _beginthread(client_handler, 0, (void*)client_socket);
    }

    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed. Error code: %d\n", WSAGetLastError());
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void client_handler(void *socket_desc) {
    SOCKET client_socket = (SOCKET)socket_desc;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Receive client ID
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    // Create a folder for the client
    char client_folder[BUFFER_SIZE];
    snprintf(client_folder, sizeof(client_folder), "client_%s", buffer);
    CreateDirectory(client_folder, NULL);

    // Main loop for client commands
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (strncmp(buffer, "$UPLOAD$", 8) == 0) {
            handle_upload(client_socket, client_folder);
        } else if (strncmp(buffer, "$REQUEST$", 9) == 0) {
            handle_download(client_socket, client_folder);
        } else if (strncmp(buffer, "$VIEW$", 6) == 0) {
            handle_view(client_socket, client_folder);
        } else {
            send(client_socket, "$ERROR$Unknown_Command$", 23, 0);
        }
    }

    printf("Client disconnected.\n");
    closesocket(client_socket);
}

void handle_upload(SOCKET client_socket, const char *client_folder) {
    char file_path[BUFFER_SIZE];
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char full_path[BUFFER_SIZE];

    // Receive file path
    bytes_received = recv(client_socket, file_path, BUFFER_SIZE, 0);
    if (bytes_received <= 0) return;
    file_path[bytes_received] = '\0';
    
    // Create full file path
    snprintf(full_path, sizeof(full_path), "%s/%s", client_folder, file_path);

    // Open file for writing
    file = fopen(full_path, "wb");
    if (file == NULL) {
        send(client_socket, "$ERROR$Cannot_Create_File$", 26, 0);
        return;
    }

    // Receive file data
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    send(client_socket, "$SUCCESS$File_Uploaded$", 23, 0);
}

void handle_download(SOCKET client_socket, const char *client_folder) {
    char file_path[BUFFER_SIZE];
    char full_path[BUFFER_SIZE];
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Receive file path
    int bytes_received = recv(client_socket, file_path, BUFFER_SIZE, 0);
    if (bytes_received <= 0) return;
    file_path[bytes_received] = '\0';

    // Create full file path
    snprintf(full_path, sizeof(full_path), "%s/%s", client_folder, file_path);

    // Open file for reading
    file = fopen(full_path, "rb");
    if (file == NULL) {
        send(client_socket, "$ERROR$File_Not_Found$", 22, 0);
        return;
    }

    // Send file data
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void handle_view(SOCKET client_socket, const char *client_folder) {
    char buffer[BUFFER_SIZE];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    SYSTEMTIME st;
    FILETIME ftLocal;

    // Create search pattern
    char search_path[BUFFER_SIZE];
    snprintf(search_path, sizeof(search_path), "%s/*", client_folder);

    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        send(client_socket, "$ERROR$No_Files_Found$", 22, 0);
        return;
    }

    // Send file details
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            FileTimeToLocalFileTime(&find_data.ftLastWriteTime, &ftLocal);
            FileTimeToSystemTime(&ftLocal, &st);

            char file_info[BUFFER_SIZE];
            __int64 file_size = ((long long)find_data.nFileSizeHigh << 32) | find_data.nFileSizeLow;
            snprintf(file_info, sizeof(file_info),
                     "Date Modified: %02d-%02d-%04d  Time: %02d:%02d  File Name: %s  Size: %lld bytes\n",
                     st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute,
                     find_data.cFileName, file_size);

            send(client_socket, file_info, strlen(file_info), 0);
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
    send(client_socket, "$END$", 5, 0);  // Indicate the end of the file listing
}

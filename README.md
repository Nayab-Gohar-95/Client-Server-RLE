# Client-Server File Transfer with RLE Compression

## Overview
This project implements a **multi-client file transfer system** using **Run-Length Encoding (RLE) compression** to optimize file size during transmission. The system allows clients to **upload** and **download** files through a central server while maintaining individual client directories.

## Features
- **Multi-Client Support:** Multiple clients can connect to the server and perform file transfers.
- **RLE Compression:** Files are compressed using Run-Length Encoding before upload.
- **RLE Decompression:** Files are decompressed when downloaded.
- **Client-Specific Directories:** Each client has its own folder on the server.
- **File Upload & Download:** Clients can send files to the server or retrieve files from any client folder.
- **Efficient Data Transfer:** Reduced file sizes using compression improves network efficiency.

## How RLE Compression Works
Run-Length Encoding (RLE) is a lossless compression technique that replaces consecutive repeated characters with a count and the character itself.

Example:
```
Original:    AAAABBBCCDAA
Compressed:  4A3B2C1D2A
```

### **Compression on Upload**
When a client uploads a file, the content is **compressed using RLE** before being sent to the server. This reduces the file size, optimizing bandwidth usage.

### **Decompression on Download**
When a client downloads a file, the **server sends the compressed version**, and the client **decompresses it** before saving it.

## Project Structure
```
/Client-Server-RLE
  /client
    - client.c   # Handles client-side operations (upload/download with RLE processing)
  /server
    - server.c   # Manages client requests, stores files, and handles RLE compression
```

## Compilation & Execution
### **1. Compile the Server and Client**
#### **On Linux**:
```sh
gcc server.c -o server -lpthread
gcc client.c -o client
```
#### **On Windows**:
```sh
gcc server.c -o server.exe -lws2_32
gcc client.c -o client.exe -lws2_32
```

### **2. Start the Server**
Run the server first to listen for incoming client connections.
```sh
./server
```

### **3. Run Clients**
Each client should be executed separately.
```sh
./client
```

## Example Usage
1. **Uploading a file from Client 1**:
   - Client reads a file.
   - Applies **RLE compression**.
   - Sends the compressed file to the server.

2. **Downloading a file to Client 2**:
   - Client requests a file from the server.
   - Receives the **compressed** version.
   - Decompresses it before saving.

## Future Improvements
🚀 **Support for Additional Compression Algorithms** (e.g., Huffman Encoding)
🚀 **File Integrity Checks** (Hash-based verification)
🚀 **User Authentication & Access Control**



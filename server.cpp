#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024
#define MAX_USERNAME_LENGTH 8
namespace fs = std::filesystem;

class MailServer {
private:
    SOCKET server_fd;
    int port;
    std::string spool_directory;

    bool validateUsername(const std::string& username) {
        std::cout << "Debug - Validating username: '" << username << "'" << std::endl;
        std::cout << "Debug - Username length: " << username.length() << std::endl;

        // Print each character and its ASCII value
        for (char c : username) {
            std::cout << "Debug - Character: '" << c << "' ASCII: " << (int)c << std::endl;
        }

        if (username.length() > MAX_USERNAME_LENGTH || username.empty()) {
            std::cout << "Debug - Invalid length" << std::endl;
            return false;
        }

        bool valid = std::all_of(username.begin(), username.end(),
            [](char c) { return std::isalnum(c) && std::islower(c); });

        std::cout << "Debug - Characters valid: " << (valid ? "yes" : "no") << std::endl;
        return valid;
    }

    std::string readMessage(SOCKET client_socket) {
        std::string message;
        char buffer[BUFFER_SIZE];
        std::cout << "Debug - Starting to read message" << std::endl;
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            std::cout << "Debug - Received bytes: " << bytes_read << std::endl;
            std::cout << "Debug - Buffer content: '" << buffer << "'" << std::endl;
            if (bytes_read <= 0) return "";

            message += buffer;
            std::cout << "Debug - Current message: '" << message << "'" << std::endl;
            if (message.find("\n.\n") != std::string::npos) {
                // Extract the content before \n.\n
                size_t pos = message.find("\n.\n");
                message = message.substr(0, pos);
                std::cout << "Debug - Final extracted message: '" << message << "'" << std::endl;
                break;
            }
        }
        return message;
    }


    void handleSend(SOCKET client_socket) {
        std::string sender, receiver, subject, message;

        sender = readMessage(client_socket);
        std::cout << "Debug - Received sender: '" << sender << "'" << std::endl;
        if (!validateUsername(sender)) {
            std::cout << "Debug - Invalid sender username" << std::endl;
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        receiver = readMessage(client_socket);
        std::cout << "Debug - Received receiver: '" << receiver << "'" << std::endl;
        if (!validateUsername(receiver)) {
            std::cout << "Debug - Invalid receiver username" << std::endl;
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        subject = readMessage(client_socket);
        std::cout << "Debug - Received subject: '" << subject << "'" << std::endl;
        message = readMessage(client_socket);
        std::cout << "Debug - Received message: '" << message << "'" << std::endl;

        try {
            std::string user_dir = spool_directory + "\\" + receiver;
            fs::create_directories(user_dir);

            std::string filename = user_dir + "\\" +
                std::to_string(std::time(nullptr)) + ".mail";
            std::ofstream file(filename);
            file << "From: " << sender << "\n";
            file << "Subject: " << subject << "\n\n";
            file << message;
            file.close();

            send(client_socket, "OK\n", 3, 0);
        } catch (const std::exception& e) {
            std::cout << "Debug - Error saving message: " << e.what() << std::endl;
            send(client_socket, "ERR\n", 4, 0);
        }
    }

    void handleList(SOCKET client_socket) {
        std::string username = readMessage(client_socket);
        if (!validateUsername(username)) {
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        std::string user_dir = spool_directory + "\\" + username;
        if (!fs::exists(user_dir)) {
            send(client_socket, "0\n", 2, 0);
            return;
        }

        std::vector<std::string> messages;
        for (const auto& entry : fs::directory_iterator(user_dir)) {
            std::ifstream file(entry.path());
            std::string sender, subject;

            std::getline(file, sender);
            std::getline(file, subject);

            messages.push_back(entry.path().filename().string() + " " +
                             sender + " " + subject + "\n");
        }

        std::string response = std::to_string(messages.size()) + "\n";
        for (const auto& msg : messages) {
            response += msg;
        }
        send(client_socket, response.c_str(), response.length(), 0);
    }

    void handleRead(SOCKET client_socket) {
        std::string username = readMessage(client_socket);
        std::string message_id = readMessage(client_socket);

        if (!validateUsername(username)) {
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        std::string filepath = spool_directory + "\\" + username + "\\" + message_id + ".mail";
        if (!fs::exists(filepath)) {
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        std::ifstream file(filepath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string message = "OK\n" + buffer.str() + "\n";
        send(client_socket, message.c_str(), message.length(), 0);
    }

    void handleDelete(SOCKET client_socket) {
        std::string username = readMessage(client_socket);
        std::string message_id = readMessage(client_socket);

        if (!validateUsername(username)) {
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        std::string filepath = spool_directory + "\\" + username + "\\" + message_id + ".mail";
        if (!fs::exists(filepath)) {
            send(client_socket, "ERR\n", 4, 0);
            return;
        }

        fs::remove(filepath);
        send(client_socket, "OK\n", 3, 0);
    }

public:
    MailServer(int port, const std::string& spool_dir)
        : port(port), spool_directory(spool_dir) {
        fs::create_directories(spool_directory);
    }

    void start() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed");
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
            closesocket(server_fd);
            WSACleanup();
            throw std::runtime_error("Bind failed");
        }

        if (listen(server_fd, 3) == SOCKET_ERROR) {
            closesocket(server_fd);
            WSACleanup();
            throw std::runtime_error("Listen failed");
        }

        std::cout << "Server listening on port " << port << std::endl;

        while (true) {
            SOCKET client_socket = accept(server_fd, nullptr, nullptr);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }

            handleClient(client_socket);
            closesocket(client_socket);
        }
    }

    void handleClient(SOCKET client_socket) {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_read <= 0) break;

            std::string command(buffer);
            if (command.find("SEND") == 0) {
                handleSend(client_socket);
            } else if (command.find("LIST") == 0) {
                handleList(client_socket);
            } else if (command.find("READ") == 0) {
                handleRead(client_socket);
            } else if (command.find("DEL") == 0) {
                handleDelete(client_socket);
            } else if (command.find("QUIT") == 0) {
                break;
            }
        }
    }

    ~MailServer() {
        closesocket(server_fd);
        WSACleanup();
    }
};
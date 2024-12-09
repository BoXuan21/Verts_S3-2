#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <regex>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

class MailClient {
private:
    SOCKET sock;
    std::string server_ip;
    int server_port;

    void sendMessage(const std::string& message) {
        send(sock, message.c_str(), message.length(), 0);
    }

    std::string readResponse() {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, buffer, BUFFER_SIZE - 1, 0);
        return std::string(buffer);
    }

    void handleSend() {
        sendMessage("SEND\n");

        std::string sender, receiver, subject, message;

        std::cout << "Sender: ";
        std::getline(std::cin, sender);
        // Trim whitespace
        sender = std::regex_replace(sender, std::regex("^\\s+|\\s+$"), "");
        std::cout << "Debug - Sending sender: '" << sender + "\n.\n" << "'" << std::endl;
        sendMessage(sender + "\n.\n");

        std::cout << "Receiver: ";
        std::getline(std::cin, receiver);
        receiver = std::regex_replace(receiver, std::regex("^\\s+|\\s+$"), "");
        std::cout << "Debug - Sending receiver: '" << receiver + "\n.\n" << "'" << std::endl;
        sendMessage(receiver + "\n.\n");

        std::cout << "Subject: ";
        std::getline(std::cin, subject);
        subject = std::regex_replace(subject, std::regex("^\\s+|\\s+$"), "");
        std::cout << "Debug - Sending subject: '" << subject + "\n.\n" << "'" << std::endl;
        sendMessage(subject + "\n.\n");

        std::cout << "Message (end with a line containing only '.'): " << std::endl;
        std::string line;
        while (std::getline(std::cin, line)) {
            line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
            if (line == ".") {
                break;
            }
            message += line + "\n";
        }

        // Remove trailing newline without using ends_with
        if (!message.empty() && message.back() == '\n') {
            message.pop_back();
        }

        std::cout << "Debug - Sending message: '" << message + "\n.\n" << "'" << std::endl;
        sendMessage(message + "\n.\n");

        std::string response = readResponse();
        std::cout << "Server response: " << response;
    }

    void handleList() {
        sendMessage("LIST\n");

        std::string username;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        username = std::regex_replace(username, std::regex("^\\s+|\\s+$"), "");
        sendMessage(username + "\n.\n");

        std::string response = readResponse();
        std::cout << "Messages:\n" << response;
    }

    void handleRead() {
        sendMessage("READ\n");

        std::string username, message_id;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        username = std::regex_replace(username, std::regex("^\\s+|\\s+$"), "");
        sendMessage(username + "\n.\n");

        std::cout << "Message ID: ";
        std::getline(std::cin, message_id);
        message_id = std::regex_replace(message_id, std::regex("^\\s+|\\s+$"), "");
        sendMessage(message_id + "\n.\n");

        std::string response = readResponse();
        std::cout << "Message:\n" << response;
    }

    void handleDelete() {
        sendMessage("DEL\n");

        std::string username, message_id;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        username = std::regex_replace(username, std::regex("^\\s+|\\s+$"), "");
        sendMessage(username + "\n.\n");

        std::cout << "Message ID: ";
        std::getline(std::cin, message_id);
        message_id = std::regex_replace(message_id, std::regex("^\\s+|\\s+$"), "");
        sendMessage(message_id + "\n.\n");

        std::string response = readResponse();
        std::cout << "Server response: " << response;
    }

    void sendCommand() {
        std::string input;
        std::cout << "Enter command (SEND/LIST/READ/DEL/QUIT): ";
        std::getline(std::cin, input);

        if (input == "SEND") {
            handleSend();
        } else if (input == "LIST") {
            handleList();
        } else if (input == "READ") {
            handleRead();
        } else if (input == "DEL") {
            handleDelete();
        } else if (input == "QUIT") {
            sendMessage(input + "\n");
            throw std::runtime_error("Quit requested");
        } else {
            std::cout << "Invalid command" << std::endl;
        }
    }

public:
    MailClient(const std::string& ip, int port)
        : server_ip(ip), server_port(port) {}

    void connect() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed");
        }

        sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
            closesocket(sock);
            WSACleanup();
            throw std::runtime_error("Invalid address");
        }

        if (::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            closesocket(sock);
            WSACleanup();
            throw std::runtime_error("Connection failed");
        }

        std::cout << "Connected to server" << std::endl;
    }

    void start() {
        try {
            while (true) {
                sendCommand();
            }
        } catch (const std::runtime_error& e) {
            if (std::string(e.what()) != "Quit requested") {
                throw;
            }
        }
    }

    ~MailClient() {
        closesocket(sock);
        WSACleanup();
    }
};
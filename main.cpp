#ifdef SERVER_MODE
#include "server.cpp"
#endif

#ifdef CLIENT_MODE
#include "client.cpp"
#endif

#include <iostream>
#include <string>

void print_usage_server() {
    std::cout << "Usage: ./twmailer-server <port> <mail-spool-directory>" << std::endl;
}

void print_usage_client() {
    std::cout << "Usage: ./twmailer-client <ip> <port>" << std::endl;
}

#ifdef SERVER_MODE
int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage_server();
        return 1;
    }

    try {
        int port = std::stoi(argv[1]);
        std::string spool_dir = argv[2];

        MailServer server(port, spool_dir);
        std::cout << "Starting server on port " << port << " with spool directory: " << spool_dir << std::endl;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif

#ifdef CLIENT_MODE
int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage_client();
        return 1;
    }

    try {
        std::string ip = argv[1];
        int port = std::stoi(argv[2]);

        MailClient client(ip, port);
        std::cout << "Connecting to server at " << ip << ":" << port << std::endl;
        client.connect();
        client.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif
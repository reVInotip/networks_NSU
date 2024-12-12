#include "socks_server.h"
#include "unistd.h"
#include <iostream>

using namespace socks_server;

int getport(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                return std::atoi(optarg);
            default:
                return -1;
        }
    }

    return -1;
}

int main(int argc, char **argv) {
    int port = getport(argc, argv);
    if (port < 0) {
        std::cerr << "Invlid port parameter. Use default port\n";
        port = 12345;
    }

    try {
        SocksServer server {port};
        std::cout << "Create SOCKS server" << std::endl;
        server.accept_connections();
    } catch (SockException &e) {
        std::cerr << "Can not create SOCKS server: " << e.what() << std::endl;
    } catch (std::bad_alloc &e) {
        std::cerr << "Can not create SOCKS server: " << e.what() << std::endl;
    }

    return 0;
}
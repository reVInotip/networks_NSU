#include <iostream>
#include "tcp_client.h"
#include "tcp_server.h"

using std::string;
using namespace client;
using namespace server;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Low count arguments\n";
        return 1;
    }
    string version {argv[1]};

    try {
        if (version == "server") {
            Server server {std::atoi(argv[2])};
            server.accept_connections();
        } else if (version == "client") {
            Client client {argv[2], argv[3], std::atoi(argv[4])};
            client.send_file_to_server();
        }
    } catch (string &e) {
        std::cout << e << std::endl;
    }

    return 0;
} 
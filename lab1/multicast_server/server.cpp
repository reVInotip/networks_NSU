#include "server.h"
#include "udp_socket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <exception>
#include <random>
#include <netdb.h>

using namespace server;
using namespace udp_socket;


unsigned randuint() {
    std::random_device rd;
    std::mt19937 gen {rd()};
    std::uniform_int_distribution<> distrib {1, 100000};

    return distrib(gen);
}

Server::Server(const string &ipaddr):
    socket_(ipaddr), server_id_(randuint())
{
    try {
        socket_.create_socket();
        socket_.server_bind();
    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}

Server::~Server() {
    std::cout << "Server destroyed" << std::endl;
    socket_.close_socket();
}

void Server::send_to_copies() {
    std::cout << "Server start to send message\n";
    try {
        while (true) {
            try {
                socket_.send_to<unsigned>(server_id_);
            } catch (udp_socket::TryAgainException&) {
                continue;
            }
        }
    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}
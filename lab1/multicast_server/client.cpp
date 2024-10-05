#include "client.h"
#include "server.h"
#include "udp_socket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <exception>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <netdb.h>

using namespace client;
using namespace udp_socket;

Client::Client(const string &ipaddr, const server::Server &server):
    socket_(ipaddr), number_of_connections_(0), connected_server_id_(server.server_id_)
{   
    try {
        socket_.create_socket();
        socket_.multi_bind();
    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}

Client::~Client() {
    std::cout << "Client destroyed" << std::endl;
    try {
        socket_.close_socket();
    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}

void Client::recv_from_copies() {
    string server_name, server_ip;
    int server_port;
    unsigned buffer;

    std::cout << "Start to recive messages\n";

    try {
        auto start = std::chrono::high_resolution_clock::now();
        while (true) {
            auto now = std::chrono::high_resolution_clock::now();
            for (auto it = connection_data_.begin(); it != connection_data_.end();) {
                const std::chrono::duration<double, std::milli> elapsed =
                    now - (it->second).second;
                if (elapsed > local_timeout_) {
                    it = connection_data_.erase(it);
                    --number_of_connections_;
                } else {
                    ++it;
                }
            }

            try {
                buffer = socket_.receive_from<unsigned>(server_ip, server_port);
            } catch (TryAgainException&) {
                continue;
            }

            if (buffer != connected_server_id_) {
                server_name = server_ip + " | " + std::to_string(server_port);
                
                if (connection_data_.count(server_name) <= 0)
                    ++number_of_connections_;
                ++connection_data_[server_name].first;
                connection_data_[server_name].second = std::chrono::high_resolution_clock::now();
            }

            const std::chrono::duration<double, std::milli> elapsed =
                    std::chrono::high_resolution_clock::now() - start;

            if (connection_data_.size() > 0 && elapsed > timeout_) {
                std::cout << "=================================\n";
                std::cout << "Count of connections: " << number_of_connections_ << std::endl;

                for (auto& [key, value] : connection_data_) {
                    std::cout << "Server: " << key << " send " << value.first << " messages per second" << std::endl;
                    value.first = 0;
                }

                std::cout << "=================================\n";

                start = std::chrono::high_resolution_clock::now();
            }
        }
    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}

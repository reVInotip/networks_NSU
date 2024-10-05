#include "tcp_server.h"
#include <experimental/filesystem>
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
#include <fstream>
#include <thread>
#include <chrono>

using namespace server;
using std::experimental::filesystem::exists, std::experimental::filesystem::create_directory;

Server::Server(const int port, const int buffer_size):  buffer_size_(buffer_size), connection_request_port_(port) {
    connection_request_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_request_sockfd_ < 0)
        throw std::string {strerror(errno)};

    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(connection_request_port_);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("10.0.0.1");

    if (
        bind(connection_request_sockfd_,
            (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw std::string {strerror(errno)};

    if (listen(connection_request_sockfd_, 10) < 0)
        throw std::string {strerror(errno)};

    if (!exists(workdir_)) {
        create_directory(workdir_);
    }

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_sockaddr.sin_addr), str, INET_ADDRSTRLEN);

    std::cout << str << std::endl;
}

Server::~Server() {
    close(connection_request_sockfd_);
}

void Server::client_thread_routine(int client_fd) const {
    std::ofstream out;
    string filename, size_str;
    int file_size = 0, curr_size = 0;
    char *buffer = new char[buffer_size_];
    string buffer_str;
    int count_recv_bytes, count_send_bytes;
    bool is_name = false, is_size = false, is_data = false;
    string server_answer;

    size_t index;
    double middle_speed = 0;
    unsigned count_recv_buffers = 0;
    auto global_start = std::chrono::high_resolution_clock::now();
    try {
        while (true) {
            memset(buffer, 0, buffer_size_);

            auto start = std::chrono::high_resolution_clock::now();
            count_recv_bytes = recv(client_fd, buffer, buffer_size_, 0);
            if (count_recv_bytes < 0)
                throw std::string {strerror(errno)};
            const std::chrono::duration<double, std::milli> elapsed =
                        std::chrono::high_resolution_clock::now() - start;
            
            const std::chrono::duration<double, std::milli> global_elapsed =
                        std::chrono::high_resolution_clock::now() - global_start;
            
            double speed = count_recv_bytes / elapsed.count();
            middle_speed += speed;
            ++count_recv_buffers;

            if (elapsed >= timeout_ || count_recv_buffers == 1) {
                std::cout << "Current speed (Kb): " << speed << " middle speed (Kb): " << middle_speed / count_recv_buffers << std::endl;
                global_start = std::chrono::high_resolution_clock::now();
            }
            
            buffer_str = buffer;
            
            if (!is_name) {
                if ((index = buffer_str.find("_$@&name?_")) == std::string::npos) {
                    filename += buffer_str;
                    continue;
                } else {
                    filename += buffer_str.substr(0, index);
                    buffer_str = index + 10 > buffer_str.size() ? "" : buffer_str.substr(index + 10);
                    is_name = true;

                    out.open(workdir_ + "/" + filename); // if it already exists?
                    if (!out.is_open())
                        throw "Can not open file";
                }
            }

            if (!is_size) {
                if ((index = buffer_str.find("_$@&size?_")) == std::string::npos) {
                    size_str += buffer_str;
                    continue;
                } else {
                    size_str += buffer_str.substr(0, index);
                    file_size = std::stoi(size_str);
                    buffer_str = index + 10 > buffer_str.size() ? "" : buffer_str.substr(index + 10);
                    is_size = true;
                }
            }

            if (!is_data) {
                if ((index = buffer_str.find("_$@&data?_")) == std::string::npos) {
                    out << buffer_str;
                    curr_size += buffer_str.size();
                    continue;
                } else {
                    buffer_str = buffer_str.substr(0, index);
                    out << buffer_str;
                    curr_size += buffer_str.size();
                    out.close();

                    server_answer = curr_size != file_size ? "fail!" : "success!";
                    count_send_bytes = send(client_fd, server_answer.c_str(), server_answer.size(), 0);
                    if (count_send_bytes < 0)
                        throw std::string {strerror(errno)};

                    close(client_fd);
                    break;
                }
            }
        }
    } catch (string &e) {
        std::cout << e << std::endl;
    }
}

void Server::accept_connections() const {
    int client_fd;
    while (true) {
        client_fd = accept(connection_request_sockfd_, NULL, NULL);
        if (client_fd < 0)
            throw std::string {strerror(errno)};
        
        std::thread t {&Server::client_thread_routine, this, client_fd};
        t.detach();
    }
}
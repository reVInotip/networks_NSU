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
#include <memory.h>

using namespace server;
using std::experimental::filesystem::exists, std::experimental::filesystem::create_directory;

Server::Server(const int port, const int buffer_size):  buffer_size_(buffer_size), connection_request_port_(port) {
    connection_request_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_request_sockfd_ < 0)
        throw OpenSocketException {strerror(errno), errno};

    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(connection_request_port_);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (
        bind(connection_request_sockfd_,
            (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw BindFailedException {strerror(errno), errno};

    if (listen(connection_request_sockfd_, 10) < 0)
        throw SetListenFailedException {strerror(errno), errno};

    if (!exists(workdir_)) {
        create_directory(workdir_);
    }

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_sockaddr.sin_addr), str, INET_ADDRSTRLEN);
}

Server::~Server() {
    close(connection_request_sockfd_);
}

string Server::get_filename_from_stream(int client_fd, string &end) const {
    size_t index;
    int count_recv_bytes;
    string filename;
    std::unique_ptr<char[]> buffer {new char[buffer_size_]};
    while (true) {
        memset(buffer.get(), 0, buffer_size_);

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};

        string buffer_str {buffer.get(), static_cast<unsigned long>(count_recv_bytes)};
        
        if ((index = buffer_str.find("_$@&name?_")) == std::string::npos) {
            filename += buffer_str;
        } else {
            filename += buffer_str.substr(0, index);
            end = index + 10 > buffer_str.size() ? "" : buffer_str.substr(index + 10);
            break;
        }
    }

    return filename;
}

double Server::get_filesize_from_stream(int client_fd, string init, string &end) const {
    size_t index;
    int count_recv_bytes;
    double file_size;
    string buffer_str = init;
    std::unique_ptr<char[]> buffer {new char[buffer_size_]};
    while (true) {
        if ((index = buffer_str.find("_$@&size?_")) != std::string::npos) {
            file_size = std::stod(buffer_str.substr(0, index));
            end = index + 10 > buffer_str.size() ? "" : buffer_str.substr(index + 10);
            break;
        }

        memset(buffer.get(), 0, buffer_size_);

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};

        buffer_str += string {buffer.get(), static_cast<unsigned long>(count_recv_bytes)};
    }

    return file_size;
}

bool Server::save_data_from_stream_to_file(int client_fd, string path_to_file, string init, double expected_file_size) const {
    std::ofstream out {path_to_file};
    if (!out.is_open())
        throw OpenFileException {};
    
    size_t index;
    int count_recv_bytes;
    double curr_size = 0, total_recieved_bytes_count = 0;
    bool is_first = true;

    string buffer_str = init;
    std::unique_ptr<char[]> buffer {new char[buffer_size_]};
    auto global_start = std::chrono::high_resolution_clock::now();
    auto timer_start = std::chrono::high_resolution_clock::now();
    while (true) {
        memset(buffer.get(), 0, buffer_size_);

        auto start = std::chrono::high_resolution_clock::now();

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};

        buffer_str += string {buffer.get(), static_cast<unsigned long>(count_recv_bytes)};

        const std::chrono::duration<double, std::milli> elapsed =
                        std::chrono::high_resolution_clock::now() - start;
            
        const std::chrono::duration<double, std::milli> global_elapsed =
                    std::chrono::high_resolution_clock::now() - global_start;

        const std::chrono::duration<double, std::milli> time_elapsed =
                    std::chrono::high_resolution_clock::now() - timer_start;
        
        double speed = count_recv_bytes / elapsed.count();
        total_recieved_bytes_count += count_recv_bytes;

        if (global_elapsed >= timeout_ || is_first) {
            std::cout << "=======================================" << std::endl;
            std::cout << path_to_file << ": Current speed (Kb): " << speed << std::endl;
            std::cout << path_to_file << ": Middle speed (Kb): " << total_recieved_bytes_count / time_elapsed.count() << std::endl;
            std::cout << path_to_file << ": Recieved " << (curr_size / expected_file_size)  * 100.0 << "%" << std::endl;
            std::cout << path_to_file << ": " << time_elapsed.count() / 1000.0 << " seconds have passed" << std::endl;
            std::cout << "=======================================" << std::endl;
            global_start = std::chrono::high_resolution_clock::now();
            is_first = false;
        }

        if ((index = buffer_str.find("_$@&data?_")) == std::string::npos) {
            out.write(buffer_str.c_str(), buffer_str.size());
            curr_size += buffer_str.size() / 1e6;
            buffer_str.clear();
        } else {
            buffer_str = buffer_str.substr(0, index);
            out.write(buffer_str.c_str(), buffer_str.size());
            curr_size += buffer_str.size() / 1e6;
            break;
        }
    }

    bool is_transfer_success = fabs(curr_size - expected_file_size) < 1e6;

    const std::chrono::duration<double, std::milli> time_elapsed =
                    std::chrono::high_resolution_clock::now() - timer_start;
    std::cout << "=======================================" << std::endl;
    std::cout << path_to_file << ": Middle speed (Kb): " << total_recieved_bytes_count / time_elapsed.count() << std::endl;
    std::cout << path_to_file << ": Recieved " << (curr_size / expected_file_size)  * 100.0 << "%" << std::endl;
    std::cout << path_to_file << ": " << time_elapsed.count() / 1000.0 << " seconds have passed" << std::endl;
    std::cout << (is_transfer_success ? "FILE RECIEVED SUCCESSFULLY" : "SOME ERROR OCCURED") << std::endl;
    std::cout << "=======================================" << std::endl;
    global_start = std::chrono::high_resolution_clock::now();

    out.flush();
    out.close();

    return fabs(curr_size - expected_file_size) < 1e6;
}

string Server::make_output_file_name(const string &init) const {
    string filename = init == "" ? workdir_ + "/out.txt" : workdir_ + "/" + init;
    string base = filename;
    std::cout << filename << std::endl;
    for (int i = 1; exists(filename); ++i) {
        filename = base + "_(" + std::to_string(i) + ")";
    }

    return filename;
}

void Server::client_thread_routine(int client_fd) const {
    try {
        string end;
        string filename = get_filename_from_stream(client_fd, end);
        filename = make_output_file_name(filename);

        std::cout << "Start receive file with name " << filename << std::endl;

        double file_size = get_filesize_from_stream(client_fd, end, end);
        std::cout << "Expected size of file \"" << filename << "\": " << file_size << " Kb" << std::endl;

        string server_answer = save_data_from_stream_to_file(client_fd, filename, end, file_size) ? "fail!" : "success!";
        int count_send_bytes = send(client_fd, server_answer.c_str(), server_answer.size(), 0);
        if (count_send_bytes < 0)
            throw SendFailedException {strerror(errno), errno};

        close(client_fd);
    } catch (string &e) {
        std::cout << e << std::endl;
    }
}

void Server::accept_connections() const {
    std::cout << "TCP server start" << std::endl;

    int client_fd;
    while (true) {
        client_fd = accept(connection_request_sockfd_, NULL, NULL);
        if (client_fd < 0)
            throw AcceptFailedException {strerror(errno), errno};
        
        std::thread t {&Server::client_thread_routine, this, client_fd};
        t.detach();
    }
}
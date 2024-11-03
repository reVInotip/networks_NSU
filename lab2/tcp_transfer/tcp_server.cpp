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

Server::Printer::Printer(): timeout_(3000),
                            total_recieved_bytes_count_(0),
                            is_first_print1_(true),
                            is_first_print2_(true)
{
    auto now = std::chrono::high_resolution_clock::now();
    global_start_ = now;
    timer_start_ = now;
}

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

void Server::Printer::print_recv_info
(
    int count_recv_bytes,
    std::chrono::_V2::system_clock::time_point &recv_start,
    string &filename
) {
    const std::chrono::duration<double, std::milli> global_elapsed =
                std::chrono::high_resolution_clock::now() - global_start_;

    if (global_elapsed < timeout_ && !is_first_print1_) {
        total_recieved_bytes_count_ += count_recv_bytes;
        return;
    }

    const std::chrono::duration<double, std::milli> elapsed =
                        std::chrono::high_resolution_clock::now() - recv_start;

    const std::chrono::duration<double, std::milli> time_elapsed =
                std::chrono::high_resolution_clock::now() - timer_start_;
    
    double speed = count_recv_bytes / elapsed.count();
    total_recieved_bytes_count_ += count_recv_bytes;
        
    std::cout << "=======================================" << std::endl;
    std::cout << filename << ": Current speed (Kb): " << speed << std::endl;
    std::cout << filename << ": Middle speed (Kb): " << total_recieved_bytes_count_ / time_elapsed.count() << std::endl;
    std::cout << filename << ": " << time_elapsed.count() / 1000.0 << " seconds have passed" << std::endl;
    std::cout << "=======================================" << std::endl;
    global_start_ = std::chrono::high_resolution_clock::now();
    is_first_print1_ = false;
}

void Server::Printer::print_recv_info_with_percents
(  
    int count_recv_bytes,
    std::chrono::_V2::system_clock::time_point &recv_start,
    string &filename,
    double curr_size,
    double expected_file_size
) {
    const std::chrono::duration<double, std::milli> global_elapsed =
                std::chrono::high_resolution_clock::now() - global_start_;

    if (global_elapsed < timeout_ && !is_first_print2_) {
        total_recieved_bytes_count_ += count_recv_bytes;
        return;
    }
    
    const std::chrono::duration<double, std::milli> elapsed =
                        std::chrono::high_resolution_clock::now() - recv_start;

    const std::chrono::duration<double, std::milli> time_elapsed =
                std::chrono::high_resolution_clock::now() - timer_start_;
    
    double speed = count_recv_bytes / elapsed.count();
    total_recieved_bytes_count_ += count_recv_bytes;
        
    std::cout << "=======================================" << std::endl;
    std::cout << filename << ": Current speed (Kb): " << speed << std::endl;
    std::cout << filename << ": Middle speed (Kb): " << total_recieved_bytes_count_ / time_elapsed.count() << std::endl;
    std::cout << filename << ": Recieved " << (curr_size / expected_file_size)  * 100.0 << "%" << std::endl;
    std::cout << filename << ": " << time_elapsed.count() / 1000.0 << " seconds have passed" << std::endl;
    std::cout << "=======================================" << std::endl;
    global_start_ = std::chrono::high_resolution_clock::now();
    is_first_print2_ = false;
}

void Server::Printer::print_recv_end(bool is_transfer_success) {
    std::cout << "=======================================" << std::endl;
    std::cout << (is_transfer_success ? "FILE RECIEVED SUCCESSFULLY" : "SOME ERROR OCCURED") << std::endl;
    std::cout << "=======================================" << std::endl;
}

string Server::get_filename_from_stream(int client_fd, string &end, Printer &printer) const {
    size_t index;
    int count_recv_bytes;
    string filename;
    std::unique_ptr<char[]> buffer {new char[buffer_size_]};

    string curr_filename = "[???]";
    while (true) {
        memset(buffer.get(), 0, buffer_size_);

        auto start = std::chrono::high_resolution_clock::now();

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};
            
        printer.print_recv_info(count_recv_bytes, start, curr_filename);

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

double Server::get_filesize_from_stream(int client_fd, string &filename, string init, string &end, Printer &printer) const {
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

        auto start = std::chrono::high_resolution_clock::now();

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};
        
        printer.print_recv_info(count_recv_bytes, start, filename);

        buffer_str += string {buffer.get(), static_cast<unsigned long>(count_recv_bytes)};
    }

    return file_size;
}

bool Server::save_data_from_stream_to_file(int client_fd, string path_to_file, string init, double expected_file_size, Printer &printer) const {
    std::ofstream out {path_to_file};
    if (!out.is_open())
        throw OpenFileException {};
    
    size_t index;
    int count_recv_bytes;
    double curr_size = 0;

    string buffer_str = init;
    std::unique_ptr<char[]> buffer {new char[buffer_size_]};

    auto now = std::chrono::high_resolution_clock::now();

    if ((index = buffer_str.find("_$@&data?_")) != std::string::npos) {
        buffer_str = buffer_str.substr(0, index);
        out.write(buffer_str.c_str(), buffer_str.size());
        curr_size += buffer_str.size() / 1e6;

        printer.print_recv_info_with_percents(buffer_str.size(), now, path_to_file, curr_size, expected_file_size);
        goto EXIT;
    }
    while (true) {
        memset(buffer.get(), 0, buffer_size_);

        auto start = std::chrono::high_resolution_clock::now();

        count_recv_bytes = recv(client_fd, buffer.get(), buffer_size_, 0);
        if (count_recv_bytes < 0)
            throw RecvFailedException {strerror(errno), errno};
        
        printer.print_recv_info_with_percents(count_recv_bytes, start, path_to_file, curr_size, expected_file_size);

        buffer_str += string {buffer.get(), static_cast<unsigned long>(count_recv_bytes)};

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

EXIT:
    bool is_transfer_success = fabs(curr_size - expected_file_size) < 1e6;

    out.flush();
    out.close();

    return is_transfer_success;
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
        Printer printer {};

        string end;
        string filename = get_filename_from_stream(client_fd, end, printer);
        filename = make_output_file_name(filename);

        std::cout << "Start receive file with name " << filename << std::endl;

        double file_size = get_filesize_from_stream(client_fd, filename, end, end, printer);
        std::cout << "Expected size of file \"" << filename << "\": " << file_size << " Kb" << std::endl;

        bool is_success = save_data_from_stream_to_file(client_fd, filename, end, file_size, printer);
        printer.print_recv_end(is_success);

        string server_answer = is_success ? "fail!" : "success!";
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
#include "tcp_client.h"
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

using namespace client;
using std::experimental::filesystem::exists, std::experimental::filesystem::file_size;

Client::Client(const string &path_to_file, const string &server_ipaddr, const int server_port):
    path_to_file_(path_to_file)
{
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0)
        throw std::string {strerror(errno)};
    
    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(server_port);
    server_sockaddr.sin_addr.s_addr = inet_addr(server_ipaddr.c_str());

    std::cout << path_to_file << std::endl;

    if (
        connect(sockfd_,
                (const sockaddr *) &server_sockaddr,
                (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw std::string {strerror(errno)};

    if (!exists(path_to_file_)) {
        throw "File does not exists";
    }
}

Client::~Client() {
    close(sockfd_);
}

string Client::get_file_name() const {
    const auto pos = path_to_file_.find_last_of('/');
    return path_to_file_.substr(pos + 1);
}

void Client::send_file_to_server() const {
    std::ifstream in;
    std::size_t count_send_bytes;

    in.open(path_to_file_);
    if (!in.is_open())
        throw "Can not open the file";

    string filename = get_file_name() + "_$@&name?_";
    count_send_bytes = send(sockfd_, filename.c_str(), filename.size(), 0);
    if (count_send_bytes < filename.size())
        throw "asas";
    
    const string size = std::to_string(file_size(path_to_file_)) + "_$@&size?_";
    count_send_bytes = send(sockfd_, size.c_str(), size.size(), 0);
    if (count_send_bytes < size.size())
        throw "asas";
    
    string line;
    while (std::getline(in, line)) {
        count_send_bytes = send(sockfd_, line.c_str(), line.size(), 0);
        if (count_send_bytes < line.size())
            throw "asas";
    }

    string buffer = "_$@&data?_";
    count_send_bytes = send(sockfd_, buffer.c_str(), buffer.size(), 0);
    if (count_send_bytes < buffer.size())
        throw "asas";
    
    int code;
    if (recv(sockfd_, &code, sizeof(code), 0) < 0)
        throw "asas";
    
    if (code)
        std::cout << "Success" << std::endl;
    else
        std::cout << "Fail" << std::endl;
    
    in.close();
}
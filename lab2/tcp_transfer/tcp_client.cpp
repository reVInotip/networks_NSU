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
#include <memory>

using namespace client;
using std::experimental::filesystem::exists, std::experimental::filesystem::file_size;

Client::Client(const string &path_to_file, const string &server_ipaddr, const int server_port):
    path_to_file_(path_to_file)
{
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0)
        throw OpenSocketException {strerror(errno), errno};
    
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
    ) throw ConnectFailedException {strerror(errno), errno};

    if (!exists(path_to_file_)) {
        throw FileNotExistsException {};
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

    in.open(path_to_file_, std::ios::binary);
    if (!in.is_open())
        throw OpenFileException {};

    string filename = get_file_name() + "_$@&name?_";
    count_send_bytes = send(sockfd_, filename.c_str(), filename.size(), 0);
    if (count_send_bytes < filename.size())
        throw SendFailedException {strerror(errno), errno};
    
    const string size = std::to_string(static_cast<double>(file_size(path_to_file_)) / 1e6) + "_$@&size?_";
    count_send_bytes = send(sockfd_, size.c_str(), size.size(), 0);
    if (count_send_bytes < size.size())
        throw SendFailedException {strerror(errno), errno};

    constexpr size_t data_size  = 1024;
    std::unique_ptr<char[]> data {new char[data_size]};
    while (in) {
        memset(data.get(), 0, data_size);
        in.read(data.get(), data_size);
        count_send_bytes = send(sockfd_, data.get(), in.gcount(), 0);
        if (count_send_bytes < static_cast<size_t>(in.gcount()))
            throw SendFailedException {strerror(errno), errno};
    }

    string buffer = "_$@&data?_";
    count_send_bytes = send(sockfd_, buffer.c_str(), buffer.size(), 0);
    if (count_send_bytes < buffer.size())
        throw SendFailedException {strerror(errno), errno};
    
    int code;
    if (recv(sockfd_, &code, sizeof(code), 0) < 0)
        throw RecvFailedException {strerror(errno), errno};
    
    if (code)
        std::cout << "Success" << std::endl;
    else
        std::cout << "Fail" << std::endl;
    
    in.close();
}
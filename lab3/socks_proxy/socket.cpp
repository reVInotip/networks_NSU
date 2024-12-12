#include "socket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>

using namespace socks_socket;

//==================Socket=====================

Socket::Socket(SockFamily family, SockType type, size_t buffer_capacity):
    family_ {family}, type_ {type}, ip_ {"localhost"}, port_ {0}, buffer_capacity_ {buffer_capacity},
    buffer_ {new unsigned char [buffer_capacity]}
{
    sockfd_ = socket(static_cast<int>(family), static_cast<int>(type), 0);
    if (sockfd_ < 0) throw OpenSocketException {strerror(errno), errno};
}

Socket::Socket(int sockfd, SockFamily family, SockType type, size_t buffer_capacity):
    family_ {family}, type_ {type}, sockfd_ {sockfd}, buffer_capacity_ {buffer_capacity},
    buffer_ {new unsigned char [buffer_capacity]} {}

Socket::~Socket() {
    close(sockfd_);
}

void Socket::bind_to(uint16_t port) {
    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (
        bind(sockfd_,
            (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw BindFailedException {strerror(errno), errno};

    port_ = port;
}

void Socket::bind_to(SockFamily family, const string &ip, uint16_t port) {
    if (family != Socket::SockFamily::IPv4)
        throw BindFailedException {"Now IPv4 version avaliable only", -1};

    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = inet_addr(ip.c_str());
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (
        bind(sockfd_,
            (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw BindFailedException {strerror(errno), errno};

    ip_ = ip;
    port_ = port;
}

void Socket::connect_with(const string &ip, uint16_t port) {
    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (
        connect(sockfd_,
            (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw ConnectFailedException {strerror(errno), errno};
}

Socket *Socket::accept_connection(string &address) const {
    if (type_ != SockType::LISTEN)
        throw AcceptFailedException {"Trying to accept connection on non listen socket", -1};

    struct sockaddr_in cli_addr;
    socklen_t socklen = sizeof(cli_addr);
    char str_addr[INET_ADDRSTRLEN];

    int conn_sock = accept(sockfd_,
            (struct sockaddr *)&cli_addr,
            &socklen);
    
    if (conn_sock < 0) throw AcceptFailedException {strerror(errno), errno};

    inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr),
                   str_addr, sizeof(cli_addr));
    
    address = str_addr + string {":"} + std::to_string(ntohs(cli_addr.sin_port));

    return new Socket {conn_sock, family_, type_, buffer_capacity_};
}

void Socket::make_listen(int count_connections) {
    if (listen(sockfd_, count_connections) < 0)
        throw SetListenFailedException {strerror(errno), errno};
    
    type_ = SockType::LISTEN;
}

void Socket::setoptions(SockOptions options) {
    int prev_options = fcntl(sockfd_, F_GETFL, 0);
    if (prev_options < 0) throw SetoptionsFailedException {strerror(errno), errno};

    if (fcntl(sockfd_, F_SETFL, prev_options | static_cast<int>(options)) < 0)
        throw SetoptionsFailedException {strerror(errno), errno};
}

bool Socket::is_nonblocking() const {
    int options;
    if ((options = fcntl(sockfd_, F_GETFL)) == -1)
        throw SetoptionsFailedException {strerror(errno), errno};
    
    return options & O_NONBLOCK;
}

int Socket::get_fd() const noexcept {
    return sockfd_;
}

uint16_t Socket::get_port() const noexcept {
    return port_;
}

string Socket::get_ip() const noexcept {
    return ip_;
}

int Socket::receive(string &address) {
    struct sockaddr_in src_addr;
    socklen_t socklen = sizeof(src_addr);
    char str_addr[INET_ADDRSTRLEN];

    bzero(buffer_.get(), buffer_capacity_);
    int n = recvfrom(sockfd_, buffer_.get(), buffer_capacity_, 0, (struct sockaddr *) &src_addr, &socklen);
    if (n < 0) {
        if (is_nonblocking() && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOSPC))
            return -1;
        throw ReceiveFailedException {strerror(errno), errno};    
    }

    inet_ntop(AF_INET, (char *)&(src_addr.sin_addr),
                   str_addr, sizeof(src_addr));
    
    address = str_addr + string {":"} + std::to_string(ntohs(src_addr.sin_port));

    buffer_size_ = n;

    return n;
}

int Socket::receive() {
    bzero(buffer_.get(), buffer_capacity_);
    int n = recv(sockfd_, buffer_.get(), buffer_capacity_, 0);
    if (n < 0) {
        if (is_nonblocking() && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOSPC))
            return -1;
        throw ReceiveFailedException {strerror(errno), errno};    
    }

    buffer_size_ = n;

    return n;
}

void Socket::send_to(const void *message, size_t message_size) const {
    if (send(sockfd_, message, message_size, 0) < 0)
        throw SendFailedException {strerror(errno), errno};
}

void Socket::send_to(const string &ip, uint16_t port, const void *message, size_t message_size) const {
    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (
        sendto(sockfd_, message, message_size, 0, (const sockaddr *) &server_sockaddr,
            (socklen_t) sizeof(server_sockaddr)) < 0
    )
        throw SendFailedException {strerror(errno), errno};
}

//==================SocketExceptions=====================

SockException::SockException(const std::string& message, const int err_code) {
    message_ = "Socket error: " + message + " - " + std::to_string(err_code);
}

const char *SockException::what() const noexcept {
    return message_.c_str();
}
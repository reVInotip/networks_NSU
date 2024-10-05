#include "udp_socket.h"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

using namespace udp_socket;

UdpSocket::UdpSocket(const string &ipaddr): ipaddr_(ipaddr) {
    addrinfo hint, *res;
    memset(&hint, 0, sizeof(hint));
    
    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(ipaddr.c_str(), NULL, &hint, &res) < 0)
        throw IncorrectIpAddrException {"Can not get information about address"};
    
    if (res->ai_family != AF_INET && res->ai_family != AF_INET6)
        throw IncorrectIpAddrException {"Unknown or unsupported ip address format"};

    ip_family_ = res->ai_family;

    freeaddrinfo(res);

    memset(&sender_addr4_, 0, sizeof(sockaddr_in));
    memset(&sender_addr6_, 0, sizeof(sockaddr_in6));
    sender_addr_len_ = ip_family_ == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

    memset(&receiver_addr4_, 0, sizeof(sockaddr_in));
    memset(&receiver_addr6_, 0, sizeof(sockaddr_in6));
    receiver_addr_len_ = ip_family_ == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

void UdpSocket::create_socket() {
    sockfd_ = socket(ip_family_, SOCK_DGRAM, 0);

    if (sockfd_ < 0)
        throw OpenSocketException {string {strerror(errno)}, errno};
}

void UdpSocket::multi_bind(const int port) const {
    int yes = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        close_socket();
        throw SetsockoptFailedException {string {strerror(errno)}, errno};
    }

    if (is_ipv6()) {
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(sockaddr_in6));

        addr.sin6_family = ip_family_;
        addr.sin6_port = htons(port);

        if (bind(sockfd_, (const sockaddr *) &addr, (socklen_t) sizeof(addr)) < 0) {
            close_socket();
            throw BindFailedException {string {strerror(errno)}, errno};
        }

        ipv6_mreq mreq6;
        if (!inet_pton(ip_family_, ipaddr_.c_str(), &mreq6.ipv6mr_multiaddr)) {
            close_socket();
            throw SocketException {string {strerror(errno)}, errno};
        }
        mreq6.ipv6mr_interface = 0;

        if (setsockopt(sockfd_, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0) {
            close_socket();
            throw SetsockoptFailedException {string {strerror(errno)}, errno};
        }
    } else {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr_in));

        addr.sin_family = ip_family_;
        addr.sin_port = htons(4569);

        if (bind(sockfd_, (const sockaddr *) &addr, (socklen_t) sizeof(addr)) < 0) {
            close_socket();
            throw BindFailedException {string {strerror(errno)}, errno};
        }

        ip_mreq mreq;
        if (!inet_pton(ip_family_, ipaddr_.c_str(), &mreq.imr_multiaddr)) {
            close_socket();
            throw SocketException {string {strerror(errno)}, errno};
        }
        mreq.imr_interface.s_addr = INADDR_ANY;

        if (setsockopt(sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            close_socket();
            throw SetsockoptFailedException {string {strerror(errno)}, errno};
        }
    }
}

void UdpSocket::server_bind(const int client_port) {
    if (is_ipv6()) {
        receiver_addr6_.sin6_port = htons(client_port);
        receiver_addr6_.sin6_family = AF_INET6;
        if (!inet_pton(AF_INET6, ipaddr_.c_str(), &receiver_addr6_.sin6_addr)) {
            close_socket();
            throw IncorrectIpAddrException {"inet_pton() err: invalid ip address: " + ipaddr_};
        }
    } else {
        receiver_addr4_.sin_port = htons(client_port);
        receiver_addr4_.sin_family = AF_INET;
        if (!inet_pton(AF_INET, ipaddr_.c_str(), &receiver_addr4_.sin_addr)) {
            close_socket();
            throw IncorrectIpAddrException {"inet_pton() err: invalid ip address: " + ipaddr_};
        }
    }
}
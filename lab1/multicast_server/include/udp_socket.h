#include <string>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

#pragma once

using std::string;

namespace udp_socket {
    class SocketException: public std::exception {
        protected:
            std::string message_;

        public:
            SocketException() = default;
            SocketException(const std::string& message);
            SocketException(const std::string& message, const int err_code);
            const char* what() const noexcept;
    };

    class TryAgainException final: public SocketException {};

    class OpenSocketException final: public SocketException {
        public:
            OpenSocketException(const std::string& message, const int err_code);
    };

    class SetsockoptFailedException final: public SocketException {
        public:
            SetsockoptFailedException(const std::string& message, const int err_code);
    };

    class BindFailedException final: public SocketException {
        public:
            BindFailedException(const std::string& message, const int err_code);
    };

    class IncorrectIpAddrException final: public SocketException {
        public:
            IncorrectIpAddrException(const std::string& message);
    };

    class RecvfromFailedException final: public SocketException {
        public:
            RecvfromFailedException(const std::string& message, const int err_code);
    };

    class SendtoFailedException final: public SocketException {
        public:
            SendtoFailedException(const std::string& message, const int err_code);
    };

    class UdpSocket final {
        private:
            int ip_family_;
            string ipaddr_;
            int sockfd_;
        
        private:
            sockaddr_in6 sender_addr6_;
            sockaddr_in sender_addr4_;
            socklen_t sender_addr_len_;

            sockaddr_in6 receiver_addr6_;
            sockaddr_in receiver_addr4_;
            socklen_t receiver_addr_len_;
        
        public:
            UdpSocket(const string &ipaddr);
            void create_socket();
            void multi_bind(const int port = 4569) const;
            void server_bind(const int port = 4569);

        public:
            template <typename T> T receive_from(string &sender_ip, int &sender_port) {
                T buffer;
                int err;
                if (is_ipv6()) {
                    err = recvfrom(sockfd_, &buffer, sizeof(T), 0, (sockaddr *) &sender_addr6_, (socklen_t *) &sender_addr_len_);
                    if (err < 0) {
                        if (err == EAGAIN || err == EWOULDBLOCK)
                            throw TryAgainException {};
                        throw RecvfromFailedException {string {strerror(errno)}, errno};
                    }

                    char str[INET6_ADDRSTRLEN];
                    if (inet_ntop(AF_INET6, &(sender_addr6_.sin6_addr), str, INET6_ADDRSTRLEN) == NULL)
                        throw SocketException {string {strerror(errno)}, errno};

                    sender_ip = str;
                    sender_port = sender_addr6_.sin6_port;
                } else {
                    err = recvfrom(sockfd_, &buffer, sizeof(T), 0, (sockaddr *) &sender_addr4_, (socklen_t *) &sender_addr_len_);
                    if (err < 0) {
                        if (err == EAGAIN || err == EWOULDBLOCK)
                            throw TryAgainException {};
                        throw RecvfromFailedException {string {strerror(errno)}, errno};
                    }

                    char str[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, &(sender_addr4_.sin_addr), str, INET_ADDRSTRLEN) == NULL)
                        throw SocketException {string {strerror(errno)}, errno};

                    sender_ip = str;
                    sender_port = sender_addr4_.sin_port;
                }

                return buffer;
            }

            template <typename T> void send_to(const T &message) {
                int err;
                if (is_ipv6()) {
                    err = sendto(sockfd_, &message, sizeof(message), 0,
                                (const sockaddr *) &receiver_addr6_, receiver_addr_len_);
                    if (err < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EALREADY)
                            throw TryAgainException {};
                        throw SendtoFailedException {string {strerror(errno)}, errno};
                    }
                } else {
                    err = sendto(sockfd_, &message, sizeof(message), 0,
                                (const sockaddr *) &receiver_addr4_, receiver_addr_len_);
                    if (err < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EALREADY)
                            throw TryAgainException {};
                        throw SendtoFailedException {string {strerror(errno)}, errno};
                    }
                }
            }
        
            void inline close_socket() const noexcept {
                close(sockfd_);
            }

            bool inline is_ipv6() const {
                return ip_family_ == AF_INET6;
            }
    };
}
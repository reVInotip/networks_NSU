#include <memory>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#pragma once

using std::string;

namespace socks_socket {
    class Socket {
        public:
            enum class SockFamily {
                IPv4 = AF_INET,
                IPv6 = AF_INET6
            };

            enum class SockType {
                STREAM = SOCK_STREAM,
                DATAGRAMM = SOCK_DGRAM,
                LISTEN
            };

            enum class SockOptions {
                NONBLOCK = O_NONBLOCK
            };
        
        private:
            SockFamily family_;
            SockType type_;
            int sockfd_;
            string ip_;
            uint16_t port_;
        
        public:
            string connected_address_;
            uint16_t connected_port_;
            std::shared_ptr<unsigned char[]> buffer_;
            size_t buffer_capacity_;
            size_t buffer_size_;
        
        public:
            Socket() = default;
            Socket(SockFamily family, SockType type, size_t buffer_capacity);
            Socket(int sockfd, SockFamily family, SockType type, size_t buffer_capacity);
            Socket(const Socket &socket) = default;
            ~Socket();
            
            void bind_to(uint16_t port);
            void bind_to(SockFamily family, const string &ip, uint16_t port);
            void connect_with(const string &ip, uint16_t port);
            void make_listen(int count_connections);
            void setoptions(SockOptions options);
            bool is_nonblocking() const;
            Socket *accept_connection(string &address) const;
            int get_fd() const noexcept;
            uint16_t get_port() const noexcept;
            string get_ip() const noexcept;

            int receive(string &address);
            int receive();
            void send_to(const void *message, size_t message_size) const;
            void send_to(const string &ip, uint16_t port, const void *message, size_t message_size) const;
    };

        class SockException: public std::exception {
        protected:
            std::string message_;

        public:
            SockException() = default;
            SockException(const std::string& message, const int err_code);
            const char* what() const noexcept;
    };

    class OpenSocketException final: public SockException {
        public:
            OpenSocketException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class CloseSocketException final: public SockException {
        public:
            CloseSocketException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class BindFailedException final : public SockException {
        public:
            BindFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class ConnectFailedException final : public SockException {
        public:
            ConnectFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class SetListenFailedException final : public SockException {
        public:
            SetListenFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class SetoptionsFailedException final : public SockException {
        public:
            SetoptionsFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class ReceiveFailedException final : public SockException {
        public:
            ReceiveFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class SendFailedException final : public SockException {
        public:
            SendFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };

    class AcceptFailedException final : public SockException {
        public:
            AcceptFailedException(const std::string& message, const int err_code): SockException {message, err_code} {};
    };
}
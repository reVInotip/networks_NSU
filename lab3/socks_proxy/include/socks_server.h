#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>

#pragma once

using std::string, std::unordered_map, std::set, std::vector;

namespace socks_server {
    struct Tunnel {
        int server_sockfd_;
        int client_sockfd_;
    
        Tunnel() = default;
        Tunnel(int server_sockfd, int client_sockfd) noexcept;
    };

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

            enum class SockState { 
                CONNECTED,
                CONNECTING,
                NEW
            };
        
        private:
            SockFamily family_;
            SockType type_;
            int sockfd_;
            size_t buffer_size_;
        
        public:
            SockState state_;
            std::shared_ptr<unsigned char[]> buffer_;
            Tunnel tunnel_;
        
        public:
            Socket() = default;
            Socket(SockFamily family, SockType type, size_t buffer_size);
            Socket(int sockfd, SockFamily family, SockType type, size_t buffer_size);
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

            int receive(string &address) const;
            void send_to(const void *message, int message_size) const;
    };

    class SocksServer {
        public:
            enum class AuthMethod {
                NO_AUTH = 0x00,
                GSSAPI = 0x01,
                USERPASS = 0x02
                // And more over methods
            };

            enum class RequestAnswer {
                OK = 0x00,
                SOCKS_SERVER_ERROR = 0x01,
                RULES_ERROR = 0x02,
                NETWORK_UNREACHABLE = 0x03,
                HOST_UNREACHABLE = 0x04,
                REJECT = 0x05,
                TTL_EXCEED = 0x06,
                COMMAND_UNSUPPORTED = 0x07,
                ADDRESS_UNSUPPORTED = 0x08
            };

            enum class AddrType {
                IPv4 = 0x01,
                DOMAIN = 0x03
            };

        private:
            Socket connection_request_sock_;
            unsigned char server_socks_version_;
            size_t buffer_size_;
            unordered_map<uint16_t, Socket*> client_sockets_;
            unordered_map<uint16_t, Socket*> server_sockets_;
            vector<struct pollfd> pfds_;

            static constexpr size_t max_answer_len = 262;
            static constexpr size_t ipv4_answer_len = 10;
        
        private:
            void send_error_message_to_client(Socket *client_socket, RequestAnswer error_answer) const noexcept;

        private:
            void poll_ctl_add(int fd, uint32_t events) noexcept;
            void poll_ctl_delete(int fd) noexcept;
            void client_communication_routine(int fd) noexcept;
            void server_communication_routine(int fd) noexcept;
            void client_disconnect(int client_fd) noexcept;
            void server_disconnect(int client_fd) noexcept;

            inline bool compare_versions(const unsigned char client_socks_version) const {
                return server_socks_version_ == client_socks_version;
            }

        public:
            SocksServer(const int port, const size_t buffer_size = 1024, unsigned char version = 5);
            SocksServer(const SocksServer &server) = delete;
            ~SocksServer();
            void accept_connections();
        
        public:
            SocksServer& operator=(const SocksServer &server) = delete;
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
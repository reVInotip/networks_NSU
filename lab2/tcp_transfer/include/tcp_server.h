#include <exception>
#include <string>
#include <thread>
#include <chrono>

#pragma once

using std::string;

namespace server {
    class Server final {
        private:
            const string workdir_ = "./uploads";

        private:
            int buffer_size_;

        private:
            int connection_request_sockfd_;
            int connection_request_port_;
            const std::chrono::milliseconds timeout_ {3000};
        
        private:
            void client_thread_routine(int client_fd) const;
            
        public:
            Server(const int port, const int buffer_size = 1000);
            Server(const Server &server) = delete;
            ~Server();
            void accept_connections() const;
        
        public:
            Server& operator=(const Server &server) = delete;
    };

    /*class ServerException: public std::exception {
        protected:
            std::string message_;

        public:
            ServerException() = default;
            ServerException(const std::string& message);
            const char* what() const noexcept;
    };

    class OpenSocketException final: public ServerException {
        public:
            OpenSocketException(const std::string& message, const int err_code);
    };

    class IncorrectIpAddrException final: public ServerException {
        public:
            IncorrectIpAddrException(const std::string& message);
    };

    class SendtoFailedException final: public ServerException {
        public:
            SendtoFailedException(const std::string& message, const int err_code);
    };*/
}
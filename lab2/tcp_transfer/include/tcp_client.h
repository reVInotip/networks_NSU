#include <exception>
#include <string>
#include <unordered_map>
#include <chrono>
#include <fstream>

#pragma once

using std::string, std::unordered_map;

namespace client {
    class Client final {
        private:
            int sockfd_;
            string path_to_file_;
        
        private:
            string get_file_name() const;

        public:
            Client(const string &path_to_file, const string &server_ipaddr, const int server_port);
            Client(const Client &client) = delete;
            ~Client();
            void send_file_to_server() const;
        
        public:
            Client& operator=(const Client &server) = delete;
    };

    class ClientException: public std::exception {
        protected:
            std::string message_;

        public:
            ClientException() = default;
            ClientException(const std::string& message);
            const char* what() const noexcept;
    };

    class OpenSocketException final: public ClientException {
        public:
            OpenSocketException(const std::string& message, const int err_code);
    };

    class ConnectFailedException final: public ClientException {
        public:
            ConnectFailedException(const std::string& message, const int err_code);
    };

    class OpenFileException final : public ClientException {
        public:
            OpenFileException();
    };

    class FileNotExistsException final : public ClientException {
        public:
            FileNotExistsException();
    };

    class SendFailedException final: public ClientException {
        public:
            SendFailedException(const std::string& message, const int err_code);
    };

    class RecvFailedException final : public ClientException {
        public:
            RecvFailedException(const std::string& message, const int err_code);
    };
}
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
            string get_filename_from_stream(int client_fd, string &end) const;
            double get_filesize_from_stream(int client_fd, string init, string &end) const;
            bool save_data_from_stream_to_file(int client_fd, string path_to_file, string init, double expected_file_size) const;
            string make_output_file_name(const string &init) const;
            
        public:
            Server(const int port, const int buffer_size = 1024);
            Server(const Server &server) = delete;
            ~Server();
            void accept_connections() const;
        
        public:
            Server& operator=(const Server &server) = delete;
    };

    class ServerException: public std::exception {
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

    class BindFailedException final : public ServerException {
        public:
            BindFailedException(const std::string& message, const int err_code);
    };

    class SetListenFailedException final : public ServerException {
        public:
            SetListenFailedException(const std::string& message, const int err_code);
    };

    class RecvFailedException final : public ServerException {
        public:
            RecvFailedException(const std::string& message, const int err_code);
    };

    class OpenFileException final : public ServerException {
        public:
            OpenFileException();
    };

    class SendFailedException final: public ServerException {
        public:
            SendFailedException(const std::string& message, const int err_code);
    };

    class AcceptFailedException final: public ServerException {
        public:
            AcceptFailedException(const std::string& message, const int err_code);
    };
}
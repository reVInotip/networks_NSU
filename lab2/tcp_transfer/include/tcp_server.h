#include <exception>
#include <string>
#include <thread>
#include <chrono>

#pragma once

using std::string;

namespace server {
    class Server final {
        private:
            class Printer final {
                private:
                    std::chrono::_V2::system_clock::time_point global_start_;
                    std::chrono::_V2::system_clock::time_point timer_start_;
                    const std::chrono::milliseconds timeout_;
                    double total_recieved_bytes_count_;
                    bool is_first_print1_;
                    bool is_first_print2_;
                
                public:
                    Printer();
                    void print_recv_info(int count_recv_bytes,
                                        std::chrono::_V2::system_clock::time_point &recv_start,
                                        string &filename);
                    void print_recv_info_with_percents(int count_recv_bytes,
                                                        std::chrono::_V2::system_clock::time_point &recv_start,
                                                        string &filename,
                                                        double curr_size,
                                                        double expected_file_size);
                    void print_recv_end(bool is_transfer_success);
            };

        private:
            const string workdir_ = "./uploads";

        private:
            int buffer_size_;

        private:
            int connection_request_sockfd_;
            int connection_request_port_;
        
        private:
            void client_thread_routine(int client_fd) const;
            string get_filename_from_stream(int client_fd, string &end, Printer &printer) const;
            double get_filesize_from_stream(int client_fd, string &filename, string init, string &end, Printer &printer) const;
            bool save_data_from_stream_to_file(int client_fd, string path_to_file, string init, double expected_file_size, Printer &printer) const;
            string make_output_file_name(const string &init) const;
            
        public:
            Server(const int port, const int buffer_size = 8 * 1024);
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
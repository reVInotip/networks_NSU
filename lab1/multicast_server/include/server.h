#include <exception>
#include <string>
#include <thread>
#include "udp_socket.h"

#pragma once

using std::string;

namespace server {
    class Server final {
        private:
            udp_socket::UdpSocket socket_;
        
        public:
            const unsigned server_id_;

        public:
            Server(const string &ipaddr);
            Server(const Server &server) = delete;
            ~Server();
            void send_to_copies();
        
        public:
            Server& operator=(const Server &server) = delete;
    };
}
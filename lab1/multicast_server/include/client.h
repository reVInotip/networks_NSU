#include <exception>
#include <string>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include "server.h"
#include "udp_socket.h"

#pragma once

using std::string, std::unordered_map;

namespace client {
    class Client final {
        private:
            udp_socket::UdpSocket socket_;
            unsigned number_of_connections_;
            unordered_map<string, std::pair<unsigned, std::chrono::_V2::system_clock::time_point>> connection_data_;
        
        private:
            const std::chrono::milliseconds timeout_ {1000};
            const std::chrono::milliseconds local_timeout_ {3000};
            const unsigned connected_server_id_;

        public:
            Client(const string &ipaddr, const server::Server &server);
            Client(const Client &client) = delete;
            ~Client();
            void recv_from_copies();

        public:
            Client& operator=(const Client &server) = delete;
    };
}
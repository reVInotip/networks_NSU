#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <poll.h>
#include <memory>
#include <tuple>
#include <ares.h>
#include "tunnel.h"
#include "socket.h"
#include "observe.h"

#pragma once

using std::string, std::unordered_map, std::set, std::vector;
using namespace tunnel;
using namespace socks_socket;

namespace socks_server {
    class SocksServer: public observe::Observer {
        private:
            Socket connection_request_sock_;
            Socket domain_names_resolver_sock_;
            unsigned char server_socks_version_;
            size_t buffer_size_;
            unordered_map<uint16_t, Tunnel*> client_connections_; // key is client socket fd
            unordered_map<uint16_t, Tunnel*> server_connections_;
            vector<struct pollfd> pfds_;
            vector<std::pair<int, int>> queue_to_delte_;

        private:
            void poll_ctl_add(int fd, uint32_t events) noexcept;
            void poll_ctl_delete(int fd) noexcept;
            void try_disconnect(std::pair<int, int> sock_pair) noexcept;

        public:
            SocksServer(const int port, const size_t buffer_size = 256000, unsigned char version = 5);
            SocksServer(const SocksServer &server) = delete;
            void accept_connections();
            void update(observe::Event &e) noexcept override;
        
        public:
            SocksServer& operator=(const SocksServer &server) = delete;
    };
}
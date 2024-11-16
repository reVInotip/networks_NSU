#include <string>
#include "ip.h"

#pragma once

namespace socks_client {
    class SocksClient {
        private:
            int start_sockfd_;
            int server_sockfd_;
            unsigned char client_socks_version_;
        
        private:
            int connect_to_server(const std::string &socks_server_ip, const uint16_t socks_server_port) const;

            inline bool compare_versions(const unsigned char server_socks_version) const {
                return client_socks_version_ == server_socks_version;
            }

            inline bool need_autentification(const unsigned char server_answer) const {
                return server_answer == 0xFF;
            }

            inline bool is_not_ipv6(const unsigned char server_answer) const {
                return server_answer != 0x04;
            }

        public:
            SocksClient(unsigned char version = 5);
            SocksClient(const SocksClient &client) = delete;
            ~SocksClient();
            void make_connection(const std::string &socks_server_ip, const uint16_t socks_server_port);
            void send_to_server(const void *buf, size_t n) const;
            void recv_from_server(void *buf, size_t n) const;
        
        public:
            SocksClient& operator=(const SocksClient &client) = delete;
    };
}
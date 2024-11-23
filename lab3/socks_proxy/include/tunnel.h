#include "observe.h"
#include "socket.h"
//#include <cpp-dns.hpp>
#include "build/async_dns_resolver/include/dns_resolve/dns_resolve.h"

#pragma once

//using namespace YukiWorkshop;
using namespace observe;
using namespace socks_socket;

namespace tunnel {
    class DeleteFromPollVectorEvent: public Event {
        public:
            int fd_;

            DeleteFromPollVectorEvent(int fd): fd_ {fd} {};
    };

    class AddToPollVectorEvent: public Event {
        public:
            int client_fd_;
            int server_fd_;

            AddToPollVectorEvent(int client_fd, int server_fd): client_fd_ {client_fd}, server_fd_ {server_fd} {};
            int get_data1() noexcept override {
                return client_fd_;
            };

            int get_data2() noexcept override {
                return server_fd_;
            };
    };

    class Tunnel: public Observable {
        public:
            enum class RequestType {
                IN,
                OUT
            };

            enum class RequestSource {
                CLIENT,
                SERVER
            };

        private:
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

            enum class Commands {
                MakeTCPConnection = 0x01
            };

            enum class TunnelState { 
                CONNECTED,
                CONNECTING,
                UNINIT,
                DISCONNECT,
                HALF_DISCONNECT
            };

        private:
            unsigned char server_socks_version_;
            socks_socket::Socket *server_;
            socks_socket::Socket *client_;
            TunnelState state_;
            dnsresolve::Resolver *resolver_;
            socks_socket::Socket *resolver_sock_;

            static constexpr size_t ipv4_answer_len = 10;
            static constexpr size_t ipv4_request_len = 10;
            static constexpr size_t min_request_len = 8;
        
        private:
            inline bool compare_versions(const unsigned char client_socks_version) const {
                return server_socks_version_ == client_socks_version;
            }

        public:
            Tunnel();
            Tunnel(unsigned char socks_version, dnsresolve::Resolver *resolver, socks_socket::Socket *resolver_sock,
                Socket *client, size_t buffer_capacity) noexcept;

            void recieve_from_client() noexcept;
            void recieve_from_server() noexcept;
            void send_to_client() noexcept;
            void send_to_server() noexcept;
            void request_handler(RequestType type, RequestSource source) noexcept;
            void disconnect() noexcept;

            int get_clientfd() noexcept;
            int get_serverfd() noexcept;
            string get_client_connected_ip() noexcept;
            uint16_t get_client_connected_port() noexcept;
            bool is_disconnect() noexcept;
            bool try_connect(const string &ip, uint16_t port);
    };
};
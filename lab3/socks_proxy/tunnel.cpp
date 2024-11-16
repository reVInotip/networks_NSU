#include "socks_server.h"

using namespace socks_server;

Tunnel::Tunnel(int server_sockfd, int client_sockfd) noexcept:
    server_sockfd_ {server_sockfd}, client_sockfd_ {client_sockfd} {};

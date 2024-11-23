#include "socks_server.h"
#include <iostream>

using namespace socks_server;

int main(void) {
    try {
        SocksServer server {12345};
        std::cout << "Create SOCKS server" << std::endl;
        server.accept_connections();
    } catch (SockException &e) {
        std::cerr << "Can not create SOCKS server: " << e.what() << std::endl;
    } catch (std::bad_alloc &e) {
        std::cerr << "Can not create SOCKS server: " << e.what() << std::endl;
    }
}
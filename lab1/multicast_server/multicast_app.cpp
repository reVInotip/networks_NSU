#include "multicast_app.h"
#include "client.h"
#include "server.h"
#include <thread>

MulticastApp::MulticastApp(const string &multicast_ip):
    server_(multicast_ip), client_(multicast_ip, server_) {}

void MulticastApp::run() {
    try {
        std::thread client_thread = std::thread(&client::Client::recv_from_copies, &client_);
        std::thread server_thread = std::thread(&server::Server::send_to_copies, &server_);

        client_thread.join();
        server_thread.join();
    } catch (std::system_error &e) {
        std::cerr << e.what() << std::endl;
        std::terminate();
    }
}
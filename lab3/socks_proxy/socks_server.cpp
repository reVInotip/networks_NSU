#include "socks_server.h"
#include <poll.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <tuple>
#include <sstream>

#include "dns_resolve/dns_resolve.h"
//#include <cpp-dns.hpp>

//using namespace YukiWorkshop;

using namespace socks_server;
using std::stringstream;
using std::string;

SocksServer::SocksServer(const int port, const size_t buffer_size, unsigned char version):
    connection_request_sock_ {Socket::SockFamily::IPv4, Socket::SockType::STREAM, buffer_size},
    domain_names_resolver_sock_ {Socket::SockFamily::IPv4, Socket::SockType::DATAGRAMM, buffer_size},
    server_socks_version_ {version}, buffer_size_ {buffer_size}
{
    connection_request_sock_.setoptions(Socket::SockOptions::NONBLOCK);
    connection_request_sock_.bind_to(port);
    connection_request_sock_.make_listen(10);

    domain_names_resolver_sock_.setoptions(Socket::SockOptions::NONBLOCK);
    domain_names_resolver_sock_.bind_to(port + 1);
    
    poll_ctl_add(connection_request_sock_.get_fd(), POLLIN);
    poll_ctl_add(domain_names_resolver_sock_.get_fd(), POLLIN);
}

void SocksServer::poll_ctl_add(int fd, uint32_t events) noexcept {
    struct pollfd pollfd;
    pollfd.fd = fd;
    pollfd.events = events;
    pfds_.push_back(pollfd);
}

void SocksServer::poll_ctl_delete(int fd) noexcept {
    for (auto it = pfds_.begin(); it != pfds_.end(); ++it) {
        if ((*it).fd == fd) {
            pfds_.erase(it);
            return;
        }
    }
}

void SocksServer::try_disconnect(std::pair<int, int> sock_pair) noexcept {
    if (!client_connections_.contains(sock_pair.first) && !server_connections_.contains(sock_pair.second)) {
        std::cerr << "[-] can not brake connection\n";
    }

    if (client_connections_.contains(sock_pair.first)) {
        poll_ctl_delete(sock_pair.first);
        delete client_connections_[sock_pair.first];
        client_connections_.erase(sock_pair.first);
    }

    if (server_connections_.contains(sock_pair.second)) {
        poll_ctl_delete(sock_pair.second);
        server_connections_.erase(sock_pair.second);
    }
}

void SocksServer::update(observe::Event &e) noexcept {
    int server_fd_ = e.get_data2();
    int client_fd_ = e.get_data1();
    if (e.get_type() == Event::EvType::ADD) {
        server_connections_[server_fd_] = client_connections_[client_fd_];
        poll_ctl_add(server_fd_, POLLIN | POLLOUT | POLLRDHUP | POLLHUP);
    } else if (e.get_type() == Event::EvType::DEL) {
        queue_to_delte_.push_back(std::pair<int, int>(client_fd_, server_fd_));
    }
}

void SocksServer::accept_connections() {
    int nfds;
    string address;
    dnsresolve::Resolver *resolver = new dnsresolve::Resolver {};

    while (true) {
        nfds = poll(pfds_.data(), pfds_.size(), -1);
        if (nfds < 0) {
            std::cerr << "[-] something went wrong: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < pfds_.size(); ++i) {
            if (pfds_[i].revents == 0) continue;

            if (pfds_[i].fd == domain_names_resolver_sock_.get_fd()) {
                uint16_t client_fd;
                int n;

                std::cout << "[!] domain name resolved\n";

                try {
                    n = domain_names_resolver_sock_.receive();
                } catch (ReceiveFailedException &e) {
                    std::cerr << "[-] error while recieve data from internal UDP socket" << e.what() << std::endl;
                    break;
                }

                client_fd = std::atoi(
                    reinterpret_cast<char *>(domain_names_resolver_sock_.buffer().data())
                );

                Tunnel *tunnel = client_connections_[client_fd];
                string ip = tunnel->get_client_connected_ip();
                uint16_t port = tunnel->get_client_connected_port();

                try {
                    if(!tunnel->try_connect(ip, port)) break;
                } catch (SendFailedException &e) {
                    std::cerr << "[-] error while sending server (" << ip << ":" << port
                        << ") message to client:" << e.what() << std::endl;
                }
            } else if (pfds_[i].fd == connection_request_sock_.get_fd()) {
                /* handle new connection */
                Socket *client_socket;
                try {
                    client_socket = connection_request_sock_.accept_connection(address);
                } catch (AcceptFailedException &e) {
                    std::cerr << "[-] can not accept new connection because: " << e.what() << std::endl;
                    break;
                }

                Tunnel *new_connection = new Tunnel {server_socks_version_, resolver,
                    &domain_names_resolver_sock_, client_socket, buffer_size_};
                
                new_connection->add(this);

                std::cout << "[+] connected with " << address << std::endl;

                poll_ctl_add(client_socket->get_fd(), POLLIN | POLLOUT | POLLRDHUP | POLLHUP);

                client_connections_[client_socket->get_fd()] = new_connection;
            } else if (pfds_[i].revents & POLLIN) {
                /* handle EPOLLIN event */
                if(client_connections_.contains(pfds_[i].fd)) {
                    client_connections_[pfds_[i].fd]->request_handler(Tunnel::RequestType::IN, Tunnel::RequestSource::CLIENT);
                } else {
                    server_connections_[pfds_[i].fd]->request_handler(Tunnel::RequestType::IN, Tunnel::RequestSource::SERVER);
                }
            } else if (pfds_[i].revents & POLLOUT) {
                if(client_connections_.contains(pfds_[i].fd)) {
                    client_connections_[pfds_[i].fd]->request_handler(Tunnel::RequestType::OUT, Tunnel::RequestSource::CLIENT);
                } else {
                    server_connections_[pfds_[i].fd]->request_handler(Tunnel::RequestType::OUT, Tunnel::RequestSource::SERVER);
                }
            } else if ((pfds_[i].revents & POLLRDHUP) | (pfds_[i].revents & POLLHUP)) {
                std::cout << "[+] disconnected with client\n";
                if(client_connections_.contains(pfds_[i].fd)) {
                    client_connections_[pfds_[i].fd]->close_connection();
                } else {
                    server_connections_[pfds_[i].fd]->close_connection();
                }
            } else {
				std::cerr << "[-] unexpected " << strerror(errno) << pfds_[i].fd << std::endl;
                if(client_connections_.contains(pfds_[i].fd)) {
                    client_connections_[pfds_[i].fd]->close_connection();
                } else {
                    server_connections_[pfds_[i].fd]->close_connection();
                }
                return;
			}

            pfds_[i].revents = 0;
        }

        while (!queue_to_delte_.empty()) {
            try_disconnect(queue_to_delte_.back());
            queue_to_delte_.pop_back();
        }
    }

    resolver->Stop();
    delete resolver;
}
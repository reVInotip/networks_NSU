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

using namespace socks_server;
using std::stringstream;
using Socket::SockState::CONNECTED;
using Socket::SockState::CONNECTING;
using Socket::SockState::NEW;

SocksServer::SocksServer(const int port, const size_t buffer_size, unsigned char version):
    connection_request_sock_ {Socket::SockFamily::IPv4, Socket::SockType::STREAM, buffer_size}, server_socks_version_ {version},
    buffer_size_ {buffer_size}
{
    connection_request_sock_.setoptions(Socket::SockOptions::NONBLOCK);
    connection_request_sock_.bind_to(port);
    connection_request_sock_.make_listen(10);
    
    poll_ctl_add(connection_request_sock_.get_fd(), POLLIN);
}

SocksServer::~SocksServer() {
    for (auto sock_pair: client_sockets_) {
        delete sock_pair.second;
    }

    for (auto sock_pair: server_sockets_) {
        delete sock_pair.second;
    }
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

void SocksServer::client_disconnect(int client_fd) noexcept {
    poll_ctl_delete(client_fd);
    Socket *client_socket = client_sockets_[client_fd];
    if (client_socket != nullptr) {
        Socket *server_socket = server_sockets_[client_socket->tunnel_.server_sockfd_];
        if (server_socket != nullptr) {
            poll_ctl_delete(server_socket->get_fd());
            server_sockets_.erase(server_socket->get_fd());
            delete server_socket;
        } else
            std::cerr << "[-] something wrong with server socket. Can not broke connection!" << std::endl;

        client_sockets_.erase(client_fd);
        delete client_socket;
    } else
        std::cerr << "[-] something wrong with client socket. Can not broke connection!" << std::endl;
}

void SocksServer::server_disconnect(int server_fd) noexcept {
    poll_ctl_delete(server_fd);
    Socket *server_socket = server_sockets_[server_fd];
    if (server_socket != nullptr) {
        Socket *client_socket = client_sockets_[server_socket->tunnel_.client_sockfd_];
        if (client_socket != nullptr) {
            poll_ctl_delete(client_socket->get_fd());
            client_sockets_.erase(client_socket->get_fd());
            delete client_socket;
        } else
            std::cerr << "[-] something wrong with client socket. Can not broke connection!" << std::endl;

        server_sockets_.erase(server_fd);
        delete server_socket;
    } else
        std::cerr << "[-] something wrong with server socket. Can not broke connection!" << std::endl;
}

void SocksServer::server_communication_routine(int fd) noexcept {
    Socket *socket = server_sockets_[fd];
    Socket *client_socket = client_sockets_[socket->tunnel_.client_sockfd_];
    string server_address;

    int n = 0;
    while (true) {
        try {
            n = socket->receive(server_address);
        } catch (ReceiveFailedException &e) {
            std::cerr << "[-] error while recieve data from server (" << server_address << "):" << e.what() << std::endl;
            server_disconnect(fd);
            break;
        }

        if (n == 0) {
            std::cout << "[+] disconnected with " << server_address << std::endl;
            server_disconnect(fd);
            break;
        } else if (n < 0) {
            std::cout << "[~] end of data from server: " << server_address << std::endl;
            break;
        }
        
        std::cout << "[~] retranslate messages to clent from server: " << server_address << std::endl;
        try {
            client_socket->send_to(socket->buffer_.get(), n);
        } catch (SendFailedException &e) {
            std::cerr << "[-] error while sending server (" << server_address << ") message to client:" << e.what() << std::endl;
            break;
        }
    }
}

void SocksServer::client_communication_routine(int fd) noexcept {
    Socket *socket = client_sockets_[fd];
    string client_address;

    int n = 0;
    while (true) {
        try {
            n = socket->receive(client_address);
        } catch (ReceiveFailedException &e) {
            std::cerr << "[-] error while recieve data from client (" << client_address << "):" << e.what() << std::endl;
            client_disconnect(fd);
            break;
        }

        if (n == 0) {
            std::cout << "[+] disconnected with " << client_address << std::endl;
            client_disconnect(fd);
            break;
        } else if (n < 0) {
            std::cout << "[~] end of data from clent: " << client_address << std::endl;
            break;
        }
        
        // if client alredy accept connection to main we should retranslate IP packets to server
        if (socket->state_ == CONNECTED) {
            std::cout << "[~] retranslate messages from clent: " << client_address << std::endl;
            Socket *server_socket = server_sockets_[socket->tunnel_.server_sockfd_];
            try {
                server_socket->send_to(socket->buffer_.get(), n);
            } catch (SendFailedException &e) {
                std::cerr << "[-] error while sending server (" << client_address << ") message to client:" << e.what() << std::endl;
                break;
            }
        // if client accept conection only with our proxy we wait command from him
        } else if (socket->state_ == CONNECTING) {
            if (!compare_versions(socket->buffer_.get()[0])) {
                std::cerr << "[-] client (" << client_address << ") version of SOCKS protocol (" << socket->buffer_.get()[0] <<
                    ") does not match with server (" << server_socks_version_ << ")\n";
                break;
            }

            unsigned char answer[max_answer_len] = {0};
            
            // connect by IPv4
            if (socket->buffer_.get()[1] == 0x01) {
                if (n < static_cast<int>(ipv4_answer_len)) break;

                answer[0] = server_socks_version_; // set SOCKS protocol version

                std::cout << "[!] getting command to make TCP/IP connect from client: " << client_address << std::endl;
                
                if (socket->buffer_.get()[3] == static_cast<unsigned char>(AddrType::IPv4)) {
                    string address = std::to_string(socket->buffer_.get()[4]) + "." +
                            std::to_string(socket->buffer_.get()[5]) + "." +
                            std::to_string(socket->buffer_.get()[6]) + "." +
                            std::to_string(socket->buffer_.get()[7]);
                    uint16_t port = (socket->buffer_.get()[8] & 0b1111111100000000) | (socket->buffer_.get()[9] & 0b0000000011111111);

                    Socket *server_socket = new Socket {Socket::SockFamily::IPv4, Socket::SockType::STREAM, buffer_size_};
                    try {
                        server_socket->connect_with(address, port);
                        server_socket->setoptions(Socket::SockOptions::NONBLOCK);
                    } catch (ConnectFailedException &e) {
                        std::cerr << "[-] can not make new connection because: " << e.what() << std::endl;

                        answer[1] = static_cast<unsigned char>(RequestAnswer::REJECT);
                        try {
                            socket->send_to(answer, ipv4_answer_len);
                        } catch (SendFailedException &e) {
                            std::cerr << "[-] error while sending server (" << client_address << ") message to client:" << e.what() << std::endl;
                            break;
                        }
                        client_disconnect(fd);
                        break;
                    } catch (SetoptionsFailedException &e) {
                        std::cerr << "[-] can not make server socket nonblocking because: " << e.what() << std::endl;

                        answer[1] = static_cast<unsigned char>(RequestAnswer::SOCKS_SERVER_ERROR);
                        try {
                            socket->send_to(answer, ipv4_answer_len);
                        } catch (SendFailedException &e) {
                            std::cerr << "[-] error while sending server (" << client_address << ") message to client:" << e.what() << std::endl;
                            break;
                        }
                        client_disconnect(fd);
                        break;
                    }

                    poll_ctl_add(server_socket->get_fd(), POLLIN | POLLRDHUP | POLLHUP);

                    server_socket->state_ = CONNECTED;
                    socket->state_ = CONNECTED;

                    server_socket->tunnel_ = Tunnel {server_socket->get_fd(), socket->get_fd()};
                    socket->tunnel_ = server_socket->tunnel_;

                    server_sockets_[server_socket->get_fd()] = server_socket;

                    answer[1] = static_cast<unsigned char>(RequestAnswer::OK);

                    // set ip address to answer
                    answer[3] = static_cast<unsigned char>(AddrType::IPv4);

                    string ip_part;
                    stringstream stream {address};
                    for (int i = 4; std::getline(stream, ip_part, '.'); ++i) {
                        answer[i] = static_cast<unsigned char>(std::atoi(ip_part.c_str()));
                    }

                    // set port to answer
                    answer[8] = socket->buffer_.get()[8];
                    answer[9] = socket->buffer_.get()[9];

                    try {
                        socket->send_to(answer, ipv4_answer_len);
                    } catch (SendFailedException &e) {
                        std::cerr << "[-] error while sending server (" << client_address << ") message to client:" << e.what() << std::endl;
                        break;
                    }
                } else if (socket->buffer_.get()[3] == static_cast<unsigned char>(AddrType::DOMAIN)) {
                    break;
                }
            } else {
                std::cerr << "[-] client (" << client_address << ") command is wrong or unsupported" << std::endl;
                break;
            }
        // else we receive first message from client and we try to accept connection with it
        } else {
            if (!compare_versions(socket->buffer_.get()[0])) {
                std::cerr << "[-] client (" << client_address << ") version of SOCKS protocol (" << socket->buffer_.get()[0] <<
                    ") does not match with server (" << server_socks_version_ << ")\n";
                break;
            }
            
            if (socket->buffer_.get()[1] != 0x00) {
                //...
            }

            unsigned char answer[2] =
            {
                server_socks_version_, // socks version
                static_cast<unsigned char>(AuthMethod::NO_AUTH) // auth method (no auth)
            };

            try {
                socket->send_to(answer, 2);
            } catch (SendFailedException &e) {
                std::cerr << "[-] error while sending server (" << client_address << ") message to client:" << e.what() << std::endl;
                break;
            }

            socket->state_ = CONNECTING;
        }
    }
}

void SocksServer::accept_connections() {
    int nfds;
    string address;

    while (true) {
        nfds = poll(pfds_.data(), pfds_.size(), -1);
        if (nfds < 0) {
            std::cerr << "[-] something went wrong: " << strerror(errno) << std::endl;
            break;
        }

        for (size_t i = 0; i < pfds_.size(); i++) {
            if (pfds_[i].revents == 0) continue;

            if (pfds_[i].fd == connection_request_sock_.get_fd()) {
				/* handle new connection */
                Socket *conn_sock;
                try {
                    conn_sock = connection_request_sock_.accept_connection(address);
                } catch (AcceptFailedException &e) {
                    std::cerr << "[-] can not accept new connection because: " << e.what() << std::endl;
                    break;
                }

				std::cout << "[+] connected with " << address << std::endl;

                conn_sock->setoptions(Socket::SockOptions::NONBLOCK);
				poll_ctl_add(conn_sock->get_fd(), POLLIN | POLLRDHUP | POLLHUP);
                conn_sock->state_ = NEW;
                
                client_sockets_[conn_sock->get_fd()] = conn_sock;
            } else if (pfds_[i].revents & POLLIN) {
				/* handle EPOLLIN event */
                if (server_sockets_.contains(pfds_[i].fd))
                    server_communication_routine(pfds_[i].fd);
                else
				    client_communication_routine(pfds_[i].fd);
            } else if ((pfds_[i].revents & POLLRDHUP) | (pfds_[i].revents & POLLHUP)) {
                std::cout << "[+] disconnected with client\n";
                if (server_sockets_.contains(pfds_[i].fd))
                    server_disconnect(pfds_[i].fd);
                else
				    client_disconnect(pfds_[i].fd);
            } else {
				std::cerr << "[-] unexpected" << strerror(errno) << std::endl;
                if (server_sockets_.contains(pfds_[i].fd))
                    server_disconnect(pfds_[i].fd);
                else
				    client_disconnect(pfds_[i].fd);
			}

            pfds_[i].revents = 0;
        }
    }
}  
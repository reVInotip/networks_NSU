#include "tunnel.h"
#include "socket.h"
//#include <ares.h>
//#include <cpp-dns.hpp>

//using namespace YukiWorkshop;

#include <iostream>
#include <string>
#include <sstream>

using std::string, std::stringstream;
using namespace socks_socket;
using namespace tunnel;

Tunnel::Tunnel(): state_ {TunnelState::UNINIT} {};

Tunnel::Tunnel(unsigned char socks_version, dnsresolve::Resolver *resolver, Socket *resolver_sock, Socket *client, size_t buffer_capacity) noexcept:
    server_socks_version_ {socks_version}, state_ {TunnelState::UNINIT}, resolver_ {resolver}, resolver_sock_ {resolver_sock},
    client_ {client}
{
    server_ = new Socket {Socket::SockFamily::IPv4, Socket::SockType::STREAM, buffer_capacity};
    client_->setoptions(Socket::SockOptions::NONBLOCK);
};

void Tunnel::recieve_from_client() noexcept {
    string client_address;

    long count_bytes = 0;
    try {
        count_bytes = client_->receive(client_address);
    } catch (ReceiveFailedException &e) {
        std::cerr << "[-] error while recieve data from client (" << client_address << "):" << e.what() << std::endl;
        disconnect();
    }

    if (count_bytes == 0) {
        std::cout << "[+] disconnected with " << client_address << std::endl;
        disconnect();
    } else if (count_bytes < 0) {
        std::cout << "[~] end of data from clent: " << client_address << std::endl;
    }

    client_->buffer_size_ = count_bytes;
    client_->connected_address_ = client_address;

    //std::cerr << "recv from client " <<client_->buffer_.get() << std::endl;
}

void Tunnel::send_to_server() noexcept {
    if (client_->buffer_size_ == 0) return;
    if (state_ != TunnelState::CONNECTED) {
        //std::cerr << "[-] tunnel not connected" << static_cast<int>(state_) << std::endl;
        return;
    }

    std::cout << "[~] retranslate messages from clent: " << client_->connected_address_ << std::endl;
    try {
        server_->send_to(client_->buffer_.get(), client_->buffer_size_);
    } catch (SendFailedException &e) {
        std::cerr << "[-] error while sending server (" << client_->connected_address_ << ") message to client:" << e.what() << std::endl;
    }

    client_->buffer_size_ = 0;

    //std::cerr << "send to server " << client_->buffer_.get() << std::endl;
}

void Tunnel::recieve_from_server() noexcept {
    if (state_ != TunnelState::CONNECTED) {
        //std::cerr << "[-] tunnel not connected\n";
        return;
    } 
    string server_address;

    long count_bytes = 0;
    try {
        count_bytes = server_->receive(server_address);
    } catch (ReceiveFailedException &e) {
        std::cerr << "[-] error while recieve data from server (" << server_address << "):" << e.what() << std::endl;
        disconnect();
    }

    if (count_bytes == 0) {
        std::cout << "[+] disconnected with " << server_address << std::endl;
        disconnect();
    } else if (count_bytes < 0) {
        std::cout << "[~] end of data from clent: " << server_address << std::endl;
    }

    server_->buffer_size_ = count_bytes;
    server_->connected_address_ = server_address;

    //std::cerr << "recv from server " << server_->buffer_.get() << std::endl;
}

void Tunnel::send_to_client() noexcept {
    if (server_->buffer_size_ == 0) return;
    std::cout << "[~] retranslate messages from server: " << server_->connected_address_ << std::endl;
    try {
        client_->send_to(server_->buffer_.get(), server_->buffer_size_);
    } catch (SendFailedException &e) {
        std::cerr << "[-] error while sending server (" << server_->connected_address_ << ") message to client:" << e.what() << std::endl;
    }

    server_->buffer_size_ = 0;

    if (state_ == TunnelState::HALF_DISCONNECT) {
        disconnect();
    }

    //std::cerr << "send to client " << server_->buffer_.get() << std::endl;
}


/* Callback that is called when DNS query is finished */
/*void addrinfo_cb(void *arg, int status, int timeouts,
                        struct ares_addrinfo *result) {
    Tunnel *curr_tunnel = (Tunnel *) arg;
    std::cout << "Result: " << ares_strerror(status) << " timeouts: " << timeouts << std::endl;

    string server_ip;
    if (result) {
        struct ares_addrinfo_node *node;
        for (node = result->nodes; node != NULL; node = node->ai_next) {
            char addr_buf[64] = "";
            const void *ptr = nullptr;
            if (node->ai_family == AF_INET) {
                const struct sockaddr_in *in_addr =
                (const struct sockaddr_in *)((void *)node->ai_addr);
                ptr = &in_addr->sin_addr;
            } else if (node->ai_family == AF_INET6) {
                const struct sockaddr_in6 *in_addr =
                (const struct sockaddr_in6 *)((void *)node->ai_addr);
                ptr = &in_addr->sin6_addr;
            } else {
                continue;
            }
            ares_inet_ntop(node->ai_family, ptr, addr_buf, sizeof(addr_buf));
            printf("Addr: %s\n", addr_buf);

            server_ip = addr_buf;
        }
    }

    curr_tunnel->try_connect(server_ip, curr_tunnel->get_client_connected_port());
    ares_freeaddrinfo(result);
}*/

void Tunnel::request_handler(RequestType type, RequestSource source) noexcept {
    if (type == RequestType::OUT && source == RequestSource::CLIENT) {
        send_to_client();
    } else if (type == RequestType::OUT && source == RequestSource::SERVER) {
        send_to_server();
    } else if (type == RequestType::IN && source == RequestSource::SERVER) {
        recieve_from_server();
    } else if (type == RequestType::IN && source == RequestSource::CLIENT) {
        recieve_from_client();

        if (state_ == TunnelState::CONNECTED || state_ == TunnelState::DISCONNECT) {
            return;
        }

        if (!compare_versions(client_->buffer_.get()[0])) {
            std::cerr << "[-] client (" << client_->connected_address_ << ") version of SOCKS protocol (" <<
             client_->buffer_.get()[0] << ") does not match with server (" << server_socks_version_ << ")\n";
            
            client_->buffer_size_ = 0;
            return;
        }
        if (state_ == TunnelState::CONNECTING) {
            // connect by IPv4
            if (client_->buffer_.get()[1] == static_cast<unsigned char>(Commands::MakeTCPConnection)) {
                if (client_->buffer_size_ < static_cast<int>(ipv4_request_len)) return; // Do something
                
                if (client_->buffer_.get()[3] == static_cast<unsigned char>(AddrType::IPv4)) {
                    std::cout << "[!] getting command to make TCP/IP connect by ip from client: " << client_->connected_address_ << std::endl;

                    string address = std::to_string(client_->buffer_.get()[4]) + "." +
                            std::to_string(client_->buffer_.get()[5]) + "." +
                            std::to_string(client_->buffer_.get()[6]) + "." +
                            std::to_string(client_->buffer_.get()[7]);
                    uint16_t port = (client_->buffer_.get()[8] & 0b1111111100000000) | (client_->buffer_.get()[9] & 0b0000000011111111);

                    try {
                        if(!try_connect(address, port)) return;
                    } catch (SendFailedException &e) {
                        std::cerr << "[-] error while sending server (" << client_->connected_address_ << ") message to client:" << e.what() << std::endl;
                    }
                } else if (client_->buffer_.get()[3] == static_cast<unsigned char>(AddrType::DOMAIN)) {
                    if (client_->buffer_size_ < static_cast<int>(min_request_len)) return; // Do something

                    int len = static_cast<int>(client_->buffer_.get()[4]);
                    string domain_name;

                    int i = 5;
                    for (int j = 0; j < len; ++i, ++j) {
                        domain_name += client_->buffer_.get()[i];
                    }


                    std::cout << "[!] getting command to make TCP/IP connect by domain name: " << domain_name
                        << " from client: " << client_->connected_address_ << std::endl;

                    uint16_t port = (client_->buffer_.get()[i] & 0b1111111100000000) | (client_->buffer_.get()[i + 1] & 0b0000000011111111);

                    resolver_->AsyncResolve(domain_name, [&](const dnsresolve::Result& result) -> void {
                        if (result.HasError()) {
                            std::cout << result.Error() << std::endl;
                        } else {
                            std::cout << result.Name() << std::endl;
                            for (auto &it : result) {
                                string server_ip;
                                std::stringstream ss {server_ip};
                                ss << it;
                                Socket sock {Socket::SockFamily::IPv4, Socket::SockType::DATAGRAMM, client_->buffer_capacity_};
                                sock.setoptions(Socket::SockOptions::NONBLOCK);

                                string ip = resolver_sock_->get_ip() == "localhost" ?
                                    "127.0.0.1" : resolver_sock_->get_ip();

                                this->server_->connected_address_ = ip;
                                this->server_->connected_port_ = port;

                                string message = std::to_string(this->client_->get_fd());
                                
                                sock.send_to(ip, resolver_sock_->get_port(), message.c_str(), message.size());
                                break;
                            }
                        }
                    });
                    resolver_->Run();

                    /*resolver_->resolve_a4(domain_name, [this, port, len](int err, auto& addrs, auto& qname, auto& cname, uint ttl) {
                        if (!err) {
                            for (auto &it : addrs) {
                                string server_ip = it.to_string();
                                Socket sock {Socket::SockFamily::IPv4, Socket::SockType::DATAGRAMM, client_->buffer_capacity_};
                                sock.setoptions(Socket::SockOptions::NONBLOCK);

                                string ip = resolver_sock_->get_ip() == "localhost" ?
                                    "127.0.0.1" : resolver_sock_->get_ip();

                                this->server_->connected_address_ = ip;
                                this->server_->connected_port_ = port;

                                string message = std::to_string(this->client_->get_fd());
                                
                                sock.send_to(ip, resolver_sock_->get_port(), message.c_str(), message.size());
                                break;
                            }
                        }
                    });*/

                    //prepare dns request
                    /*ares_getaddrinfo(resolver_->channel_, domain_name.c_str(), NULL,
                        &(resolver_->hints_), addrinfo_cb, this);*/
                    
                    std::cout << "[!] start resolve domain name\n";
                }
            } else {
                std::cerr << "[-] client (" << client_->connected_address_ << ") command is wrong or unsupported" << std::endl;
            }
        } else if (state_ == TunnelState::UNINIT) {
            if (client_->buffer_.get()[1] != 0x00) {
                //...
            }

            //ares_process_fd(resolver_->channel_, client_->get_fd(), client_->get_fd());

            server_->buffer_.get()[0] = server_socks_version_;
            server_->buffer_.get()[1] = static_cast<unsigned char>(AuthMethod::NO_AUTH);

            server_->buffer_size_ = 2;
            client_->buffer_size_ = 0;

            state_ = TunnelState::CONNECTING;
        }
    }
}

bool Tunnel::try_connect(const string &ip, uint16_t port) {
    server_->buffer_.get()[0] = server_socks_version_;
    try {
        server_->connect_with(ip, port);
        server_->setoptions(Socket::SockOptions::NONBLOCK);
    } catch (ConnectFailedException &e) {
        std::cerr << "[-] can not make new connection because: " << e.what() << std::endl;

        server_->buffer_.get()[1] = static_cast<unsigned char>(RequestAnswer::REJECT);
        state_ = TunnelState::HALF_DISCONNECT;
        return false;
    } catch (SetoptionsFailedException &e) {
        std::cerr << "[-] can not make server socket nonblocking because: " << e.what() << std::endl;

        server_->buffer_.get()[1] = static_cast<unsigned char>(RequestAnswer::SOCKS_SERVER_ERROR);
        state_ = TunnelState::HALF_DISCONNECT;
        return false;
    }

    // add new socket to poll
    AddToPollVectorEvent event {client_->get_fd(), server_->get_fd()};
    update(event);

    // set connection set
    state_ = TunnelState::CONNECTED;

    server_->buffer_.get()[1] = static_cast<unsigned char>(RequestAnswer::OK);

    // set ip address to answer
    server_->buffer_.get()[3] = static_cast<unsigned char>(AddrType::IPv4);

    string ip_part;
    stringstream stream {ip};
    for (int i = 4; std::getline(stream, ip_part, '.'); ++i) {
        server_->buffer_.get()[i] = static_cast<unsigned char>(std::atoi(ip_part.c_str()));
    }

    // set port to answer
    server_->buffer_.get()[8] = client_->buffer_.get()[8];
    server_->buffer_.get()[9] = client_->buffer_.get()[9];

    server_->buffer_size_ = ipv4_answer_len;
    client_->buffer_size_ = 0;

    return true;
}

void Tunnel::disconnect() noexcept {
    state_ = TunnelState::DISCONNECT;
}

int Tunnel::get_serverfd() noexcept {
    if (state_ == TunnelState::CONNECTING)
        return -1;
    
    return server_->get_fd();
}

int Tunnel::get_clientfd() noexcept {
    return client_->get_fd();
}

string Tunnel::get_client_connected_ip() noexcept {
    return client_->connected_address_;
}
uint16_t Tunnel::get_client_connected_port() noexcept {
    return client_->connected_port_;
}

bool Tunnel::is_disconnect() noexcept {
    return state_ == TunnelState::DISCONNECT;
}
#include "socks_client.h"
#include "ip.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <endian.h>
#include <unistd.h>
#include <sstream>

using namespace socks_client;
using namespace address;

using std::string, std::stringstream;

SocksClient::SocksClient(unsigned char version = 5): client_socks_version_{version}, start_sockfd_{-1}, server_sockfd_{-1} {}

int SocksClient::connect_to_server(const string &socks_server_ip, const uint16_t socks_server_port) const {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw "aboba";
        //throw OpenSocketException {strerror(errno), errno};
    
    sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(socks_server_port);
    server_sockaddr.sin_addr.s_addr = inet_addr(socks_server_ip.c_str());

    if (
        connect(sockfd,
                (const sockaddr *) &server_sockaddr,
                (socklen_t) sizeof(server_sockaddr))
        < 0
    ) throw "aboba";//throw ConnectFailedException {strerror(errno), errno};

    return sockfd;
}

void SocksClient::make_connection(const string &socks_server_ip, const uint16_t socks_server_port) {
    start_sockfd_ = connect_to_server(socks_server_ip, socks_server_port);

    //client_hello_message
    unsigned char message[2] = {0x05, 0x00};
    if (send(start_sockfd_, message, 2, 0) < 0)
        throw "something";
    
    //server_hello_answer
    unsigned char first_answer[2];
    if (recv(start_sockfd_, first_answer, 2, 0) < 0)
        throw "something";
    
    if (compare_versions(first_answer[0]))
        throw "something";
    
    if (need_autentification(first_answer[1]))
        throw "something";
    
    unsigned char request[10] =
    {
        0x05, // protocol version
        0x01, // establish TCP/IP connection
        0x00, // special byte
        0x01, // IPv4 addres (TODO add domain names)
        0x00,
        0x00,
        0x00,
        0x00, // for IPv4 address
        0x00,
        0x00 // for port
    };

    uint16_t server_port_in_big_endian = htobe16(socks_server_port);

    string ip_part;
    stringstream stream {socks_server_ip};
    for (int i = 4; std::getline(stream, ip_part, '.'); ++i) {
        request[i] = static_cast<unsigned char>(std::atoi(ip_part.c_str()));
    }

    request[8] = server_port_in_big_endian & 0b1111111100000000;
    request[9] = server_port_in_big_endian & 0b0000000011111111;

    if (send(start_sockfd_, request, 10, 0) < 0)
        throw "something";
    
    unsigned char second_answer[10];
    if (recv(start_sockfd_, second_answer, 10, 0) < 0)
        throw "something";
    
    if (compare_versions(second_answer[0]))
        throw "something";

    // check errors
    if (second_answer[1] != 0x00)
        throw "something";

    string address;
    uint16_t port;
    if (second_answer[3] == 0x01) {
        address = std::to_string(second_answer[4]) + "." +
                        std::to_string(second_answer[5]) + "." +
                        std::to_string(second_answer[6]) + "." +
                        std::to_string(second_answer[7]);
        port = (second_answer[8] & 0b1111111100000000) | (second_answer[9] & 0b0000000011111111);
        port = be16toh(port);
    } else {
        // Add domain parse
    }

    server_sockfd_ = connect_to_server(address, port);
}

void SocksClient::send_to_server(const void *buf, size_t n) const {
    if (send(server_sockfd_, buf, n, 0) < 0)
        throw "something";
}

void SocksClient::recv_from_server(void *buf, size_t n) const {
    if (recv(server_sockfd_, buf, n, 0) < 0)
        throw "something";
}

SocksClient::~SocksClient() {
    if (start_sockfd_ != -1)
        close(start_sockfd_);
    
    if (server_sockfd_ != -1)
        close(server_sockfd_);
}
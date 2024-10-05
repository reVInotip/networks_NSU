#include "udp_socket.h"
#include <string.h>

using namespace udp_socket;

SocketException::SocketException(const std::string& message, const int err_code) {
    message_ = "Something went wrong: " + message + " " + std::to_string(err_code) + "\n";
}

SocketException::SocketException(const std::string& message) {
    message_ = message + "\n";
}

const char* SocketException::what() const noexcept {
    return message_.c_str();
}

OpenSocketException::OpenSocketException(const std::string& message, const int err_code)
{
    message_ = "Error during opening the socket: " + message + ".\n Programm failed with error code: "
                + std::to_string(err_code);
}

IncorrectIpAddrException::IncorrectIpAddrException(const std::string& message):
    SocketException(message) {}

RecvfromFailedException::RecvfromFailedException(const std::string& message, const int err_code)
{
    message_ = "Error during receive message from server: " + message + ".\n Programm failed with error code: "
                + std::to_string(err_code);
}

BindFailedException::BindFailedException(const std::string& message, const int err_code)
{
    message_ = "Error during bind socket: " + message + ".\n Programm failed with error code: "
                + std::to_string(err_code);
}

SetsockoptFailedException::SetsockoptFailedException(const std::string& message, const int err_code)
{
    message_ = "Error during set socket options: " + message + ".\n Programm failed with error code: "
                + std::to_string(err_code);
}

SendtoFailedException::SendtoFailedException(const std::string& message, const int err_code)
{
    message_ = "Error during send to client: " + message + ".\n Programm failed with error code: "
                + std::to_string(err_code);
}
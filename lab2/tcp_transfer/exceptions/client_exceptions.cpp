#include "tcp_client.h"
#include <iostream>

using namespace client;

ClientException::ClientException(const std::string &message) {
    message_ = "Some error occured: " + message + "\n";
}

const char *ClientException::what() const noexcept {
    return message_.c_str();
}

OpenSocketException::OpenSocketException(const std::string& message, const int err_code):
    ClientException(message + ". Error code: " + std::to_string(err_code)) {}

ConnectFailedException::ConnectFailedException(const std::string& message, const int err_code):
    ClientException(message + ". Error code: " + std::to_string(err_code)) {}

RecvFailedException::RecvFailedException(const std::string& message, const int err_code):
    ClientException(message + ". Error code: " + std::to_string(err_code)) {}

SendFailedException::SendFailedException(const std::string& message, const int err_code):
    ClientException(message + ". Error code: " + std::to_string(err_code)) {}

OpenFileException::OpenFileException(): ClientException("Can not open file\n") {}

FileNotExistsException::FileNotExistsException(): ClientException("File does not exists\n") {}
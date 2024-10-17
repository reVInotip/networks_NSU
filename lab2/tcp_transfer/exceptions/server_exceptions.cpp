#include "tcp_server.h"
#include <iostream>

using namespace server;

ServerException::ServerException(const std::string &message) {
    message_ = "Some error occured: " + message + "\n";
}

const char *ServerException::what() const noexcept {
    return message_.c_str();
}

OpenSocketException::OpenSocketException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

BindFailedException::BindFailedException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

SetListenFailedException::SetListenFailedException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

RecvFailedException::RecvFailedException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

SendFailedException::SendFailedException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

AcceptFailedException::AcceptFailedException(const std::string& message, const int err_code):
    ServerException(message + ". Error code: " + std::to_string(err_code)) {}

OpenFileException::OpenFileException(): ServerException("Can not open file\n") {}

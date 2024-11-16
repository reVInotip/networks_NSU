#include <string>
#include <variant>
#include <netinet/in.h>

#pragma once

namespace address {
    enum class AddrType {
        IPv4,
        DOMAIN,
        UNDEFINED
    };

    using enum AddrType;

    struct IP { // IPv4
        int          port = 0;
        in_addr_t ip_addr = 0;
    };

    struct Address {
        AddrType type = UNDEFINED;
        std::variant<IP, std::string> address;
    };
}
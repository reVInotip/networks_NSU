#include <iostream>
#include <string>
#include "multicast_app.h"

using std::string;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Low count arguments\n";
        return 1;
    }
    string ipaddr {argv[1]};

    MulticastApp app {ipaddr};
    app.run();

    return 0;
} 
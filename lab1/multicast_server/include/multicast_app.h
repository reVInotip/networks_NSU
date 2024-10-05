#include "client.h"
#include "server.h"
#include <thread>

class MulticastApp {
    private:
        server::Server server_;
        client::Client client_;
    
    public:
        MulticastApp(const string &multicast_ip);
        void run();
};
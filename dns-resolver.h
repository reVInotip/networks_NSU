#include <ares.h>
#include <cstring>
#include <iostream>

#pragma once

struct DNSResolver {
    ares_channel channel_;
    struct ares_options options_;
    int optmask_ = 0;
    struct ares_addrinfo_hints hints_;

    DNSResolver() {
        ares_library_init(ARES_LIB_INIT_ALL);

        /* Enable event thread so we don't have to monitor file descriptors */
        memset(&options_, 0, sizeof(options_));
        optmask_ |= ARES_OPT_SOCK_STATE_CB;


        memset(&hints_, 0, sizeof(hints_));
        hints_.ai_family = AF_INET;
        hints_.ai_flags = ARES_AI_CANONNAME;

        /* Initialize channel to run queries, a single channel can accept unlimited
        * queries */
        if (ares_init_options(&channel_, &options_, optmask_) != ARES_SUCCESS) {
            std::cerr << "c-ares initialization issue\n";
            throw "exception";
        }
    }

    ~DNSResolver() {
        /* Cleanup */
        ares_destroy(channel_);

        ares_library_cleanup();
    }
};
/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "handshake_types.hpp"

namespace proxy::tcp::tls::handshake {
    bool is_valid(cipher_suite_name cipher) {
        switch (cipher) {
#define X(id, name) case cipher_suite_name::name:
            CIPHER_SUITE_NAMES(X)
#undef X
                return true;
            default: return false;
        }
    }
}
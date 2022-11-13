/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "handshake_types.hpp"

#include <iostream>

namespace proxy::tcp::tls::handshake {

bool cipher_is_valid(cipher_suite_name cipher) {
  switch (cipher) {
#define X(id, name, str) case cipher_suite_name::name:
    CIPHER_SUITE_NAMES(X)
#undef X
    return true;
    default:
      return false;
  }
}

std::ostream& operator<<(std::ostream& output, cipher_suite_name cipher) {
  output << cipher_to_string(cipher);
  return output;
}

}  // namespace proxy::tcp::tls::handshake

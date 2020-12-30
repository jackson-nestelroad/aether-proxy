/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "opcode.hpp"
#include "close_code.hpp"

namespace proxy::tcp::websocket {
    namespace convert {
        std::string_view to_string(opcode o) {
            switch (o) {
#define X(name, num, string) case opcode::name: return string;
                WEBSOCKET_OPCODES(X)
#undef X
            }

            if (o > opcode::max) {
                throw error::websocket::invalid_opcode_exception { };
            }
            return "reserved";
        }

        std::string_view close_code_to_reason(close_code code) {
            switch (code) {
#define X(num, name, str) case close_code::name: return str;
                WEBSOCKET_CLOSE_CODES(X)
#undef X
            }
            return "Unknown Close Code";
        }
    }

    std::ostream &operator<<(std::ostream &output, opcode o) {
        output << static_cast<std::uint8_t>(o);
        return output;
    }

    std::ostream &operator<<(std::ostream &output, close_code code) {
        output << static_cast<std::int16_t>(code);
        return output;
    }
}
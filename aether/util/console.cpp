/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "console.hpp"

namespace out {
    const manip_t manip::endl = static_cast<manip_t>(std::endl);
    const manip_t manip::ends = static_cast<manip_t>(std::ends);
    const manip_t manip::flush = static_cast<manip_t>(std::flush);
}
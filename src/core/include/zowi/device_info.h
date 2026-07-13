#pragma once

#include <string>

namespace zowi {

struct DeviceInfo {
    std::string name;
    std::string address;
    int rssi = 0;

    bool isValid() const { return !address.empty(); }

    bool operator==(const DeviceInfo &other) const {
        return address == other.address;
    }
};

} // namespace zowi

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>
#include <QHash>

struct DeviceInfo {
    QString name;
    QString address;
    int rssi = 0;

    bool isValid() const { return !address.isEmpty(); }
};

inline uint qHash(const DeviceInfo &d, uint seed = 0) {
    return qHash(d.address, seed);
}

inline bool operator==(const DeviceInfo &a, const DeviceInfo &b) {
    return a.address == b.address;
}

#endif

#include "zowi/stk500v1.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <thread>

// ── STK500v1 / Optiboot protocol constants (bq/protocol-stk-500-v1) ──
namespace {
constexpr uint8_t STK_GET_SYNC        = 0x30;
constexpr uint8_t STK_ENTER_PROGMODE  = 0x50;
constexpr uint8_t STK_LEAVE_PROGMODE  = 0x51;
constexpr uint8_t STK_LOAD_ADDRESS    = 0x55;
constexpr uint8_t STK_UNIVERSAL       = 0x56;
constexpr uint8_t STK_PROG_PAGE       = 0x64;
constexpr uint8_t STK_INSYNC          = 0x14;
constexpr uint8_t STK_OK              = 0x10;
constexpr uint8_t CRC_EOP             = 0x20;
constexpr uint8_t STK_MEMTYPE_FLASH   = 'F';

// Flash page size used when grouping HEX data for STK_PROG_PAGE.
constexpr std::size_t kStkPageSize = 128;

// ── Intel HEX parsing (mirrors bq Hex.java: data records concatenated in file
//    order, standard Intel checksum verified per record) ──────────────────
struct HexRecord {
    uint32_t address = 0;
    std::vector<uint8_t> data;
};

int hexNibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseIntelHex(const std::string &path, std::vector<HexRecord> &records)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Failed to open firmware file: " << path << std::endl;
        return false;
    }

    std::string line;
    uint32_t linearBase = 0;
    uint32_t segmentBase = 0;

    auto nib = [&](const std::string &s, std::size_t pos) -> int {
        if (pos + 2 > s.size()) return -1;
        const int hi = hexNibble(s[pos]);
        const int lo = hexNibble(s[pos + 1]);
        if (hi < 0 || lo < 0) return -1;
        return (hi << 4) | lo;
    };

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] != ':') continue;

        const int count = nib(line, 1);
        const int addrHi = nib(line, 3);
        const int addrLo = nib(line, 5);
        const int type = nib(line, 7);
        if (count < 0 || addrHi < 0 || addrLo < 0 || type < 0) {
            std::cerr << "Malformed HEX line in " << path << std::endl;
            return false;
        }

        const uint32_t addr = static_cast<uint32_t>((addrHi << 8) | addrLo)
                              + linearBase + segmentBase;

        if (type == 0) {
            std::vector<uint8_t> data;
            data.reserve(static_cast<std::size_t>(count));
            int sum = count + addrHi + addrLo + type;
            bool ok = true;
            for (int i = 0; i < count; i++) {
                const int v = nib(line, 9 + static_cast<std::size_t>(i) * 2);
                if (v < 0) { ok = false; break; }
                data.push_back(static_cast<uint8_t>(v));
                sum += v;
            }
            const int cs = nib(line, 9 + static_cast<std::size_t>(count) * 2);
            if (!ok || cs < 0 || ((sum + cs) & 0xFF) != 0) {
                std::cerr << "Malformed or bad-checksum HEX data in " << path << std::endl;
                return false;
            }
            records.push_back({addr, std::move(data)});
        } else if (type == 1) {
            break; // EOF
        } else if (type == 2) {
            const int segHi = nib(line, 9);
            const int segLo = nib(line, 11);
            if (segHi < 0 || segLo < 0) return false;
            segmentBase = static_cast<uint32_t>((segHi << 8) | segLo) * 16;
        } else if (type == 4) {
            const int linHi = nib(line, 9);
            const int linLo = nib(line, 11);
            if (linHi < 0 || linLo < 0) return false;
            linearBase = (static_cast<uint32_t>((linHi << 8) | linLo) << 16);
        } else if (type == 3 || type == 5) {
            // start address records: ignored
        }
    }

    return true;
}

// Merge parsed records into flash pages (<= kStkPageSize bytes), preserving real
// addresses and breaking on gaps so the bootloader writes to the correct location.
std::vector<HexRecord> buildFlashPages(const std::vector<HexRecord> &records)
{
    std::vector<HexRecord> pages;
    HexRecord cur;
    for (const auto &rec : records) {
        if (rec.data.empty()) continue;
        std::size_t off = 0;
        while (off < rec.data.size()) {
            if (cur.data.empty()) {
                cur.address = rec.address + static_cast<uint32_t>(off);
            } else if (rec.address + off != cur.address + cur.data.size()) {
                pages.push_back(std::move(cur));
                cur.address = rec.address + static_cast<uint32_t>(off);
            }
            std::size_t room = kStkPageSize - cur.data.size();
            if (room == 0) {
                pages.push_back(std::move(cur));
                cur.address = rec.address + static_cast<uint32_t>(off);
                room = kStkPageSize;
            }
            const std::size_t take = std::min(rec.data.size() - off, room);
            cur.data.insert(cur.data.end(), rec.data.begin() + off, rec.data.begin() + off + take);
            off += take;
        }
    }
    if (!cur.data.empty()) pages.push_back(std::move(cur));
    return pages;
}

// ── Transport helpers ───────────────────────────────────────────
bool stkSend(const zowi::BootloaderTransport &t, const std::vector<uint8_t> &cmd)
{
    return t.send(cmd);
}

// Read an INSYNC (0x14) followed by a status byte, tolerating stray bytes before
// the sync marker. Returns true when the status is STK_OK (0x10).
bool readStkReply(const zowi::BootloaderTransport &t, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    bool gotSync = false;
    std::vector<uint8_t> buf;
    while (std::chrono::steady_clock::now() < deadline) {
        t.pump();
        buf.clear();
        const int n = t.receive(buf, 256);
        if (n > 0) {
            for (uint8_t b : buf) {
                if (!gotSync) {
                    if (b == STK_INSYNC) gotSync = true;
                } else {
                    return b == STK_OK;
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return false;
}

// Read N bytes after the first INSYNC, returning the bytes (skipping leading
// non-sync junk). Used by chipEraseUniversal which expects INSYNC, <byte>, OK.
bool readStkReplyN(const zowi::BootloaderTransport &t, int timeoutMs,
                   std::vector<uint8_t> &out)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    out.clear();
    bool gotSync = false;
    std::vector<uint8_t> buf;
    while (std::chrono::steady_clock::now() < deadline) {
        t.pump();
        buf.clear();
        const int n = t.receive(buf, 256);
        if (n > 0) {
            for (uint8_t b : buf) {
                if (!gotSync) {
                    if (b == STK_INSYNC) { gotSync = true; out.push_back(b); }
                } else {
                    out.push_back(b);
                }
            }
            if (gotSync && out.size() >= 3) return true;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return gotSync && out.size() >= 3;
}

bool stkCommand(const zowi::BootloaderTransport &t, const std::vector<uint8_t> &cmd, int timeoutMs)
{
    if (!stkSend(t, cmd)) return false;
    return readStkReply(t, timeoutMs);
}

bool stkGetSync(const zowi::BootloaderTransport &t)
{
    return stkCommand(t, {STK_GET_SYNC, CRC_EOP}, 1500);
}

bool stkEnterProgMode(const zowi::BootloaderTransport &t)
{
    return stkCommand(t, {STK_ENTER_PROGMODE, CRC_EOP}, 1500);
}

// Chip erase via the universal command (0xAC 0x80 0x00 0x00). Optiboot echoes
// INSYNC, <byte>, OK.
bool stkChipErase(const zowi::BootloaderTransport &t)
{
    if (!stkSend(t, {STK_UNIVERSAL, 0xAC, 0x80, 0x00, 0x00, CRC_EOP}))
        return false;
    std::vector<uint8_t> resp;
    if (!readStkReplyN(t, 3000, resp)) return false;
    return resp.size() >= 3 && resp[0] == STK_INSYNC && resp[2] == STK_OK;
}

bool stkLoadAddress(const zowi::BootloaderTransport &t, uint32_t byteAddr)
{
    const uint16_t wordAddr = static_cast<uint16_t>(byteAddr / 2);
    return stkCommand(t,
                      {STK_LOAD_ADDRESS,
                       static_cast<uint8_t>(wordAddr & 0xFF),
                       static_cast<uint8_t>((wordAddr >> 8) & 0xFF),
                       CRC_EOP},
                      1500);
}

bool stkProgramPage(const zowi::BootloaderTransport &t, const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> cmd;
    cmd.reserve(data.size() + 5);
    cmd.push_back(STK_PROG_PAGE);
    cmd.push_back(static_cast<uint8_t>((data.size() >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    cmd.push_back(STK_MEMTYPE_FLASH);
    cmd.insert(cmd.end(), data.begin(), data.end());
    cmd.push_back(CRC_EOP);
    return stkCommand(t, cmd, 3000);
}

bool stkLeaveProgMode(const zowi::BootloaderTransport &t)
{
    return stkCommand(t, {STK_LEAVE_PROGMODE, CRC_EOP}, 1500);
}

} // namespace

namespace zowi {

bool stk500UploadFirmware(const zowi::BootloaderTransport &transport,
                           const std::string &hexPath)
{
    std::vector<HexRecord> records;
    if (!parseIntelHex(hexPath, records)) return false;

    const std::vector<HexRecord> pages = buildFlashPages(records);
    if (pages.empty()) {
        std::cerr << "No data records found in firmware: " << hexPath << std::endl;
        return false;
    }

    std::size_t totalBytes = 0;
    for (const auto &p : pages) totalBytes += p.data.size();

    // The robot is assumed to already be in its bootloader (the caller reset it
    // into the bootloader by pulsing DTR when the serial port was opened — exactly
    // like the bq/protocol-stk-500-v1 library, which does NOT send a soft-reset
    // sequence; the hardware reset is what enters the bootloader).

    bool synced = false;
    for (int i = 0; i < 10 && !synced; i++) {
        synced = stkGetSync(transport);
        if (!synced) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!synced) {
        std::cerr << "Failed to synchronize with the bootloader (no STK_INSYNC/STK_OK)."
                  << std::endl;
        return false;
    }
    std::cout << "Bootloader synchronized." << std::endl;

    bool entered = false;
    for (int i = 0; i < 5 && !entered; i++) {
        entered = stkEnterProgMode(transport);
    }
    if (!entered) {
        std::cerr << "Failed to enter programming mode." << std::endl;
        return false;
    }

    std::cout << "Uploading firmware from " << hexPath << "..." << std::endl;
    std::size_t sentBytes = 0;
    int lastPercent = -1;
    for (const auto &page : pages) {
        if (!stkLoadAddress(transport, page.address)) {
            std::cerr << "\nFailed to load address 0x" << std::hex << page.address << std::dec
                      << std::endl;
            return false;
        }
        if (!stkProgramPage(transport, page.data)) {
            std::cerr << "\nFailed to program page at 0x" << std::hex << page.address << std::dec
                      << std::endl;
            return false;
        }
        sentBytes += page.data.size();
        const int percent = totalBytes > 0
                                ? static_cast<int>((100.0 * sentBytes) / totalBytes)
                                : 100;
        if (percent == 100 || percent >= lastPercent + 5) {
            std::cout << "\r  Progress: " << percent << "%" << std::flush;
            lastPercent = percent;
        }
        if (transport.progress) transport.progress(percent, sentBytes, totalBytes);
        transport.pump();
    }

    std::cout << "\r  Progress: 100%" << std::endl;

    bool left = false;
    for (int i = 0; i < 3 && !left; i++) {
        left = stkLeaveProgMode(transport);
    }
    if (!left) {
        std::cerr << "Warning: failed to leave programming mode cleanly." << std::endl;
    }

    return true;
}

bool zowiRawHexUploadFirmware(const zowi::BootloaderTransport &transport,
                               const std::string &hexPath)
{
    std::ifstream f(hexPath, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "Failed to open firmware file: " << hexPath << std::endl;
        return false;
    }

    std::cout << "Streaming firmware to bootloader..." << std::endl;

    // File size for progress reporting.
    f.seekg(0, std::ios::end);
    const std::size_t totalSize = static_cast<std::size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    std::cout << "Uploading firmware from " << hexPath << "..." << std::endl;
    char buffer[256];
    std::size_t totalSent = 0;
    int lastPercent = -1;
    while (f.good()) {
        f.read(buffer, sizeof(buffer));
        const std::streamsize n = f.gcount();
        if (n <= 0) break;

        std::vector<uint8_t> chunk(buffer, buffer + static_cast<std::size_t>(n));
        if (!transport.send(chunk)) {
            std::cerr << "\nFailed to send firmware chunk." << std::endl;
            return false;
        }

        totalSent += static_cast<std::size_t>(n);
        const int percent = totalSize > 0
                                ? static_cast<int>((100.0 * totalSent) / totalSize)
                                : 100;
        if (percent == 100 || percent >= lastPercent + 5) {
            std::cout << "\r  Progress: " << percent << "%" << std::flush;
            lastPercent = percent;
        }
        transport.pump();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "\r  Progress: 100%" << std::endl;
    return true;
}

} // namespace zowi

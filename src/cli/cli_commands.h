#ifndef ZOWI_CLI_COMMANDS_H
#define ZOWI_CLI_COMMANDS_H

#include <string>

// One handler per CLI subcommand. Each receives the already-parsed option
// values captured by main() and returns a process exit code. Handlers that
// need the Qt event loop take argc/argv so they can build a QCoreApplication.
namespace zowi_cli {

struct SessionArgs {
    std::string getKey;
    std::string setKey;
    std::string setValue;
    bool get = false;
    bool set = false;
    bool list = false;
};

struct TranslateArgs {
    std::string locale;
    std::string context;
    std::string source;
};

struct ConfigArgs {
    std::string key;
    bool get = false;
    bool list = false;
};

struct ScanArgs {
    int timeout = 5;
    bool filterName = true;
    bool filterMac = true;
};

struct ConnectArgs {
    std::string address;
    std::string tty;
    int baud = 9600;
    std::string backend = "auto";
    int timeout = 3;
};

struct RenameArgs {
    std::string name;
    std::string tty;
    int baud = 9600;
    std::string backend = "auto";
    int timeout = 3;
};

// Shared options for the three firmware-install subcommands.
struct FirmwareArgs {
    std::string firmwarePath;
    int timeout = 10;
    int batteryTimeout = 2;
    bool forceLowBattery = false;
    std::string protocol = "stk";
    std::string tty;
    int baud = 9600;
    std::string address;
    std::string backend = "auto";
};

struct ControlArgs {
    std::string address;
    std::string speed = "medium";
    int timeout = 3;
};

struct StatusArgs {
    std::string tty;
    int baud = 9600;
    std::string backend = "auto";
    int timeout = 3;
};

int runSession(const SessionArgs &a);
int runTranslate(const TranslateArgs &a);
int runConfig(const ConfigArgs &a);
int runPorts();
int runScan(int argc, char **argv, const ScanArgs &a);
int runConnect(int argc, char **argv, const ConnectArgs &a);
int runRename(int argc, char **argv, const RenameArgs &a);
int runFirmware(int argc, char **argv, const FirmwareArgs &a, const std::string &actionLabel);
int runDisconnect(int argc, char **argv);
int runStatus(int argc, char **argv, const StatusArgs &a);
int runControl(int argc, char **argv, const ControlArgs &a);

} // namespace zowi_cli

#endif // ZOWI_CLI_COMMANDS_H

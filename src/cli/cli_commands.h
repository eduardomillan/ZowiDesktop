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
    bool clear = false;
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
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
};

struct StatusArgs {
    std::string tty;
    int baud = 9600;
    std::string backend = "auto";
    int timeout = 3;
};

// Interactive servo-trim calibration (C/G protocol). The wizard clamps trims
// to [-60, 60]; pass all four trims to apply them non-interactively.
struct CalibrateArgs {
    std::string address;
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
    int timeout = 3;
    bool noVictory = false;
    bool direct = false;
    int yl = 0;
    int yr = 0;
    int rl = 0;
    int rr = 0;
};

// One-shot movement command (M protocol). Runs the requested number of gait
// cycles and stops the robot afterwards.
struct MoveArgs {
    std::string direction;  // forward/fw, backward/bk, left/lf, right/rg, moonwalker-left/ml, moonwalker-right/mr
    int cycles = 1;         // gait cycles (>= 1); the robot stops automatically afterwards
    std::string speed = "medium";  // slow/s, medium/m, fast/f
    std::string address;
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
    int timeout = 3;
    bool list = false;
};

// One-shot gesture command (H protocol). Sends a gesture and exits.
struct GestureArgs {
    std::string gesture;  // name or numeric id (1-13)
    std::string address;
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
    int timeout = 3;
    bool list = false;
};

// One-shot mouth command (L protocol). Sends a mouth pattern and exits.
struct MouthArgs {
    std::string mouth;  // name or numeric id (0-30)
    std::string address;
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
    int timeout = 3;
    bool list = false;
};

// One-shot sing/melody command (K protocol). Sends a melody and exits.
struct SingArgs {
    std::string melody;  // name or numeric id (1-19)
    std::string address;
    std::string tty;
    int baud = 115200;
    std::string backend = "auto";
    int timeout = 3;
    bool list = false;
};

// Persistent-connection command shell. Connects once, then reads command
// lines from stdin (REPL when interactive, batch when piped) and executes
// each against the open connection. See docs/project/ZOWI_CLI_SHELL.md.
struct ShellArgs {
    std::string address;
    std::string tty;
    int baud = 115200;
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
int runCalibrate(int argc, char **argv, const CalibrateArgs &a);
int runMove(int argc, char **argv, const MoveArgs &a);
int runGesture(int argc, char **argv, const GestureArgs &a);
int runMouth(int argc, char **argv, const MouthArgs &a);
int runSing(int argc, char **argv, const SingArgs &a);
int runShell(int argc, char **argv, const ShellArgs &a);

} // namespace zowi_cli

#endif // ZOWI_CLI_COMMANDS_H

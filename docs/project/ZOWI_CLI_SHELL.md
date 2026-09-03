# Zowi CLI Shell — Design

Status: implemented (v1). Design agreed before implementation; this document
records the decisions so the future daemon work can build on them.


## Table of Contents

- [Motivation](#motivation)
- [Goals and non-goals](#goals-and-non-goals)
- [Options considered](#options-considered)
- [UX specification](#ux-specification)
- [Command set](#command-set)
- [Technical design](#technical-design)
- [Limitations](#limitations)
- [Testing plan](#testing-plan)
- [Future work: daemon mode](#future-work-daemon-mode)


## Motivation

Every action-oriented CLI subcommand (`move`, `gesture`, `mouth`, `sing`) is a
one-shot process: it goes through `sendOneShotCommand()`, which connects,
sends a single command, waits for the final ack and disconnects. Each
invocation therefore pays the full connection cost:

- Bluetooth SPP connect, plus the STATE-pin reset cycle: the first connect
  makes the robot reboot, the backend auto-reconnects, and the link has to
  settle before it is usable (see the stabilisation loop in
  `installFirmwareToPairedZowi`, `cli_util.cpp`).
- USB serial: the robot needs time to boot and start emitting data, so the
  default timeout is raised to 8 s.

Chaining a gesture, a mouth pattern and a melody means three connections and
three robot reset cycles, several seconds apart, for what is conceptually one
performance. A persistent connection removes that latency: connect once, then
fire commands as fast as the robot accepts them.


## Goals and non-goals

**Goals**

- Keep a single connection open while several commands are sent in sequence.
- Support interactive use (human at the terminal) and scripted use (commands
  piped from another process) with the same code path.
- Reuse the existing protocol builders and connection helpers; no new
  protocol or backend work.
- Keep the change small and aligned with the existing command patterns
  (`control` and `calibrate` already hold long-lived interactive sessions).

**Non-goals (v1)**

- Multi-client access: only one process can hold the SPP/serial channel at a
  time (the GUI competes for the same channel; this is true today for every
  command, the shell just holds it longer).
- Command history, completion or fancy line editing.
- A background daemon that outlives the invoking shell (deferred, see
  [Future work](#future-work-daemon-mode)).


## Options considered

| Option | Description | Verdict |
|--------|-------------|---------|
| A. Interactive shell subcommand | New `shell` subcommand: connect once, read command lines from stdin, execute each against the open connection | **Chosen** |
| B. Batch mode via stdin | Same loop, but stdin is a pipe/file: read lines until EOF. Falls out of A almost for free (the codebase already branches on `isatty`) | **Chosen** (comes with A) |
| C. Daemon + Unix socket | `connect --daemon` holds the connection in the background; other processes send commands over IPC | Deferred: protocol, lifecycle and locking are a project of their own, and the daemon would fight the GUI for the channel |


## UX specification

### Interactive mode

```console
$ zowi_cli shell
Connecting to B4:9D:0B:32:41:0E...
Connected.
  Name:    Zowi
  App ID:  ZOWI_ALLOCATOR_V2
  Battery: 87%

Commands: move <dir> [speed] | gesture <name|id> | mouth <name|id> |
          sing <name|id> | stop | status | help | quit
ZOWI_CLI:Zowi> gesture happy
Sent: gesture happy
ZOWI_CLI:Zowi> mouth heart
Sent: mouth heart
ZOWI_CLI:Zowi> move forward fast
Sent: move forward
ZOWI_CLI:Zowi> stop
Sent: stop
ZOWI_CLI:Zowi> quit
Disconnected. Bye!
```

- A prompt of the form `ZOWI_CLI:<name>` — with the connected robot's name,
  e.g. `ZOWI_CLI:Zowi>` when the name was never received — is printed when
  stdin is a TTY.
- Lines are read with `std::getline`; no raw terminal mode is needed.
- Empty lines are ignored; lines whose first non-blank character is `#` are
  treated as comments (useful in scripts).
- On `quit`, `exit`, `q` or EOF the robot is sent a `stop` command (if still
  connected) and the link is closed, releasing any bound RFCOMM TTY.

### Batch mode

When stdin is not a TTY the same loop reads until EOF, which makes the shell
usable from scripts and other processes:

```bash
printf "gesture 3\nmouth 5\nsing 4\n" | zowi_cli shell
zowi_cli shell < sequence.txt
```

Output is identical to interactive mode, minus the prompt. The exit code is
`0` when the session ran (individual unknown commands are reported but do not
abort the batch), `1` when the connection could not be established.


## Command set

| Command | Action | Protocol |
|---------|--------|----------|
| `move <dir> [speed]` | One movement; `dir` ∈ forward, backward, left, right, moonwalker-left, moonwalker-right; `speed` ∈ slow, medium, fast | M |
| `gesture <name\|id>` | Play a gesture (1–13, same names as `zowi_cli gesture --list`) | H |
| `mouth <name\|id>` | Show a mouth/LED pattern (0–30) | L |
| `sing <name\|id>` | Play a melody (1–19) | K |
| `stop` | Stop movement | M |
| `status` | Print the cached robot identity (name, app ID, battery) without sending anything | — |
| `help` | Print the command summary | — |
| `quit`, `exit`, `q` | Disconnect and leave | — |

Name-to-id parsing uses exactly the same tables and mapping as the one-shot
subcommands (`parseNameOrId` + the shared name tables), so `gesture happy`,
`gesture 1`, `mouth heart` and `mouth 13` behave identically in both places.


## Technical design

### Files touched

| File | Change |
|------|--------|
| `src/cli/cli_commands.h` | `ShellArgs` struct + `runShell()` declaration |
| `src/cli/cli_commands.cpp` | Hoisted name tables; command-builder helpers shared with the one-shot commands; `runShell()` implementation |
| `src/cli/main.cpp` | `shell` subcommand registration (options mirror `move`/`gesture`) and dispatch |
| `docs/project/ZOWI_CLI_SHELL.md` | This document |

### Shared helpers (refactor, no behaviour change)

The gesture/mouth/melody name tables currently live as function-local
statics inside `runGesture`/`runMouth`/`runSing`. They are hoisted to
file-scope constants so both the one-shot commands and the shell parse names
identically. Thin builders encapsulate the id math (gesture protocol ids are
1-based, mouth/melody enums are 0-based):

```cpp
static bool buildMoveCommand(const std::string &direction,
                             const std::string &speed,
                             std::string &cmd);
static bool buildGestureCommand(const std::string &token,
                                std::string &cmd, std::string &desc);
static bool buildMouthCommand(const std::string &token,
                              std::string &cmd, std::string &desc);
static bool buildSingCommand(const std::string &token,
                             std::string &cmd, std::string &desc);
```

`runMove`/`runGesture`/`runMouth`/`runSing` are rewritten in terms of these
helpers; their CLI behaviour (error messages, `--list`, exit codes) is
unchanged.

### Connection flow (`runShell`)

1. Resolve the backend: explicit `--backend`, else the transport registered
   in the session (`activeZowiTransport`, `bt` → `bluetooth`), else
   Bluetooth — the same resolution `sendOneShotCommand` performs.
2. Resolve the target address: `--address`, else `activeZowiDeviceAddress`.
   Error out with "Run 'connect' first" if neither is set.
3. `prepareFlashBackend()` → Qt Bluetooth SPP or serial backend; for the
   non-native Qt backend run `discoverDevice()` first (BlueZ requirement).
4. Connect and wait for `g_connected` (`waitUntil`), with the USB timeout
   raised to at least 8 s. On failure: disconnect, release any bound RFCOMM
   TTY, exit `1`.
5. Identify the robot: `waitForRobotIdentity()` polls the identity request
   every 500 ms until name / app ID / battery have all arrived or its window
   (floored at 10 s) closes — one burst can be lost while the robot boots
   (the USB port bounces on open and Bluetooth goes through the STATE-pin
   reset cycle). The same helper family (`waitForRobotIdentity` /
   `waitForRobotReady`) gates `connect`, `status` and the one-shot commands,
   which wait for the robot to answer before sending anything. The identity
   is printed, with a warning when the battery is below the low-battery
   threshold (same threshold as `control`).
6. REPL loop (below).
7. On exit: `commandStop()` if still connected, `disconnect()`, `rfcomm
   release 0` when a TTY was bound.

### REPL loop

```
while true:
    if interactive: print "ZOWI_CLI:<name>> " (flush)
    if not getline(cin, line): break            # EOF
    strip; skip empty and '#' comment lines
    if not connected (checked under g_mtx): report and break
    build (cmd, desc) from the verb + arguments via the shared builders
        unknown verb → error + hint, continue
    g_finalAck = false
    bt->send(cmd);  print "Sent: <desc>"
    waitUntil(qtApp, 2000ms, g_finalAck)         # best-effort ack wait
    qtApp.processEvents()
```

- Acks: each command waits up to 2 s for the firmware's final ack (`&&F`)
  before accepting the next line, so the robot is never flooded — the same
  pacing `sendOneShotCommand` uses.
- Disconnects are detected through the backend's `onConnectionChanged`
  callback (which mirrors `g_connected`); the loop checks the flag before
  every command.
- Input strategy: blocking `std::getline` in cooked mode. The Qt event loop
  is pumped by `waitUntil()` while a command is in flight, and once more
  after it completes. While the user idles at the prompt nothing is expected
  from the robot (the protocol is request/response), so a deferred
  disconnect notice (surfaced by the next command) is acceptable. This avoids
  a second hand-rolled line editor next to the raw-mode readers in
  `cli_util.cpp` and stays portable (Windows console + POSIX).

### Exit codes

- `0` — session completed (connection established, EOF or `quit`).
- `1` — no paired device / backend error / connection could not be
  established.


## Limitations

- One client at a time: while the shell is connected, the GUI and other CLI
  commands cannot open the robot.
- Disconnect detection while idling at the interactive prompt is deferred to
  the next command (see input strategy above).
- `status` reports the values cached at connect time; it does not re-poll
  the robot in v1.
- Ctrl-C during an interactive session terminates the process without an
  explicit disconnect handshake; the OS closes the socket, matching the
  behaviour of the one-shot commands.


## Testing plan

The CLI has no automated tests (the `ctest` suite covers the Qt-free core
only), so verification is:

1. `./build.sh --cli` compiles cleanly (also spot-check the Qt 5 path is
   unaffected — no new Qt API is used).
2. `ctest --test-dir build` — no core regressions.
3. Manual, with a robot over Bluetooth:
   - `zowi_cli shell` → `gesture happy`, `mouth heart`, `sing 4`,
     `move forward fast`, `stop`, `status`, `help`, `quit`.
   - Unknown command and `gesture nope` produce errors without dropping the
     connection.
   - `printf "gesture 3\nmouth 5\n" | zowi_cli shell` runs the batch and
     disconnects at EOF.
   - Commands work back-to-back without reconnect messages between them.
4. Manual, without a robot: `zowi_cli shell` with no session reports
   "No paired device found" and exits `1`.


## Future work: daemon mode

Option C was deferred, not discarded. When needed, the shell provides the
seam:

- Factor command execution out of the REPL into a
  `executeShellCommand(BluetoothApi &, const std::string &line)` function.
- A `shell --daemon` (or `connect --stay`) variant would then replace the
  stdin loop with a Unix-socket listener that feeds lines to the same
  function, plus:
  - a lockfile/socket path under `QStandardPaths::AppDataLocation`,
  - stale-daemon detection and an explicit `disconnect` verb,
  - a policy for GUI/CLI contention over the channel (refuse, or evict).

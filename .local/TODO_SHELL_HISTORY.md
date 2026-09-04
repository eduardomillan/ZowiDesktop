# TODO: Shell history for the CLI shell (arrow-up recall, bash-style)

Status: STUDY DONE, NOT IMPLEMENTED. This file holds the full analysis from a
plan-mode session so the work can start later without re-researching.
Decisions taken: (1) history in memory, per session (no persistence);
(2) Windows is planned for v2 — the POSIX-only v1 ships a graceful fallback,
and the Win32 branch will be implemented/tested from a Windows machine.

## Current state (as of commit bd57d35)

The shell loop lives in `runShell` (`src/cli/cli_commands.cpp:1772`):

- Input is read with `std::getline(std::cin, line)` in **canonical terminal
  mode**: backspace works (OS line discipline), but arrow keys arrive as
  literal bytes — pressing ↑ injects the raw escape sequence (`\x1b[A`, 3
  bytes) into the command line, which is then sent as an unknown command.
- The loop already distinguishes interactive vs piped input through
  `interactive = isatty(g_stdinFd)` (line 1757). Piped scripts
  (`{ echo ...; } | zowi_cli shell`) are exercised by ctest and **must keep
  working unchanged** — any history implementation only applies on the
  interactive path.

## Key finding: most of the terminal infrastructure already exists

Do NOT build raw-mode plumbing from scratch — it is already in the tree:

- `enableRawMode()` / `disableRawMode()` in `src/cli/cli_util.cpp`
  (cli_util.h:49-50). POSIX: termios with `ECHO|ICANON` off, `VMIN=0,
  VTIME=0` (non-blocking reads — cli_util.cpp:207-225). Windows: full Win32
  equivalents with `GetConsoleMode`/`SetConsoleMode` (cli_util.cpp:73-92).
- `readKey()` (cli_util.cpp:248-271 POSIX, 124-180 Windows) already decodes
  arrow-key escape sequences (ESC `[`/`O` + `A/B/C/D` → named keys) using a
  `select()`-based `readByte(ms)`. The keyboard-control (`control`/`keys`)
  and minigame commands already run inside this raw mode.
- SIGINT + atexit restore pattern in use (cli_commands.cpp:692-698 and
  931-933): `atexit(disableRawMode)` plus a `std::signal(SIGINT, ...)`
  handler that stores quit and calls `disableRawMode()`. The shell editor
  must mirror this — with a default SIGINT handler, dying while in raw mode
  would leave the user's terminal raw (the kernel does not restore termios).
- Raw mode uses a process-global static (`g_origTermios`/`g_rawMode`), but
  only one command runs per process, so no conflict with the shell editor.
- First unit-test target for the CLI does not exist yet — `src/cli/tests/`
  only holds hardware check scripts (blackbox/bt/usb). The new test target
  would go directly in `src/cli/CMakeLists.txt` (the executable is declared
  there; no test targets yet).

## Design

New files:

```
src/cli/shell_history.h/.cpp   (new)

class ShellHistory — pure logic, unit-testable:
    void  add(const std::string &line);  // skip empty, dedup consecutive,
                                         // cap at kMaxEntries (200)
    bool  empty() const;
    std::string up(const std::string &currentDraft);  // older entry; stores
                                         // the draft being typed when ↑ is
                                         // pressed for the first time
    std::string down();                  // newer entry; "" = back to draft
private:
    std::vector<std::string> m_lines;
    int m_index;                         // == size() → bottom
    std::string m_draft;
    static constexpr size_t kMaxEntries = 200;
```

```
src/cli/line_editor.h/.cpp      (new)

bool readShellLine(const std::string &prompt, ShellHistory &history,
                   std::string &out);
// Blocking line reader with history recall. POSIX path:
//   - runs inside the existing raw mode (needs a patient/blocking read:
//     select with a long/infinite timeout, since enableRawMode() sets
//     VMIN=0/VTIME=0 non-blocking)
//   - reuses readKey()'s escape-decode pattern for ↑/↓ (ESC [ A / ESC [ B)
//   - printable chars → append to buffer + manual echo
//   - backspace (0x7f / 0x08) → remove char + echo "\b \b"
//   - ↑/↓ → ShellHistory::up/down + redraw via "\r\x1b[K"
//   - Enter (0x0d / 0x0a) → return the line
//   - Ctrl-C → existing pattern: SIGINT handler sets quit + disableRawMode
//     (install the handler in the editor/runShell, mirroring line 695)
//   - Ctrl-D on an empty line → returns false (EOF; the shell breaks, same
//     as getline EOF today)
// Windows path (v1): graceful fallback to plain std::getline + a
// TODO(Win32) comment pointing at the existing enableRawMode()/VK-decode
// infrastructure to reuse for v2.
```

Integration in `runShell` (cli_commands.cpp:1772):

- `interactive && !WIN32` → `readShellLine(prompt, history, line)`; piped →
  current `std::getline` path untouched.
- The prompt text stays `ZOWI_CLI:<name>> ` but the editor draws it (today it
  is `std::cout << prompt << std::flush` + getline).
- Every accepted non-empty submitted line goes into the history (like bash,
  minus empty lines); `#` comment lines may be skipped — minor choice.
- Known interleaving (robot async messages, e.g. `&&M 512`, visually mixing
  with what is being typed): same as today, out of v1 scope; could be
  polished in v2 with a redraw-on-async-print hook.

## Alternatives considered and discarded

- GNU readline: system dependency + GPL licensing; too heavy.
- Vendored linenoise (~1100 lines C, BSD): robust but introduces a
  third_party convention the repo does not have yet, for ~150 lines of own
  code that suffice.

## Test strategy

- New `test_shell_history` unit-test target in `src/cli/CMakeLists.txt`
  (first CLI unit test): navigation, dedup, 200-entry cap, draft restore,
  behavior at top/bottom extremes.
- Existing ctest piped-script suite validates the non-interactive path.
- Manual interactive pass: ↑ recalls `move fw 3 s`, ↓ goes back to the
  draft, Enter re-executes; Ctrl-C exits cleanly with the terminal restored
  (check `stty` sanity after exit); Ctrl-D breaks the shell.

## Effort estimate

| Piece                                      | Time     |
|--------------------------------------------|----------|
| ShellHistory + unit tests                  | ~45 min  |
| readShellLine POSIX                        | ~1-1.25 h|
| Windows stub + fallback                    | ~15 min  |
| runShell integration                       | ~20 min  |
| Manual pass + docs + CHANGELOG             | ~20 min  |
| **Total**                                  | **~2.5-3 h** |

(Original estimate was 2-3 h with the editor built from scratch; reusing the
existing raw-mode/escape-decode infrastructure cuts it down.)

## Follow-ups (v2, out of scope)

- Win32 branch of readShellLine: reuse `enableRawMode()` (Win32 section,
  cli_util.cpp:73) + `KEY_EVENT_RECORD` VK decode (cli_util.cpp:94-122);
  implement and test from a Windows machine.
- Optional: persistence across sessions (~20-30 min) — decided against for
  now.
- Optional polish: clean redraw when robot async messages arrive while
  typing.

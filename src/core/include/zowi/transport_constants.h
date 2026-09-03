#pragma once

namespace zowi {

inline constexpr const char *kTransportUsb = "usb";
inline constexpr const char *kTransportBt  = "bt";

// Identity request (E/I/B) polling cadence, shared by the CLI ready-wait and
// the GUI data poll. Deliberately slow: every queued request makes the robot
// run zowi.home() (~500 ms) when it drains its RX queue, so a burst of polls
// delays and interrupts anything the robot is asked to do right after.
inline constexpr int kIdentityPollMs = 1200;

} // namespace zowi

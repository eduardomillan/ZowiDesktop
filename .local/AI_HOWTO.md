# AI Integration with Zowi — HowTO

## Zowi Capabilities Reference

Zowi communicates via a serial protocol (`&&cmd args%%`) over Bluetooth/USB. These are its built-in capabilities:

### Movement
- Walk forward/backward, turn left/right, moonwalk left/right
- Up/down, swing, crusaito forward/backward, jump
- Flapping (left/right), tiptoe swing, bend forward/backward
- Shake leg left/right, jitter, ascending turn
- Speed presets: Slow (2000ms), Medium (1000ms), Fast (700ms)

### Gestures (13)
Happy, SuperHappy, Sad, Sleeping, Fart, Confused, Love, Angry, Fretful, Magic, Wave, Victory, Fail

### Mouth/LED Matrix (31)
Zero–Nine, Smile, HappyOpen, HappyClosed, Heart, BigSurprise, SmallSurprise, TongueOut, Vamp1/2, LineMouth, Confused, Diagonal, Sad, SadOpen, SadClosed, Ok, X, Interrogation, Thunder, Culito, Angry

### Sounds/Melodies (19)
Connection, Disconnection, Surprise, OhOoh, OhOoh2, Cuddly, Sleeping, Happy, SuperHappy, HappyShort, Sad, Confused, Fart1/2/3, Mode1/2/3, ButtonPushed

### Buzzer
Raw tone: frequency Hz + duration ms

### Sensors
Distance, noise, battery level, name, program ID

### Architecture
- `src/core/` — Qt-free C++ business logic (protocol, commands, state)
- `src/gui/` — Qt/QML adapter layer
- `src/backends/bt_qt/` — shared Bluetooth backend
- `MovementSequencer` — state machine for coordinated multi-cycle movements
- `RobotCommands` — builds serial command strings
- `RobotController` — dispatches commands, manages situation state machine (Demo/Unregistered/Connecting/Connected/Disconnected/TransportLost)

---

## AI Integration Ideas

### 1. Voice Command Interface (Speech-to-Command)

Use a local Whisper model (e.g., `whisper.cpp` or `faster-whisper`) to transcribe spoken commands in real time, then map them to Zowi's command set.

**Examples:**
- *"Zowi, camina"* → `commandWalkForward()`
- *"Zowi, gira"* → `commandTurnLeft()`
- *"Zowi, feliz"* → `commandGesture(Happy)` + `commandMouthById(Smile)` + `commandSing(Happy)`
- *"Zowi, callate"* → `commandStop()`

**Where to hook:** A new `VoiceInput` module in `src/core/` (Qt-free) could feed text into a command mapper. The `RobotController` already dispatches commands.

### 2. Emotion Recognition from Voice Tone

Use a lightweight emotion classifier on audio features (pitch, speed, volume) to detect the user's emotional state and have Zowi respond empathetically.

**Examples:**
- Angry voice → Zowi shows `Angry` mouth + `Angry` gesture
- Happy voice → Zowi shows `SuperHappy` + `Happy` melody
- Sad voice → Zowi shows `Sad` + plays `Sad` melody

**Implementation:** Pure C++ audio analysis module in `src/core/`.

### 3. Conversational AI (LLM Integration)

Connect Zowi to a local LLM (e.g., Ollama, llama.cpp) for natural-language conversation. The desktop app sends the user's text to the LLM, which generates a response that Zowi "performs".

**Examples:**
- User: *"¿Qué haces?"* → LLM → Zowi says a text-to-speech response + plays a matching melody + shows an appropriate mouth

**Architecture:** New `AIEngine` class in `src/core/` handles LLM API calls; the GUI provides a chat screen in QML.

### 4. Computer Vision → Behavior

Use a camera + lightweight vision model (MediaPipe, YOLO via ONNX) to detect:
- **Person detection**: Zowi waves (`commandGesture(Wave)`) when someone approaches
- **Distance tracking**: Already have `GetDistance` sensor — use it for obstacle avoidance AI
- **Gesture recognition**: User makes a hand sign → Zowi mirrors it with a gesture

**Implementation:** The `GetDistance` command already exists in the protocol — obstacle avoidance could be implemented as a new `MovementSequencer` mode.

### 5. Reactive Personality Engine

An AI "personality" layer that makes Zowi behave autonomously between commands.

**Examples:**
- Track interaction history → Zowi "remembers" the user and adjusts its behavior
- If the user is frequently angry → Zowi becomes more careful
- If the user hasn't interacted in a while → Zowi does a `Sleeping` gesture + `Sleeping` melody

**Implementation:** Could use a simple rule-based system or a small local model. Store state in `SessionStore` (already exists).

### 6. Text-to-Speech + Sound Synthesis

Instead of the fixed 19 melodies, use a TTS engine (e.g., `piper-tts` or `edge-tts`) to make Zowi speak actual sentences through the buzzer, combined with dynamic mouth patterns.

**Examples:**
- User speaks → Zowi "repeats" it back in its own robotic voice
- Map phonemes to specific mouth patterns dynamically

### 7. LLM-Powered Gesture/Mouth Synthesis

Instead of the fixed 31 mouth patterns and 13 gestures, use an LLM to generate a "performance script" that sequences multiple commands.

**Example:**
- Prompt: *"Zowi is celebrating a birthday"* → LLM generates sequence: `Happy` gesture → `HappyOpen` mouth → `Happy` melody → `Jump` → `Victory` gesture → `Happy` melody

**Implementation:** The `MovementSequencer` already handles multi-cycle choreography — it just needs an AI choreographer.

### 8. Offline-First Hybrid Architecture

Since Zowi is a physical robot, latency matters. A tiered approach:

- **Tier 1 (fast, local)**: Keyword spotting → instant response (e.g., "Zowi" wake word → `Wave`)
- **Tier 2 (local, moderate)**: Whisper for speech-to-text → command mapping
- **Tier 3 (cloud or local LLM)**: Open-ended conversation → complex behavior

---

## Implementation Starting Points

| Layer | Component | Location |
|---|---|---|
| Core logic | `AICommandMapper` (text → `robot_commands.h` calls) | `src/core/src/` |
| Audio STT | `WhisperEngine` wrapper | `src/core/src/` (Qt-free) |
| LLM client | `LLMClient` (HTTP → Ollama/API) | `src/core/src/` |
| GUI chat screen | New QML screen + controller | `src/gui/` + `src/views/screens/` |
| Wake word | `KeywordDetector` | `src/core/src/` |

### Recommended First Step

A **voice command prototype**: add a `SpeechToCommand` class in `src/core/` that maps transcribed text to existing `robot_commands`, triggered from the GUI or CLI. Since `src/core` is Qt-free, the heavy AI work (Whisper, LLM calls) can be in a separate library that the core calls.

### Key Constraints

- `src/core/` must stay Qt-free — all AI logic that doesn't need Qt belongs here
- GUI is an adapter layer only — no business logic in QML
- Commands follow the `&&cmd args%%` protocol format
- The `MovementSequencer` handles coordinated multi-cycle movements and must be respected for proper sequencing
- Config keys (`activeZowiDeviceAddress`, `activeZowiName`, etc.) must stay in sync between GUI (Qt resource `:/src/config.json`) and CLI (`src/config.json`)
- Bluetooth backend is shared in `src/backends/bt_qt/`

---

## Protocol Quick Reference

```
Command   Char   Format
--------  -----  -------------------
Stop      S      S\r
LED/Mouth L      L <32-bit pattern>\r
Buzzer    T      T <freqHz> <durationMs>\r
Move      M      M <moveId> <periodT>\r
Gesture   H      H <gestureId>\r
Sing      K      K <melodyId>\r
Trims     C      C <yl> <yr> <rl> <rr>\r
Servo     G      G <yl> <yr> <rl> <rr>\r
SetName   R      R <name>\r
GetName   E      E\r
GetDist   D      D\r
GetNoise  N      N\r
GetBattery B     B\r
GetProgId I      I\r
```

Responses use `&&<cmd> <value>%%` framing.

---

## Local Whisper Model — What & How

### What Is a Local Whisper Model?

A **local Whisper model** is a self-hosted version of OpenAI's Whisper speech-to-text engine that runs entirely on your own hardware, without sending audio to any external server. This guarantees:

- **Privacy** — no audio leaves the device
- **Low latency** — no network round-trip
- **Offline capability** — works without internet

Whisper is a general-purpose speech recognition model trained on 680,000 hours of multilingual audio. It supports 99+ languages including Spanish. Available sizes: `tiny` (~75 MB), `base` (~140 MB), `small` (~470 MB), `medium` (~1.5 GB), `large` (~2.9 GB) — larger models are more accurate but slower.

### Implementation Options

#### Option A: `whisper.cpp` (recommended for C++)

The C/C++ port of Whisper, optimized for CPU inference. Ideal for integration with Zowi's Qt-free `src/core/`.

```
zowi-cli/
├── external/
│   └── whisper.cpp/    (submodule or vendor)
├── src/core/
│   └── whisper/
│       ├── WhisperEngine.h/cpp   # Wrapper C++
│       ├── WhisperModel.h/cpp    # Model loader (.gguf)
│       └── AudioCapture.h/cpp    # Microphone capture
```

**Steps:**
1. Compile `whisper.cpp` as a static or linked library
2. Download a model file (e.g., `tiny-es.gguf` for lightweight Spanish, ~75 MB, or `base` ~140 MB)
3. Capture audio from the microphone (PortAudio / PulseAudio / ALSA)
4. Pass audio to the model → retrieve text
5. Map text to a Zowi command (e.g., `"camina"` → `commandWalkForward()`)

#### Option B: `faster-whisper` (Python)

Easier to prototype with Python bindings using CUDA or CPU:

```python
from faster_whisper import WhisperModel
model = WhisperModel("base", device="cpu")
segments, info = model.transcribe("audio.wav")
for segment in segments:
    print(segment.text)  # → map to Zowi command
```

Would be invoked from `zowi_cli` via a child process or IPC.

#### Option C: Separate Whisper Server

Run `whisper.cpp` as a background gRPC/HTTP server, and have ZowiDesktop communicate with it. This decouples model loading from the main application.

### Integration Architecture for Zowi

```
[Microphone] → [Audio Capture] → [Whisper Local] → [Text] → [AICommandMapper] → [RobotCommands] → [Bluetooth/USB] → [Zowi]
```

- `WhisperEngine` lives in `src/core/` (Qt-free, pure C++)
- `AICommandMapper` translates text into existing `robot_commands.h` calls
- The GUI doesn't need to change — add a "voice" mode to the `RobotController`

### Recommended Models for Spanish

| Model  | Size    | Speed        | Accuracy |
|--------|---------|--------------|----------|
| `tiny` | ~75 MB  | ~4x realtime | Low      |
| `base` | ~140 MB | ~2x realtime | Medium   |
| `small`| ~470 MB | ~0.5x realtime| High    |

For an interactive robot, `base` or `small` offer the best trade-off between speed and accuracy.

### Command Mapping Example

| Whisper Output     | Zowi Command                     |
|--------------------|----------------------------------|
| "camina"           | `commandWalkForward(Medium)`     |
| "gira a la izquierda" | `commandTurnLeft(Medium)`    |
| "feliz"            | `commandGesture(Happy)` + `commandMouthById(Smile)` + `commandSing(Happy)` |
| "callate"          | `commandStop()`                  |
| "sube"             | `commandUpDown(Medium, 15)`      |
| "baila"            | `commandJitter(Medium, 15)` + `commandSing(Happy)` |

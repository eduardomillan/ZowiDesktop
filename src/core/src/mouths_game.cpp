#include "zowi/mouths_game.h"

#include <algorithm>
#include <random>

namespace zowi {

namespace {
std::mt19937& rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}
} // namespace

MouthsGame::MouthsGame(const MouthsGameConfig& cfg)
    : m_config(cfg) {}

void MouthsGame::startGame() {
    m_level = 1;
    m_score = 0;
    m_state = MouthsGameState::RoundActive;
    // Two consecutive games never start with the same mouth.
    std::vector<MouthId> forbidden;
    if (m_hasPreviousGame)
        forbidden.push_back(m_previousFirstTarget);
    pickTarget(forbidden);
    m_previousFirstTarget = m_target;
    m_hasPreviousGame = true;
}

void MouthsGame::reset() {
    m_level = 0;
    m_score = 0;
    m_target = MouthId::Smile;
    m_targetPattern = mouthPatternForId(m_target);
    m_state = MouthsGameState::Idle;
}

bool MouthsGame::submitDraw(unsigned long matrix) {
    if (m_state != MouthsGameState::RoundActive || matrix != m_targetPattern)
        return false;
    m_state = MouthsGameState::RoundSolved;
    m_score = m_level;
    return true;
}

void MouthsGame::advanceLevel() {
    if (m_state != MouthsGameState::RoundSolved)
        return;
    ++m_level;
    m_state = MouthsGameState::RoundActive;
    // No two consecutive rounds show the same mouth: forbid the one just solved.
    std::vector<MouthId> forbidden = { m_target };
    pickTarget(forbidden);
}

void MouthsGame::onTimeout() {
    if (m_state != MouthsGameState::RoundActive)
        return;
    m_score = m_level - 1;
    m_state = MouthsGameState::GameOver;
}

int MouthsGame::countdownMsForLevel(int level) const {
    if (level <= 0) return m_config.initialCountdownMs;
    const int steps = level / m_config.countdownStepEveryLevels;
    const int ms = m_config.initialCountdownMs - steps * m_config.countdownStepMs;
    return ms < m_config.minCountdownMs ? m_config.minCountdownMs : ms;
}

int MouthsGame::unlockedBandCount(int level) const {
    const std::vector<std::vector<MouthId>> bands = {
        m_config.band0, m_config.band1, m_config.band2, m_config.band3, m_config.band4
    };
    // Empty bands never unlock (they are placeholder slots in custom configs).
    // When fewer unlock levels are provided than bands, the trailing bands keep
    // the last provided gate — they stay locked until that level is reached.
    const int lastGate = m_config.bandUnlockLevels.empty()
                             ? 1
                             : m_config.bandUnlockLevels.back();
    int count = 0;
    for (int b = 0; b < static_cast<int>(bands.size()); ++b) {
        if (bands[b].empty())
            continue;
        const int unlockLevel = b < static_cast<int>(m_config.bandUnlockLevels.size())
                                    ? m_config.bandUnlockLevels[b]
                                    : lastGate;
        if (level < unlockLevel)
            break; // unlock levels are ascending
        ++count;
    }
    return count > 0 ? count : 1; // never lock every band, even below the first gate
}

void MouthsGame::pickTarget(const std::vector<MouthId>& forbidden) {
    const std::vector<std::vector<MouthId>> bands = {
        m_config.band0, m_config.band1, m_config.band2, m_config.band3, m_config.band4
    };
    // Exclusive pool: the target comes only from the band that unlocks at the
    // current level (find the `unlocked`-th non-empty band). Reaching the next
    // unlock level moves the game onto the harder band, so the difficulty
    // curve never regresses to earlier (easier) mouths.
    const int unlocked = unlockedBandCount(m_level);
    int seen = 0;
    for (int b = 0; b < static_cast<int>(bands.size()); ++b) {
        if (bands[b].empty())
            continue;
        ++seen;
        if (seen == unlocked) {
            const std::vector<MouthId>& band = bands[b];
            // Preferred pool: band mouths minus the forbidden ones. When that
            // leaves nothing (e.g. single-mouth bands in tests), fall back to
            // the full band — a repeat is unavoidable then.
            std::vector<MouthId> allowed;
            for (const MouthId mouth : band) {
                if (std::find(forbidden.begin(), forbidden.end(), mouth) == forbidden.end())
                    allowed.push_back(mouth);
            }
            const std::vector<MouthId>& pool = allowed.empty() ? band : allowed;
            std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
            m_target = pool[dist(rng())];
            m_targetPattern = mouthPatternForId(m_target);
            return;
        }
    }
    m_target = MouthId::Smile;
    m_targetPattern = mouthPatternForId(m_target);
}

} // namespace zowi
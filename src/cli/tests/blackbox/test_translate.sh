#!/usr/bin/env bash
# Black-box test of the `translate` subcommand for every supported locale.
# The CLI loads i18n/zowi_<locale>.json relative to the current directory
# (setResourceBasePath(".")), so this must run from the repository root.
#
# Note: the CLI prints an "[i18n] Loaded ..." notice on stderr; the translated
# result goes to stdout, so we assert on stdout only.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"
[ -d "i18n" ] || fail "i18n/ not found; run from the repository root"

# A source string that does not exist anywhere, so the TranslationEngine must
# fall back to echoing the source unchanged (identity fallback).
SRC="ZedoZowi_MissingKey_2026"
RES=$("$CLI" translate -s "$SRC" 2>/dev/null) || fail "translate failed"
[ "$RES" = "$SRC" ] || fail "translate should echo unknown source unchanged, got: $RES"
echo "ok: unknown source string echoes unchanged (identity fallback)"

# The command must accept every supported locale without error.
for locale in es_ES ca_ES en_US fr_FR bg_BG; do
    "$CLI" translate -l "$locale" -s "$SRC" >/dev/null 2>&1 \
        || fail "translate with locale $locale failed"
done
echo "ok: all 5 supported locales load without error"

# Translate a real string known to exist in the es_ES dictionary to prove the
# lookup path works end to end.
REAL=$("$CLI" translate -l es_ES -c "WelcomeScreen.qml" -s "ZOWI" 2>/dev/null) \
    || fail "translate of a real key failed"
[ "$REAL" = "ZOWI" ] || fail "expected 'ZOWI' for es_ES WelcomeScreen value, got: $REAL"
echo "ok: real dictionary lookup returns expected value"

echo "All translate tests passed."

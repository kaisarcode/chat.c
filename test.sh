#!/bin/sh
# Summary: Validation suite for chat functionality.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: https://www.gnu.org/licenses/gpl-3.0.html

# Prints one failure line.
# @param $1 Failure message.
# @return 1 on failure.
kc_test_fail() {
    printf '\033[31m[FAIL]\033[0m %s\n' "$1"
    return 1
}

# Prints one success line.
# @param $1 Success message.
# @return 0 on success.
kc_test_pass() {
    printf '\033[32m[PASS]\033[0m %s\n' "$1"
}

# Detects the artifact architecture for the current machine.
# @return Architecture name on stdout.
kc_test_arch() {
    case "$(uname -m)" in
        x86_64 | amd64)            printf '%s\n' "x86_64"      ;;
        aarch64 | arm64)           printf '%s\n' "aarch64"     ;;
        armv7l | armv7)            printf '%s\n' "armv7"       ;;
        i386 | i486 | i586 | i686) printf '%s\n' "i686"        ;;
        ppc64le | powerpc64le)     printf '%s\n' "powerpc64le" ;;
        *)                         uname -m                    ;;
    esac
}

# Detects the artifact platform for the current machine.
# @return Platform name on stdout.
kc_test_platform() {
    case "$(uname -s)" in
        Linux) printf '%s\n' "linux" ;;
        *)     uname -s | tr '[:upper:]' '[:lower:]' ;;
    esac
}

# Returns the CLI path for the current architecture and platform.
# @return CLI path on stdout.
kc_test_binary_path() {
    printf './bin/%s/%s/chat\n' "$(kc_test_arch)" "$(kc_test_platform)"
}

# Returns the artifact directory for the current architecture and platform.
# @return Artifact path on stdout.
kc_test_artifact_dir() {
    printf './bin/%s/%s\n' "$(kc_test_arch)" "$(kc_test_platform)"
}

# Verifies the binary exists and is executable.
# @return 0 on success, 1 on failure.
kc_test_check_binary() {
    if [ ! -x "$BIN" ]; then
        kc_test_fail "binary existence: expected executable file at $BIN, but it was not found"
        return 1
    fi
    return 0
}

# Verifies the library artifacts exist for the current build.
# @return 0 on success, 1 on failure.
kc_test_check_libraries() {
    if [ ! -f "$ARTIFACT_DIR/libchat.a" ]; then
        kc_test_fail "static library existence: expected file at $ARTIFACT_DIR/libchat.a, but it was not found"
        return 1
    fi

    if [ "$(kc_test_platform)" = "windows" ]; then
        if [ ! -f "$ARTIFACT_DIR/libchat.dll" ]; then
            kc_test_fail "shared library existence: expected file at $ARTIFACT_DIR/libchat.dll, but it was not found"
            return 1
        fi
    elif [ ! -f "$ARTIFACT_DIR/libchat.so" ]; then
        kc_test_fail "shared library existence: expected file at $ARTIFACT_DIR/libchat.so, but it was not found"
        return 1
    fi

    return 0
}

# Tests fail-fast CLI behavior.
# @return 0 on success, 1 on failure.
kc_test_cli() {
    if "$BIN" --unknown > /dev/null 2>&1; then
        kc_test_fail "cli unknown option: expected exit code non-zero, got 0"
        return 1
    fi
    kc_test_pass "cli unknown option fails"

    if "$BIN" > /dev/null 2>&1; then
        kc_test_fail "cli missing command: expected exit code non-zero, got 0"
        return 1
    fi
    kc_test_pass "cli missing delegate command fails"

    if "$BIN" -x > /dev/null 2>&1; then
        kc_test_fail "cli missing value for -x: expected exit code non-zero, got 0"
        return 1
    fi
    kc_test_pass "cli missing value for -x fails"

    if "$BIN" -e > /dev/null 2>&1; then
        kc_test_fail "cli missing value for -e: expected exit code non-zero, got 0"
        return 1
    fi
    kc_test_pass "cli missing value for -e fails"

    return 0
}

# Tests basic delegation: sends input to a command and checks output.
# @return 0 on success, 1 on failure.
kc_test_delegate() {
    out=$(printf 'hello world\nexit' | "$BIN" 'cat' 2>/dev/null)

    case "$out" in
        *"hello world"*)
            kc_test_pass "delegate: session output contains input hello world"
            ;;
        *)
            kc_test_fail "delegate: expected output to contain 'hello world', got '$out'"
            return 1
            ;;
    esac
    return 0
}

# Tests delegation with an exit command.
# @return 0 on success, 1 on failure.
kc_test_exit_cmd() {
    out=$(printf 'hello\nquit' | "$BIN" -x quit 'cat' 2>/dev/null)

    case "$out" in
        *"hello"*)
            kc_test_pass "exit command: quit command terminates session successfully"
            ;;
        *)
            kc_test_fail "exit: expected output to contain 'hello', got '$out'"
            return 1
            ;;
    esac
    return 0
}

# Tests delegation with an end token.
# @return 0 on success, 1 on failure.
kc_test_end_token() {
    out=$(printf 'line1\nline2\nEND' | "$BIN" -e END 'cat' 2>/dev/null)

    case "$out" in
        *"line1"*"line2"*)
            kc_test_pass "end token: end token END terminates session successfully"
            ;;
        *)
            kc_test_fail "end: expected output to contain 'line1' and 'line2', got '$out'"
            return 1
            ;;
    esac
    return 0
}

# Tests that the prompt is shown.
# @return 0 on success, 1 on failure.
kc_test_prompt() {
    out=$(printf 'hello\nexit' | "$BIN" -p 'IN: ' 'cat' 2>&1)

    case "$out" in
        *"IN: "*)
            kc_test_pass "prompt: custom prompt prefix is rendered"
            ;;
        *)
            kc_test_fail "prompt: expected prompt text in output, got '$out'"
            return 1
            ;;
    esac
    return 0
}

# Tests that the initial message is shown.
# @return 0 on success, 1 on failure.
kc_test_message() {
    out=$(printf 'hello\nexit' | "$BIN" -m 'ready' 'cat' 2>&1)

    case "$out" in
        "ready"*)
            kc_test_pass "message: initial ready message is shown on startup"
            ;;
        *)
            kc_test_fail "message: expected initial message in output, got '$out'"
            return 1
            ;;
    esac
    return 0
}

# Validate public static-library embedding.
# @return 0 on success, 1 on failure.
kc_test_api() {
    LIB_PATH="$ROOT/bin/$(kc_test_arch)/$(kc_test_platform)/libchat.a"
    if [ ! -f "$LIB_PATH" ]; then
        kc_test_fail "static library existence: expected file at $LIB_PATH, but it was not found"
        return 1
    fi

    {
        printf '%s\n' '#include "chat.h"'
        printf '%s\n' '#include <stdio.h>'
        printf '%s\n' '#include <string.h>'
        printf '%s\n' 'int main(void) {'
        printf '%s\n' '    kc_chat_t *ctx = NULL;'
        printf '%s\n' '    kc_chat_options_t opts = kc_chat_options_default();'
        printf '%s\n' '    opts.cmd = "cat";'
        printf '%s\n' '    opts.exit_cmd = "quit";'
        printf '%s\n' '    if (kc_chat_open(&ctx, &opts) != KC_CHAT_OK) return 2;'
        printf '%s\n' '    if (!ctx) return 3;'
        printf '%s\n' '    if (kc_chat_is_exit(ctx, "quit") != 1) return 4;'
        printf '%s\n' '    if (kc_chat_is_exit(ctx, "hello") != 0) return 5;'
        printf '%s\n' '    char *out = NULL;'
        printf '%s\n' '    if (kc_chat_exec(ctx, "hello C API", &out) != KC_CHAT_OK) return 6;'
        printf '%s\n' '    if (!out) return 7;'
        printf '%s\n' '    if (strstr(out, "hello C API") == NULL) return 8;'
        printf '%s\n' '    kc_chat_free(out);'
        printf '%s\n' '    kc_chat_close(ctx);'
        printf '%s\n' '    return 0;'
        printf '%s\n' '}'
    } > "$TMPDIR/consumer.c"

    if ! cc -I "$ROOT/src" "$TMPDIR/consumer.c" "$LIB_PATH" -o "$TMPDIR/consumer"; then
        kc_test_fail "static library compile: expected consumer.c to compile, but compilation failed"
        return 1
    fi
    kc_test_pass "static library consumer program compiles successfully"

    if ! "$TMPDIR/consumer"; then
        kc_test_fail "static library execution: expected consumer program to return 0, got non-zero"
        return 1
    fi
    kc_test_pass "static library public C API behaviors verified successfully"
    return 0
}

# Runs the full validation suite.
# @return 0 on success, 1 on failure.
kc_test_main() {
    failed=0
    ROOT=$(CDPATH='' cd -- "$(dirname "$0")" && pwd)
    BIN="$ROOT/$(kc_test_binary_path)"
    ARTIFACT_DIR="$ROOT/$(kc_test_artifact_dir)"
    TMPDIR=$(mktemp -d)
    trap 'rm -rf "$TMPDIR"' EXIT INT HUP TERM

    kc_test_check_binary || exit 1
    kc_test_check_libraries || exit 1

    kc_test_cli || failed=$((failed + 1))
    kc_test_delegate || failed=$((failed + 1))
    kc_test_exit_cmd || failed=$((failed + 1))
    kc_test_end_token || failed=$((failed + 1))
    kc_test_prompt || failed=$((failed + 1))
    kc_test_message || failed=$((failed + 1))
    kc_test_api || failed=$((failed + 1))

    return "$failed"
}

kc_test_main

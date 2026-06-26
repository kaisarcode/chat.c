/**
 * test.c - libchat portable contract tests.
 * Summary: Validates exported libchat behavior through the public C API.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "chat.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int signal_count = 0;

/**
 * Stores one observed signal callback.
 * @param ctx Context passed by the library.
 * @return None.
 */
static void count_signal(kc_chat_t *ctx) {
    if (ctx != NULL) {
        signal_count++;
    }
}

/**
 * Sets or clears a process environment variable.
 * @param name Variable name.
 * @param value Variable value, or NULL to clear.
 * @return 0 on success, 1 on failure.
 */
static int set_env_value(const char *name, const char *value) {
#ifdef _WIN32
    return _putenv_s(name, value != NULL ? value : "") == 0 ? 0 : 1;
#else
    if (value == NULL) return unsetenv(name) == 0 ? 0 : 1;
    return setenv(name, value, 1) == 0 ? 0 : 1;
#endif
}

/**
 * Duplicates a string into caller-owned memory.
 * @param text Input string.
 * @return Allocated copy, or NULL on failure.
 */
static char *copy_string(const char *text) {
    char *copy;
    size_t length;

    length = strlen(text) + 1;
    copy = (char *)malloc(length);
    if (copy == NULL) return NULL;
    memcpy(copy, text, length);
    return copy;
}

/**
 * Verifies one integer result.
 * @param name Check name.
 * @param expected Expected value.
 * @param actual Actual value.
 * @return 0 on success, 1 on failure.
 */
static int expect_int(const char *name, int expected, int actual) {
    if (expected != actual) {
        fprintf(stderr, "%s: expected %d, got %d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

/**
 * Verifies one string result.
 * @param name Check name.
 * @param expected Expected string.
 * @param actual Actual string.
 * @return 0 on success, 1 on failure.
 */
static int expect_string(const char *name, const char *expected, const char *actual) {
    if (actual == NULL || strcmp(expected, actual) != 0) {
        fprintf(stderr, "%s: expected '%s', got '%s'\n", name, expected,
            actual != NULL ? actual : "NULL");
        return 1;
    }
    return 0;
}

/**
 * Verifies that a string contains a substring.
 * @param name Check name.
 * @param haystack Output string.
 * @param needle Expected substring.
 * @return 0 on success, 1 on failure.
 */
static int expect_contains(const char *name, const char *haystack, const char *needle) {
    if (haystack == NULL || strstr(haystack, needle) == NULL) {
        fprintf(stderr, "%s: expected output to contain '%s', got '%s'\n", name,
            needle, haystack != NULL ? haystack : "NULL");
        return 1;
    }
    return 0;
}

/**
 * Verifies the build version API.
 * @return 0 on success, 1 on failure.
 */
static int case_version(void) {
    if (kc_chat_version() == 0U) {
        fprintf(stderr, "version: expected non-zero build timestamp, got 0\n");
        return 1;
    }
    return 0;
}

/**
 * Verifies options defaults, environment loading, and cleanup.
 * @return 0 on success, 1 on failure.
 */
static int case_options(void) {
    kc_chat_options_t opts;
    int rc;

    opts = kc_chat_options_default();
    rc = 0;
    if (opts.cmd != NULL || opts.end_token != NULL || opts.exit_cmd != NULL ||
        opts.msg != NULL || opts.prompt != NULL) {
        fprintf(stderr, "default options: expected all NULL string fields\n");
        return 1;
    }
    if (set_env_value("KC_CHAT_CMD", "env-cmd") != 0) {
        fprintf(stderr, "set KC_CHAT_CMD: expected success, got failure\n");
        return 1;
    }
    if (set_env_value("KC_CHAT_EXIT_CMD", "env-exit") != 0) {
        fprintf(stderr, "set KC_CHAT_EXIT_CMD: expected success, got failure\n");
        return 1;
    }
    if (set_env_value("KC_CHAT_PROMPT", "env-prompt") != 0) {
        fprintf(stderr, "set KC_CHAT_PROMPT: expected success, got failure\n");
        return 1;
    }
    kc_chat_options_load_env(&opts);
    rc += expect_string("env cmd", "env-cmd", opts.cmd);
    rc += expect_string("env exit_cmd", "env-exit", opts.exit_cmd);
    rc += expect_string("env prompt", "env-prompt", opts.prompt);
    set_env_value("KC_CHAT_CMD", NULL);
    set_env_value("KC_CHAT_EXIT_CMD", NULL);
    set_env_value("KC_CHAT_PROMPT", NULL);
    kc_chat_options_free(&opts);
    if (opts.cmd != NULL || opts.exit_cmd != NULL || opts.prompt != NULL) {
        fprintf(stderr, "options free: expected NULL fields, got non-NULL\n");
        return 1;
    }
    kc_chat_options_load_env(NULL);
    kc_chat_options_free(NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies context lifecycle and NULL guard contracts.
 * @return 0 on success, 1 on failure.
 */
static int case_lifecycle(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("open(NULL, opts)", KC_CHAT_ERROR, kc_chat_open(NULL, &opts));
    rc += expect_int("open(out, NULL)", KC_CHAT_ERROR, kc_chat_open(&ctx, NULL));
    rc += expect_int("open(out, opts)", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("is_exit(NULL)", 0, kc_chat_is_exit(NULL, "exit"));
    kc_chat_close(NULL);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies exit-command matching contracts.
 * @return 0 on success, 1 on failure.
 */
static int case_is_exit(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    opts.exit_cmd = copy_string("quit");
    if (opts.cmd == NULL || opts.exit_cmd == NULL) {
        fprintf(stderr, "copy options: expected allocation, got NULL\n");
        kc_chat_options_free(&opts);
        return 1;
    }
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("is_exit match", 1, kc_chat_is_exit(ctx, "quit"));
    rc += expect_int("is_exit mismatch", 0, kc_chat_is_exit(ctx, "hello"));
    rc += expect_int("is_exit NULL input", 0, kc_chat_is_exit(ctx, NULL));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = kc_chat_options_default();
    opts.cmd = copy_string("cat");
    rc += expect_int("open no exit_cmd", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("is_exit no exit_cmd set", 0, kc_chat_is_exit(ctx, "exit"));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies exec delegation through the configured command.
 * POSIX only: kc_chat_exec uses fork/pipe/execl on /bin/sh.
 * @return 0 on success, 1 on failure. Skipped on Windows.
 */
static int case_exec(void) {
#ifdef _WIN32
    return 0;
#else
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    char *out;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    out = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("exec(NULL)", KC_CHAT_ERROR,
        kc_chat_exec(NULL, "hello", &out));
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("exec(ctx, NULL)", KC_CHAT_ERROR,
        kc_chat_exec(ctx, NULL, &out));
    rc += expect_int("exec(ctx, input, NULL)", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", NULL));
    rc += expect_int("exec(ctx, input)", KC_CHAT_OK,
        kc_chat_exec(ctx, "hello C API", &out));
    rc += expect_contains("exec output", out, "hello C API");
    kc_chat_free(out);
    out = NULL;
    rc += expect_int("exec(ctx, second)", KC_CHAT_OK,
        kc_chat_exec(ctx, "second turn", &out));
    rc += expect_contains("exec second output", out, "second turn");
    kc_chat_free(out);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Verifies signal callback registration and context routing contracts.
 * @return 0 on success, 1 on failure.
 */
static int case_signals(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    signal_count = 0;
    rc += expect_int("on_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_on_signal(NULL, SIGINT, count_signal));
    rc += expect_int("raise_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_raise_signal(NULL, SIGINT));
    rc += expect_int("listen_signals(NULL)", KC_CHAT_ERROR,
        kc_chat_listen_signals(NULL));
    rc += expect_int("listen_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_listen_signal(NULL, SIGINT));
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("raise unhandled signal", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, SIGINT));
    rc += expect_int("register signal handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, SIGINT, count_signal));
    rc += expect_int("raise handled signal", KC_CHAT_OK,
        kc_chat_raise_signal(ctx, SIGINT));
    if (signal_count != 1) {
        fprintf(stderr, "signal callback count: expected 1, got %d\n", signal_count);
        rc++;
    }
    rc += expect_int("remove signal handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, SIGINT, NULL));
    rc += expect_int("raise removed signal", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, SIGINT));
    rc += expect_int("listen signals", KC_CHAT_OK, kc_chat_listen_signals(ctx));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies context-owned stop state isolation across two contexts.
 * POSIX only: uses kc_chat_exec to confirm the second context still runs.
 * @return 0 on success, 1 on failure. Skipped on Windows.
 */
static int case_multictx_stop(void) {
#ifdef _WIN32
    return 0;
#else
    kc_chat_options_t opts;
    kc_chat_t *first;
    kc_chat_t *second;
    char *out;
    int rc;

    opts = kc_chat_options_default();
    first = NULL;
    second = NULL;
    out = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("open first", KC_CHAT_OK, kc_chat_open(&first, &opts));
    rc += expect_int("open second", KC_CHAT_OK, kc_chat_open(&second, &opts));
    rc += expect_int("stop first", KC_CHAT_OK, kc_chat_stop(first));
    rc += expect_int("exec second after first stopped", KC_CHAT_OK,
        kc_chat_exec(second, "hello second", &out));
    rc += expect_contains("second output after stop", out, "hello second");
    kc_chat_free(out);
    out = NULL;
    rc += expect_int("stop second", KC_CHAT_OK, kc_chat_stop(second));
    rc += expect_int("stop first again", KC_CHAT_OK, kc_chat_stop(first));
    rc += expect_int("stop(NULL)", KC_CHAT_ERROR, kc_chat_stop(NULL));
    kc_chat_close(first);
    kc_chat_close(second);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Runs one libchat contract test case.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 or 2 on failure.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "test case: expected one argument, got %d\n", argc - 1);
        return 2;
    }
    if (strcmp(argv[1], "version") == 0) return case_version();
    if (strcmp(argv[1], "options") == 0) return case_options();
    if (strcmp(argv[1], "lifecycle") == 0) return case_lifecycle();
    if (strcmp(argv[1], "is-exit") == 0) return case_is_exit();
    if (strcmp(argv[1], "exec") == 0) return case_exec();
    if (strcmp(argv[1], "signals") == 0) return case_signals();
    if (strcmp(argv[1], "multictx-stop") == 0) return case_multictx_stop();
    fprintf(stderr, "unknown test case: %s\n", argv[1]);
    return 2;
}

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
static int signal_count_b = 0;
static kc_chat_t *signal_ctx_seen = NULL;

static int case_lifecycle(void);
static int case_is_exit(void);

/**
 * Stores one observed signal callback.
 * @param ctx Context passed by the library.
 * @return None.
 */
static void count_signal(kc_chat_t *ctx) {
    if (ctx != NULL) {
        signal_count++;
        signal_ctx_seen = ctx;
    }
}

/**
 * Stores one observed secondary signal callback.
 * @param ctx Context passed by the library.
 * @return None.
 */
static void count_signal_b(kc_chat_t *ctx) {
    if (ctx != NULL) {
        signal_count_b++;
        signal_ctx_seen = ctx;
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
 * Verifies one boolean result.
 * @param name Check name.
 * @param condition Condition expected to be true.
 * @return 0 on success, 1 on failure.
 */
static int expect_true(const char *name, int condition) {
    if (!condition) {
        fprintf(stderr, "%s: expected true, got false\n", name);
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

#ifndef _WIN32
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
#endif

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
    rc += expect_int("set KC_CHAT_CMD", 0, set_env_value("KC_CHAT_CMD", "env-cmd"));
    rc += expect_int("set KC_CHAT_END_TOKEN", 0,
        set_env_value("KC_CHAT_END_TOKEN", "env-end"));
    rc += expect_int("set KC_CHAT_EXIT_CMD", 0,
        set_env_value("KC_CHAT_EXIT_CMD", "env-exit"));
    rc += expect_int("set KC_CHAT_MSG", 0, set_env_value("KC_CHAT_MSG", "env-msg"));
    rc += expect_int("set KC_CHAT_PROMPT", 0,
        set_env_value("KC_CHAT_PROMPT", "env-prompt"));
    kc_chat_options_load_env(&opts);
    rc += expect_string("env cmd", "env-cmd", opts.cmd);
    rc += expect_string("env end_token", "env-end", opts.end_token);
    rc += expect_string("env exit_cmd", "env-exit", opts.exit_cmd);
    rc += expect_string("env msg", "env-msg", opts.msg);
    rc += expect_string("env prompt", "env-prompt", opts.prompt);
    rc += expect_int("overwrite KC_CHAT_CMD", 0,
        set_env_value("KC_CHAT_CMD", "env-cmd-2"));
    kc_chat_options_load_env(&opts);
    rc += expect_string("env cmd overwrite", "env-cmd-2", opts.cmd);
    set_env_value("KC_CHAT_CMD", NULL);
    set_env_value("KC_CHAT_END_TOKEN", NULL);
    set_env_value("KC_CHAT_EXIT_CMD", NULL);
    set_env_value("KC_CHAT_MSG", NULL);
    set_env_value("KC_CHAT_PROMPT", NULL);
    kc_chat_options_free(&opts);
    if (opts.cmd != NULL || opts.end_token != NULL || opts.exit_cmd != NULL ||
        opts.msg != NULL || opts.prompt != NULL) {
        fprintf(stderr, "options free: expected NULL fields, got non-NULL\n");
        return 1;
    }
    kc_chat_options_free(&opts);
    kc_chat_options_load_env(NULL);
    kc_chat_options_free(NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies kc_chat_open deep-copies option strings owned by the caller.
 * @return 0 on success, 1 on failure.
 */
static int case_open_deep_copy(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
#ifndef _WIN32
    char *out;
#endif
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
#ifndef _WIN32
    out = NULL;
#endif
    rc = 0;
    opts.cmd = copy_string("cat");
    opts.exit_cmd = copy_string("quit");
    opts.prompt = copy_string("prompt-a");
    opts.msg = copy_string("msg-a");
    opts.end_token = copy_string("END");
    if (opts.cmd == NULL || opts.exit_cmd == NULL || opts.prompt == NULL ||
        opts.msg == NULL || opts.end_token == NULL) {
        kc_chat_options_free(&opts);
        return 1;
    }
    rc += expect_int("open deep-copy ctx", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    kc_chat_options_free(&opts);
    rc += expect_int("deep-copy exit command survives caller free", 1,
        kc_chat_is_exit(ctx, "quit"));
#ifndef _WIN32
    rc += expect_int("deep-copy command survives caller free", KC_CHAT_OK,
        kc_chat_exec(ctx, "deep copy", &out));
    rc += expect_contains("deep-copy exec output", out, "deep copy");
    kc_chat_free(out);
#endif
    kc_chat_close(ctx);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies options and version behavior as one configuration contract.
 * @return 0 on success, 1 on failure.
 */
static int case_options_contract(void) {
    int rc;

    rc = 0;
    rc += case_version();
    rc += case_options();
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies context lifecycle and option ownership behavior.
 * @return 0 on success, 1 on failure.
 */
static int case_context_contract(void) {
    int rc;

    rc = 0;
    rc += case_lifecycle();
    rc += case_open_deep_copy();
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
static int case_exec_guards(void) {
#ifdef _WIN32
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    char *out;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    out = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("windows exec returns ERROR", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", &out));
    rc += expect_true("windows exec leaves output NULL", out == NULL);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
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
    rc += expect_true("exec(ctx, NULL) leaves output NULL", out == NULL);
    rc += expect_int("exec(ctx, input, NULL)", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", NULL));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = kc_chat_options_default();
    ctx = NULL;
    rc += expect_int("open without command", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("exec without command", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", &out));
    rc += expect_true("exec without command leaves output NULL", out == NULL);
    kc_chat_close(ctx);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Verifies successful exec delegation, repeated calls, and empty output.
 * POSIX only: kc_chat_exec uses fork/pipe/execl on /bin/sh.
 * @return 0 on success, 1 on failure. Windows validates unsupported exec.
 */
static int case_exec_roundtrip(void) {
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
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("exec(ctx, input)", KC_CHAT_OK,
        kc_chat_exec(ctx, "hello C API", &out));
    rc += expect_contains("exec output", out, "hello C API");
    kc_chat_free(out);
    out = NULL;
    rc += expect_int("exec(ctx, second)", KC_CHAT_OK,
        kc_chat_exec(ctx, "second turn", &out));
    rc += expect_contains("exec second output", out, "second turn");
    kc_chat_free(out);
    out = NULL;
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = kc_chat_options_default();
    opts.cmd = copy_string("true");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open empty-output command", KC_CHAT_OK,
        kc_chat_open(&ctx, &opts));
    rc += expect_int("exec empty-output command", KC_CHAT_OK,
        kc_chat_exec(ctx, "ignored", &out));
    rc += expect_string("empty-output command returns empty string", "", out);
    kc_chat_free(out);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Verifies large exec output exercises dynamic read-buffer growth.
 * @return 0 on success, 1 on failure. Skipped on Windows.
 */
static int case_exec_large(void) {
#ifdef _WIN32
    return 0;
#else
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    char input[8193];
    char *out;
    int rc;
    size_t i;

    opts = kc_chat_options_default();
    ctx = NULL;
    out = NULL;
    rc = 0;
    for (i = 0; i < sizeof(input) - 1; i++) input[i] = (char)('a' + (i % 26));
    input[sizeof(input) - 1] = '\0';
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open large exec", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("large exec succeeds", KC_CHAT_OK,
        kc_chat_exec(ctx, input, &out));
    rc += expect_string("large exec output is byte-exact", input, out);
    kc_chat_free(out);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Verifies command failure propagates as KC_CHAT_ERROR.
 * @return 0 on success, 1 on failure. Skipped on Windows.
 */
static int case_exec_failure(void) {
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
    opts.cmd = copy_string("sh -c 'printf failure-output; exit 7'");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open failing command", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("failing command returns ERROR", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "ignored", &out));
    rc += expect_contains("failing command still captures stdout", out,
        "failure-output");
    kc_chat_free(out);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
#endif
}

/**
 * Verifies exec behavior as one functional contract.
 * @return 0 on success, 1 on failure.
 */
static int case_exec_contract(void) {
    int rc;

    rc = 0;
    rc += case_exec_guards();
#ifndef _WIN32
    rc += case_exec_roundtrip();
    rc += case_exec_large();
    rc += case_exec_failure();
#endif
    return rc == 0 ? 0 : 1;
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
    signal_count_b = 0;
    signal_ctx_seen = NULL;
    rc += expect_int("on_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_on_signal(NULL, 10, count_signal));
    rc += expect_int("raise_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_raise_signal(NULL, 10));
    rc += expect_int("listen_signals(NULL)", KC_CHAT_ERROR,
        kc_chat_listen_signals(NULL));
    rc += expect_int("listen_signal(NULL)", KC_CHAT_ERROR,
        kc_chat_listen_signal(NULL, 10));
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("raise unhandled signal", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, 10));
    rc += expect_int("register signal handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 10, count_signal));
    rc += expect_int("raise handled signal", KC_CHAT_OK,
        kc_chat_raise_signal(ctx, 10));
    rc += expect_int("signal callback count", 1, signal_count);
    rc += expect_true("signal callback saw correct ctx", signal_ctx_seen == ctx);
    rc += expect_int("different unhandled signal returns ERROR", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, 11));
    rc += expect_int("remove signal handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 10, NULL));
    rc += expect_int("raise removed signal", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, 10));
    rc += expect_int("remove absent signal is OK", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 10, NULL));
    rc += expect_int("listen signals", KC_CHAT_OK, kc_chat_listen_signals(ctx));
    rc += expect_int("listen one signal", KC_CHAT_OK, kc_chat_listen_signal(ctx, 12));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies signal handler replacement keeps one handler for the signal.
 * @return 0 on success, 1 on failure.
 */
static int case_signal_replace(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    signal_count = 0;
    signal_count_b = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("register first handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 20, count_signal));
    rc += expect_int("replace handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 20, count_signal_b));
    rc += expect_int("raise replaced signal", KC_CHAT_OK,
        kc_chat_raise_signal(ctx, 20));
    rc += expect_int("old handler not called after replacement", 0, signal_count);
    rc += expect_int("new handler called after replacement", 1, signal_count_b);
    rc += expect_int("remove replaced handler", KC_CHAT_OK,
        kc_chat_on_signal(ctx, 20, NULL));
    rc += expect_int("raise removed replaced handler", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, 20));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies signal handler storage grows past the initial capacity.
 * @return 0 on success, 1 on failure.
 */
static int case_signal_growth(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;
    int i;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    signal_count = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    for (i = 0; i < 10; i++) {
        rc += expect_int("register growth signal", KC_CHAT_OK,
            kc_chat_on_signal(ctx, 100 + i, count_signal));
    }
    rc += expect_int("raise last growth signal", KC_CHAT_OK,
        kc_chat_raise_signal(ctx, 109));
    rc += expect_int("growth handler called once", 1, signal_count);
    for (i = 0; i < 10; i++) {
        rc += expect_int("remove growth signal", KC_CHAT_OK,
            kc_chat_on_signal(ctx, 100 + i, NULL));
    }
    rc += expect_int("raise removed growth signal", KC_CHAT_ERROR,
        kc_chat_raise_signal(ctx, 109));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies global signal listener dispatches to registered contexts.
 * @return 0 on success, 1 on failure.
 */
static int case_signal_listener_dispatch(void) {
    kc_chat_options_t opts;
    kc_chat_t *first;
    kc_chat_t *second;
    int rc;

    opts = kc_chat_options_default();
    first = NULL;
    second = NULL;
    rc = 0;
    signal_count = 0;
    signal_count_b = 0;
    signal_ctx_seen = NULL;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open first", KC_CHAT_OK, kc_chat_open(&first, &opts));
    rc += expect_int("open second", KC_CHAT_OK, kc_chat_open(&second, &opts));
    rc += expect_int("first listens globally", KC_CHAT_OK,
        kc_chat_listen_signals(first));
    rc += expect_int("second listens globally", KC_CHAT_OK,
        kc_chat_listen_signals(second));
    rc += expect_int("second has handler", KC_CHAT_OK,
        kc_chat_on_signal(second, 77, count_signal_b));
    kc_chat_signal_listener(77);
    rc += expect_int("listener calls second handler", 1, signal_count_b);
    rc += expect_true("listener routed correct ctx", signal_ctx_seen == second);
    kc_chat_close(first);
    kc_chat_close(second);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies signal behavior as one functional contract.
 * @return 0 on success, 1 on failure.
 */
static int case_signal_contract(void) {
    int rc;

    rc = 0;
    rc += case_signals();
    rc += case_signal_replace();
    rc += case_signal_growth();
    rc += case_signal_listener_dispatch();
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies context-owned stop state isolation across two contexts.
 * @return 0 on success, 1 on failure.
 */
static int case_multictx_stop(void) {
    kc_chat_options_t opts;
    kc_chat_t *first;
    kc_chat_t *second;
#ifndef _WIN32
    char *out;
#endif
    int rc;

    opts = kc_chat_options_default();
    first = NULL;
    second = NULL;
#ifndef _WIN32
    out = NULL;
#endif
    rc = 0;
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) {
        fprintf(stderr, "copy cmd: expected allocation, got NULL\n");
        return 1;
    }
    rc += expect_int("open first", KC_CHAT_OK, kc_chat_open(&first, &opts));
    rc += expect_int("open second", KC_CHAT_OK, kc_chat_open(&second, &opts));
    rc += expect_int("stop first", KC_CHAT_OK, kc_chat_stop(first));
#ifndef _WIN32
    rc += expect_int("exec second after first stopped", KC_CHAT_OK,
        kc_chat_exec(second, "hello second", &out));
    rc += expect_contains("second output after stop", out, "hello second");
    kc_chat_free(out);
    out = NULL;
#else
    rc += expect_int("exec second after first stopped returns Windows error",
        KC_CHAT_ERROR, kc_chat_exec(second, "hello second", NULL));
#endif
    rc += expect_int("stop second", KC_CHAT_OK, kc_chat_stop(second));
    rc += expect_int("stop first again", KC_CHAT_OK, kc_chat_stop(first));
    rc += expect_int("stop(NULL)", KC_CHAT_ERROR, kc_chat_stop(NULL));
    kc_chat_close(first);
    kc_chat_close(second);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
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
    if (strcmp(argv[1], "options") == 0) return case_options_contract();
    if (strcmp(argv[1], "context") == 0) return case_context_contract();
    if (strcmp(argv[1], "exit") == 0) return case_is_exit();
    if (strcmp(argv[1], "exec") == 0) return case_exec_contract();
    if (strcmp(argv[1], "signal") == 0) return case_signal_contract();
    if (strcmp(argv[1], "version") == 0) return case_version();
    if (strcmp(argv[1], "options") == 0) return case_options();
    if (strcmp(argv[1], "open-deep-copy") == 0) return case_open_deep_copy();
    if (strcmp(argv[1], "lifecycle") == 0) return case_lifecycle();
    if (strcmp(argv[1], "is-exit") == 0) return case_is_exit();
    if (strcmp(argv[1], "exec-guards") == 0) return case_exec_guards();
    if (strcmp(argv[1], "exec-roundtrip") == 0) return case_exec_roundtrip();
    if (strcmp(argv[1], "exec-large") == 0) return case_exec_large();
    if (strcmp(argv[1], "exec-failure") == 0) return case_exec_failure();
    if (strcmp(argv[1], "signals") == 0) return case_signals();
    if (strcmp(argv[1], "signal-replace") == 0) return case_signal_replace();
    if (strcmp(argv[1], "signal-growth") == 0) return case_signal_growth();
    if (strcmp(argv[1], "signal-listener-dispatch") == 0) {
        return case_signal_listener_dispatch();
    }
    if (strcmp(argv[1], "multictx-stop") == 0) return case_multictx_stop();
    fprintf(stderr, "unknown test case: %s\n", argv[1]);
    return 2;
}

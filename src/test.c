/**
 * test.c - libchat public API contract tests.
 * Summary: Validates each exported chat function through one dedicated test case.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "libchat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * Verifies one boolean condition.
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
 * Creates a basic options struct with a command.
 * @return Prepared options struct.
 */
static kc_chat_options_t make_cat_options(void) {
    kc_chat_options_t opts;

    opts = kc_chat_options_default();
    opts.cmd = copy_string("cat");
    return opts;
}

/**
 * Tests kc_chat_open.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_open(void) {
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
    rc += expect_int("open NULL out", KC_CHAT_ERROR, kc_chat_open(NULL, &opts));
    rc += expect_int("open NULL opts", KC_CHAT_ERROR, kc_chat_open(&ctx, NULL));
    rc += expect_int("open valid context", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    kc_chat_options_free(&opts);
    rc += expect_int("deep-copied exit command survives caller free", 1,
        kc_chat_is_exit(ctx, "quit"));
#ifndef _WIN32
    rc += expect_int("deep-copied command survives caller free", KC_CHAT_OK,
        kc_chat_exec(ctx, "deep copy", &out));
    rc += expect_contains("deep copy output", out, "deep copy");
    kc_chat_free(out);
#endif
    kc_chat_close(ctx);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_close.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_close(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = make_cat_options();
    ctx = NULL;
    rc = 0;
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open before close", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    kc_chat_close(NULL);
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_exec.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_exec(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    char *out;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    out = NULL;
    rc = 0;
    rc += expect_int("exec NULL ctx", KC_CHAT_ERROR,
        kc_chat_exec(NULL, "hello", &out));
    opts.cmd = copy_string("cat");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open for exec", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("exec NULL input", KC_CHAT_ERROR,
        kc_chat_exec(ctx, NULL, &out));
    rc += expect_int("exec NULL output", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", NULL));
#ifdef _WIN32
    rc += expect_int("windows exec unsupported", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", &out));
    rc += expect_true("windows exec keeps output NULL", out == NULL);
#else
    rc += expect_int("exec roundtrip", KC_CHAT_OK,
        kc_chat_exec(ctx, "hello C API", &out));
    rc += expect_contains("exec output", out, "hello C API");
    kc_chat_free(out);
    out = NULL;
    rc += expect_int("exec second roundtrip", KC_CHAT_OK,
        kc_chat_exec(ctx, "second turn", &out));
    rc += expect_contains("exec second output", out, "second turn");
    kc_chat_free(out);
    out = NULL;
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = kc_chat_options_default();
    rc += expect_int("open without command", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("exec without command", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "hello", &out));
    rc += expect_true("exec without command keeps output NULL", out == NULL);
    kc_chat_close(ctx);

    opts = make_cat_options();
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open for large exec", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    {
        char input[8193];
        size_t i;

        for (i = 0; i < sizeof(input) - 1; i++) input[i] = (char)('a' + (i % 26));
        input[sizeof(input) - 1] = '\0';
        rc += expect_int("large exec succeeds", KC_CHAT_OK,
            kc_chat_exec(ctx, input, &out));
        rc += expect_string("large exec is byte exact", input, out);
        kc_chat_free(out);
        out = NULL;
    }
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = kc_chat_options_default();
    opts.cmd = copy_string("sh -c 'printf failure-output; exit 7'");
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open failing command", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("failing command returns error", KC_CHAT_ERROR,
        kc_chat_exec(ctx, "ignored", &out));
    rc += expect_contains("failing command captures stdout", out, "failure-output");
    kc_chat_free(out);
#endif
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_is_exit.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_is_exit(void) {
    kc_chat_options_t opts;
    kc_chat_t *ctx;
    int rc;

    opts = kc_chat_options_default();
    ctx = NULL;
    rc = 0;
    opts.cmd = copy_string("cat");
    opts.exit_cmd = copy_string("quit");
    if (opts.cmd == NULL || opts.exit_cmd == NULL) {
        kc_chat_options_free(&opts);
        return 1;
    }
    rc += expect_int("open for exit", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("is_exit NULL ctx", 0, kc_chat_is_exit(NULL, "quit"));
    rc += expect_int("is_exit NULL input", 0, kc_chat_is_exit(ctx, NULL));
    rc += expect_int("is_exit match", 1, kc_chat_is_exit(ctx, "quit"));
    rc += expect_int("is_exit mismatch", 0, kc_chat_is_exit(ctx, "hello"));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);

    opts = make_cat_options();
    if (opts.cmd == NULL) return 1;
    rc += expect_int("open without exit command", KC_CHAT_OK, kc_chat_open(&ctx, &opts));
    rc += expect_int("is_exit without exit command", 0, kc_chat_is_exit(ctx, "quit"));
    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_free.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_free(void) {
    char *text;

    text = copy_string("owned string");
    if (text == NULL) return 1;
    kc_chat_free(text);
    kc_chat_free(NULL);
    return 0;
}

/**
 * Tests kc_chat_options_default.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_options_default(void) {
    kc_chat_options_t opts;

    opts = kc_chat_options_default();
    if (opts.cmd != NULL || opts.end_token != NULL || opts.exit_cmd != NULL ||
        opts.msg != NULL || opts.prompt != NULL) {
        fprintf(stderr, "default options: expected all NULL string fields\n");
        return 1;
    }
    return 0;
}

/**
 * Tests kc_chat_options_load_env.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_options_load_env(void) {
    kc_chat_options_t opts;
    int rc;

    opts = kc_chat_options_default();
    rc = 0;
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
    rc += expect_string("env end token", "env-end", opts.end_token);
    rc += expect_string("env exit cmd", "env-exit", opts.exit_cmd);
    rc += expect_string("env msg", "env-msg", opts.msg);
    rc += expect_string("env prompt", "env-prompt", opts.prompt);
    rc += expect_int("overwrite KC_CHAT_CMD", 0,
        set_env_value("KC_CHAT_CMD", "env-cmd-2"));
    kc_chat_options_load_env(&opts);
    rc += expect_string("env cmd overwrite", "env-cmd-2", opts.cmd);
    kc_chat_options_load_env(NULL);
    kc_chat_options_free(&opts);
    set_env_value("KC_CHAT_CMD", NULL);
    set_env_value("KC_CHAT_END_TOKEN", NULL);
    set_env_value("KC_CHAT_EXIT_CMD", NULL);
    set_env_value("KC_CHAT_MSG", NULL);
    set_env_value("KC_CHAT_PROMPT", NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_options_free.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_options_free(void) {
    kc_chat_options_t opts;
    int rc;

    opts = kc_chat_options_default();
    rc = 0;
    rc += expect_int("set KC_CHAT_CMD", 0, set_env_value("KC_CHAT_CMD", "env-cmd"));
    rc += expect_int("set KC_CHAT_END_TOKEN", 0,
        set_env_value("KC_CHAT_END_TOKEN", "env-end"));
    kc_chat_options_load_env(&opts);
    rc += expect_true("options allocated", opts.cmd != NULL && opts.end_token != NULL);
    kc_chat_options_free(&opts);
    rc += expect_true("options freed", opts.cmd == NULL && opts.end_token == NULL &&
        opts.exit_cmd == NULL && opts.msg == NULL && opts.prompt == NULL);
    kc_chat_options_free(&opts);
    kc_chat_options_free(NULL);
    set_env_value("KC_CHAT_CMD", NULL);
    set_env_value("KC_CHAT_END_TOKEN", NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_stop.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_stop(void) {
    kc_chat_options_t opts;
    kc_chat_t *first;
    kc_chat_t *second;
    char *out;
    int rc;

    opts = make_cat_options();
    first = NULL;
    second = NULL;
    out = NULL;
    rc = 0;
    if (opts.cmd == NULL) return 1;
    rc += expect_int("stop NULL ctx", KC_CHAT_ERROR, kc_chat_stop(NULL));
    rc += expect_int("open first context", KC_CHAT_OK, kc_chat_open(&first, &opts));
    rc += expect_int("open second context", KC_CHAT_OK, kc_chat_open(&second, &opts));
    rc += expect_int("stop first context", KC_CHAT_OK, kc_chat_stop(first));
    rc += expect_int("stop first context again", KC_CHAT_OK, kc_chat_stop(first));
#ifdef _WIN32
    rc += expect_int("windows exec remains unsupported after stop", KC_CHAT_ERROR,
        kc_chat_exec(second, "hello second", &out));
    rc += expect_true("windows exec keeps output NULL", out == NULL);
#else
    rc += expect_int("second context still executes after first stop", KC_CHAT_OK,
        kc_chat_exec(second, "hello second", &out));
    rc += expect_contains("second output after first stop", out, "hello second");
    kc_chat_free(out);
#endif
    rc += expect_int("stop second context", KC_CHAT_OK, kc_chat_stop(second));
    kc_chat_close(first);
    kc_chat_close(second);
    kc_chat_options_free(&opts);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_chat_version.
 * @return 0 on success, 1 on failure.
 */
static int case_kc_chat_version(void) {
    return expect_true("version is non-zero", kc_chat_version() != 0U);
}

/**
 * Runs one public API contract test case.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 or 2 on failure.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "test case: expected one argument, got %d\n", argc - 1);
        return 2;
    }
    if (strcmp(argv[1], "kc_chat_open") == 0) return case_kc_chat_open();
    if (strcmp(argv[1], "kc_chat_close") == 0) return case_kc_chat_close();
    if (strcmp(argv[1], "kc_chat_exec") == 0) return case_kc_chat_exec();
    if (strcmp(argv[1], "kc_chat_is_exit") == 0) return case_kc_chat_is_exit();
    if (strcmp(argv[1], "kc_chat_free") == 0) return case_kc_chat_free();
    if (strcmp(argv[1], "kc_chat_options_default") == 0) {
        return case_kc_chat_options_default();
    }
    if (strcmp(argv[1], "kc_chat_options_load_env") == 0) {
        return case_kc_chat_options_load_env();
    }
    if (strcmp(argv[1], "kc_chat_options_free") == 0) {
        return case_kc_chat_options_free();
    }
    if (strcmp(argv[1], "kc_chat_stop") == 0) return case_kc_chat_stop();
    if (strcmp(argv[1], "kc_chat_version") == 0) return case_kc_chat_version();
    fprintf(stderr, "unknown test case: %s\n", argv[1]);
    return 2;
}

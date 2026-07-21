/**
 * libchat.c - Chat Loop Delegator
 * Summary: Core implementation for the chat library.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#endif

#include "libchat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <signal.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stddef.h>
#include <stdint.h>

#ifndef KC_CHAT_BUILD_VERSION
#define KC_CHAT_BUILD_VERSION 0
#endif

/**
 * Returns the build version generated at compile time.
 * @return Unix timestamp for the current build.
 */
uint64_t kc_chat_version(void) {
    return (uint64_t)KC_CHAT_BUILD_VERSION;
}

typedef enum {
    KC_ENV_TYPE_INT,
    KC_ENV_TYPE_FLOAT,
    KC_ENV_TYPE_STR
} kc_env_type_t;

typedef struct {
    const char *env_var;
    size_t offset;
    kc_env_type_t type;
} kc_env_map_t;

static const kc_env_map_t env_config_table[] = {
    { "KC_CHAT_CMD",       offsetof(kc_chat_options_t, cmd),       KC_ENV_TYPE_STR },
    { "KC_CHAT_END_TOKEN", offsetof(kc_chat_options_t, end_token), KC_ENV_TYPE_STR },
    { "KC_CHAT_EXIT_CMD",  offsetof(kc_chat_options_t, exit_cmd),  KC_ENV_TYPE_STR },
    { "KC_CHAT_MSG",       offsetof(kc_chat_options_t, msg),       KC_ENV_TYPE_STR },
    { "KC_CHAT_PROMPT",    offsetof(kc_chat_options_t, prompt),    KC_ENV_TYPE_STR },
};
static const int env_config_table_n =
    sizeof(env_config_table) / sizeof(env_config_table[0]);

struct kc_chat {
    kc_chat_options_t opts;
    volatile sig_atomic_t stop_requested;
};

/**
 * Initialize a new chat context.
 * @param out Pointer to receive the context pointer.
 * @param opts Options.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_open(kc_chat_t **out, const kc_chat_options_t *opts) {
    kc_chat_t *ctx;

    if (!out || !opts) {
        return KC_CHAT_ERROR;
    }

    ctx = (kc_chat_t *)calloc(1, sizeof(kc_chat_t));
    if (!ctx) {
        return KC_CHAT_ERROR;
    }

    ctx->opts = *opts;
    ctx->opts.cmd = opts->cmd ? strdup(opts->cmd) : NULL;
    ctx->opts.end_token = opts->end_token ? strdup(opts->end_token) : NULL;
    ctx->opts.exit_cmd = opts->exit_cmd ? strdup(opts->exit_cmd) : NULL;
    ctx->opts.msg = opts->msg ? strdup(opts->msg) : NULL;
    ctx->opts.prompt = opts->prompt ? strdup(opts->prompt) : NULL;

    *out = ctx;
    return KC_CHAT_OK;
}

/**
 * Release a chat context.
 * @param ctx Context pointer.
 * @return None.
 */
void kc_chat_close(kc_chat_t *ctx) {
    if (!ctx) {
        return;
    }

    kc_chat_options_free(&ctx->opts);
    free(ctx);
}

/**
 * Read all bytes from a file descriptor into an owned buffer.
 * @param fd Source file descriptor.
 * @param out Receives the allocated buffer (null-terminated).
 * @return 0 on success, -1 on failure.
 */
#ifndef _WIN32
static int kc_chat_read_fd(int fd, char **out) {
    char buf[4096];
    char *data = NULL;
    size_t used = 0;
    size_t cap = 0;
    ssize_t n;

    if (!out) {
        return -1;
    }

    for (;;) {
        n = read(fd, buf, sizeof(buf));

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            free(data);
            return -1;
        }

        if (n == 0) {
            break;
        }

        if (used + (size_t)n + 1 > cap) {
            size_t next_cap = cap == 0 ? 4096 : cap * 2;
            char *next;

            while (next_cap < used + (size_t)n + 1) {
                next_cap *= 2;
            }

            next = (char *)realloc(data, next_cap);
            if (!next) {
                free(data);
                return -1;
            }

            data = next;
            cap = next_cap;
        }

        memcpy(data + used, buf, (size_t)n);
        used += (size_t)n;
    }

    if (!data) {
        data = (char *)malloc(1);
        if (!data) {
            return -1;
        }
    }

    data[used] = '\0';
    *out = data;
    return 0;
}
#endif

/**
 * Execute a chat turn by delegating input to the configured command.
 * @param ctx Context pointer.
 * @param input Null-terminated input string.
 * @param output Receives the owned command output string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_exec(kc_chat_t *ctx, const char *input, char **output) {
#ifndef _WIN32
    int stdin_pipe[2];
    int stdout_pipe[2];
    pid_t pid;
    int status;
    size_t input_len;
    size_t written;
    ssize_t nw;

    if (!ctx || !ctx->opts.cmd || !input || !output) {
        return KC_CHAT_ERROR;
    }

    *output = NULL;

    if (pipe(stdin_pipe) != 0) {
        return KC_CHAT_ERROR;
    }

    if (pipe(stdout_pipe) != 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return KC_CHAT_ERROR;
    }

    pid = fork();

    if (pid < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return KC_CHAT_ERROR;
    }

    if (pid == 0) {
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) {
            _exit(1);
        }

        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) {
            _exit(1);
        }

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        execl("/bin/sh", "sh", "-c", ctx->opts.cmd, (char *)NULL);
        _exit(1);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    input_len = strlen(input);
    written = 0;

    while (written < input_len) {
        nw = write(stdin_pipe[1], input + written, input_len - written);

        if (nw < 0) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        written += (size_t)nw;
    }

    close(stdin_pipe[1]);

    if (kc_chat_read_fd(stdout_pipe[0], output) != 0) {
        close(stdout_pipe[0]);
        waitpid(pid, &status, 0);
        return KC_CHAT_ERROR;
    }

    close(stdout_pipe[0]);

    if (waitpid(pid, &status, 0) < 0) {
        return KC_CHAT_ERROR;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return KC_CHAT_ERROR;
    }

    return KC_CHAT_OK;
#else
    (void)ctx;
    (void)input;
    (void)output;
    return KC_CHAT_ERROR;
#endif
}

/**
 * Check whether the given input matches the configured exit command.
 * @param ctx Context pointer.
 * @param input Input string to check.
 * @return 1 if input matches exit command, 0 otherwise.
 */
int kc_chat_is_exit(kc_chat_t *ctx, const char *input) {
    if (!ctx || !ctx->opts.exit_cmd || !input) {
        return 0;
    }

    return strcmp(input, ctx->opts.exit_cmd) == 0;
}

/**
 * Release a string allocated by the chat library.
 * @param text Owned string returned by the API.
 * @return None.
 */
void kc_chat_free(char *text) {
    free(text);
}

/**
 * Create an options struct initialized with default values.
 * @param none Unused.
 * @return Default-initialized options.
 */
kc_chat_options_t kc_chat_options_default(void) {
    kc_chat_options_t opts;
    memset(&opts, 0, sizeof(opts));
    return opts;
}

/**
 * Load configuration from environment variables.
 * @param opts Options to update.
 * @return None.
 */
void kc_chat_options_load_env(kc_chat_options_t *opts) {
    int i;

    if (!opts) {
        return;
    }

    for (i = 0; i < env_config_table_n; i++) {
        const char *val = getenv(env_config_table[i].env_var);
        char *end;

        if (!val) {
            continue;
        }

        switch (env_config_table[i].type) {
            case KC_ENV_TYPE_INT: {
                long v = strtol(val, &end, 10);
                if (end != val && *end == '\0') {
                    *(int *)((char *)opts + env_config_table[i].offset) = (int)v;
                }
                break;
            }
            case KC_ENV_TYPE_FLOAT: {
                float v = strtof(val, &end);
                if (end != val && *end == '\0') {
                    *(float *)((char *)opts + env_config_table[i].offset) = v;
                }
                break;
            }
            case KC_ENV_TYPE_STR: {
                char **p = (char **)((char *)opts + env_config_table[i].offset);
                free(*p);
                *p = strdup(val);
                break;
            }
        }
    }
}

/**
 * Request stop for a specific chat context.
 * @param ctx Chat context.
 * @return KC_CHAT_OK on success, KC_CHAT_ERROR on failure.
 */
int kc_chat_stop(kc_chat_t *ctx) {
    if (!ctx) return KC_CHAT_ERROR;
    ctx->stop_requested = 1;
    return KC_CHAT_OK;
}

/**
 * Free dynamically allocated resources within an options struct.
 * @param opts Options to clean up.
 * @return None.
 */
void kc_chat_options_free(kc_chat_options_t *opts) {
    if (!opts) {
        return;
    }

    free(opts->cmd);
    opts->cmd = NULL;
    free(opts->end_token);
    opts->end_token = NULL;
    free(opts->exit_cmd);
    opts->exit_cmd = NULL;
    free(opts->msg);
    opts->msg = NULL;
    free(opts->prompt);
    opts->prompt = NULL;
}

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

#include "chat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

struct kc_chat {
    char *cmd;
    char *exit_cmd;
};

/**
 * Initialize a new chat context.
 * @param none Unused.
 * @return Context pointer or NULL on failure.
 */
kc_chat_t *kc_chat_open(void) {
    return (kc_chat_t *)calloc(1, sizeof(kc_chat_t));
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

    free(ctx->cmd);
    free(ctx->exit_cmd);
    free(ctx);
}

/**
 * Set the command to delegate input to.
 * @param ctx Context pointer.
 * @param cmd Shell command string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on invalid input.
 */
int kc_chat_set_cmd(kc_chat_t *ctx, const char *cmd) {
    char *dup;

    if (!ctx || !cmd) {
        return KC_CHAT_ERROR;
    }

    dup = strdup(cmd);
    if (!dup) {
        return KC_CHAT_ERROR;
    }

    free(ctx->cmd);
    ctx->cmd = dup;
    return KC_CHAT_OK;
}

/**
 * Set the exit command text.
 * @param ctx Context pointer.
 * @param exit_cmd Exit command string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on invalid input.
 */
int kc_chat_set_exit(kc_chat_t *ctx, const char *exit_cmd) {
    char *dup;

    if (!ctx || !exit_cmd) {
        return KC_CHAT_ERROR;
    }

    dup = strdup(exit_cmd);
    if (!dup) {
        return KC_CHAT_ERROR;
    }

    free(ctx->exit_cmd);
    ctx->exit_cmd = dup;
    return KC_CHAT_OK;
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

    if (!ctx || !ctx->cmd || !input || !output) {
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
        execl("/bin/sh", "sh", "-c", ctx->cmd, (char *)NULL);
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
    if (!ctx || !ctx->exit_cmd || !input) {
        return 0;
    }

    return strcmp(input, ctx->exit_cmd) == 0;
}

/**
 * Release a string allocated by the chat library.
 * @param text Owned string returned by the API.
 * @return None.
 */
void kc_chat_free(char *text) {
    free(text);
}

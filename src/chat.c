/**
 * chat.c - Chat Loop Delegator
 * Summary: Command line interface for the chat tool.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define KC_CHAT_READ  _read
#define KC_CHAT_STDIN_FD 0
#else
#include <unistd.h>
#define KC_CHAT_READ  read
#define KC_CHAT_STDIN_FD STDIN_FILENO
#endif

#define KC_CHAT_DEFAULT_PROMPT "> "
#define KC_CHAT_DEFAULT_EXIT   "exit"

/**
 * Read a single line from standard input.
 * @param out Receives the allocated line (without newline).
 * @return 0 on success, 1 on EOF, -1 on error.
 */
static int kc_chat_read_line(char **out) {
    char *buf = NULL;
    size_t cap = 0;
    size_t used = 0;
    int c;

    if (!out) {
        return -1;
    }

    for (;;) {
        c = fgetc(stdin);

        if (c == EOF) {
            if (used == 0) {
                free(buf);
                *out = NULL;
                return 1;
            }

            break;
        }

        if (c == '\n') {
            break;
        }

        if (used + 2 > cap) {
            size_t next_cap = cap == 0 ? 128 : cap * 2;
            char *next = (char *)realloc(buf, next_cap);

            if (!next) {
                free(buf);
                return -1;
            }

            buf = next;
            cap = next_cap;
        }

        buf[used++] = (char)c;
    }

    if (!buf) {
        buf = (char *)malloc(1);
        if (!buf) {
            return -1;
        }
    }

    buf[used] = '\0';
    *out = buf;
    return 0;
}

/**
 * Read input from stdin until an end token is found, or EOF.
 * @param end_token Token that signals end of input.
 * @param out Receives the allocated multi-line text (without the end token).
 * @return 0 on success, 1 on EOF before any data, -1 on error.
 */
static int kc_chat_read_end(const char *end_token, char **out) {
    char *buf = NULL;
    size_t cap = 0;
    size_t used = 0;
    char *line = NULL;
    int has_data = 0;
    int rc;

    if (!out) {
        return -1;
    }

    for (;;) {
        rc = kc_chat_read_line(&line);

        if (rc < 0) {
            free(buf);
            return -1;
        }

        if (rc == 1) {
            break;
        }

        if (end_token && strcmp(line, end_token) == 0) {
            free(line);
            line = NULL;
            break;
        }

        if (used + strlen(line) + 2 > cap) {
            size_t next_cap = cap == 0 ? 256 : cap * 2;
            char *next;

            while (next_cap < used + strlen(line) + 2) {
                next_cap *= 2;
            }

            next = (char *)realloc(buf, next_cap);
            if (!next) {
                free(buf);
                free(line);
                return -1;
            }

            buf = next;
            cap = next_cap;
        }

        if (has_data) {
            buf[used++] = '\n';
        }

        memcpy(buf + used, line, strlen(line));
        used += strlen(line);
        has_data = 1;
        free(line);
        line = NULL;
    }

    free(line);

    if (!has_data) {
        free(buf);
        *out = NULL;
        return 1;
    }

    buf[used] = '\0';
    *out = buf;
    return 0;
}

/**
 * Read one input payload from stdin.
 * If end_token is set, read multi-line until the token.
 * Otherwise, read a single line.
 * @param end_token End token or NULL for single-line mode.
 * @param out Receives the allocated input text.
 * @return 0 on success, 1 on EOF, -1 on error.
 */
static int kc_chat_read_input(const char *end_token, char **out) {
    if (end_token && *end_token) {
        return kc_chat_read_end(end_token, out);
    }

    return kc_chat_read_line(out);
}

/**
 * Print command usage information.
 * @param name Program executable name.
 * @return None.
 */
static void kc_print_help(const char *name) {
    printf("Usage: %s [options] <command>\n", name);
    printf("\n");
    printf("Parameters:\n");
    printf("    <command>      Command to execute with piped input\n");
    printf("\n");
    printf("Options:\n");
    printf("    -e <token>     End token for multi-line input\n");
    printf("    -x <text>      Exit command (default: exit)\n");
    printf("    -m <text>      Initial message\n");
    printf("    -p <text>      Prompt text (default: > )\n");
    printf("    -h, --help     Show this help\n");
    printf("    -v, --version  Show version\n");
}

/**
 * Print command version information.
 * @return None.
 */
static void kc_print_version(void) {
    printf("chat build %llu\n", (unsigned long long)kc_chat_version());
}

/**
 * Execute the command line interface.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status code.
 */
int main(int argc, char **argv) {
    kc_chat_options_t opts = kc_chat_options_default();
    kc_chat_t *ctx = NULL;
    int i;

    kc_chat_options_load_env(&opts);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            kc_print_help(argv[0]);
            return 0;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            kc_print_version();
            return 0;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "-x") == 0 ||
                strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "-p") == 0) {
                i++;
            }

            continue;
        }

        free(opts.cmd);
        opts.cmd = strdup(argv[i]);
        break;
    }

    if (!opts.cmd) {
        kc_print_help(argv[0]);
        kc_chat_options_free(&opts);
        return 1;
    }

    if (!opts.exit_cmd) {
        opts.exit_cmd = strdup(KC_CHAT_DEFAULT_EXIT);
    }
    if (!opts.prompt) {
        opts.prompt = strdup(KC_CHAT_DEFAULT_PROMPT);
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return 0;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            return 0;
        }

        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            continue;
        }

        if (strcmp(argv[i], "-e") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "chat: missing value for -e\n");
                kc_chat_options_free(&opts);
                return 1;
            }
            free(opts.end_token);
            opts.end_token = strdup(argv[i]);
        } else if (strcmp(argv[i], "-x") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "chat: missing value for -x\n");
                kc_chat_options_free(&opts);
                return 1;
            }
            free(opts.exit_cmd);
            opts.exit_cmd = strdup(argv[i]);
        } else if (strcmp(argv[i], "-m") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "chat: missing value for -m\n");
                kc_chat_options_free(&opts);
                return 1;
            }
            free(opts.msg);
            opts.msg = strdup(argv[i]);
        } else if (strcmp(argv[i], "-p") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "chat: missing value for -p\n");
                kc_chat_options_free(&opts);
                return 1;
            }
            free(opts.prompt);
            opts.prompt = strdup(argv[i]);
        } else {
            fprintf(stderr, "chat: unknown option '%s'\n", argv[i]);
            kc_chat_options_free(&opts);
            return 1;
        }
    }

    if (kc_chat_open(&ctx, &opts) != KC_CHAT_OK) {
        fprintf(stderr, "chat: out of memory\n");
        kc_chat_options_free(&opts);
        return 1;
    }

    kc_chat_listen_signals(ctx);
#ifndef _WIN32
    kc_chat_listen_signal(ctx, 2);
    kc_chat_listen_signal(ctx, 15);
#endif

    if (opts.msg && *opts.msg) {
        printf("%s\n", opts.msg);
    }

    for (;;) {
        char *input = NULL;
        char *output = NULL;
        int rc;

        printf("%s", opts.prompt);
        fflush(stdout);

        rc = kc_chat_read_input(opts.end_token, &input);

        if (rc < 0) {
            fprintf(stderr, "chat: failed to read input\n");
            kc_chat_close(ctx);
            kc_chat_options_free(&opts);
            return 1;
        }

        if (rc == 1) {
            break;
        }

        if (!opts.end_token || !*opts.end_token) {
            if (kc_chat_is_exit(ctx, input)) {
                free(input);
                break;
            }
        }

        if (kc_chat_exec(ctx, input, &output) != KC_CHAT_OK) {
            kc_chat_free(input);
            kc_chat_close(ctx);
            kc_chat_options_free(&opts);
            return 1;
        }

        if (output) {
            printf("%s\n", output);
            kc_chat_free(output);
        }

        free(input);
    }

    kc_chat_close(ctx);
    kc_chat_options_free(&opts);
    return 0;
}

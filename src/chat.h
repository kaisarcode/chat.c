/**
 * chat.h - Chat Loop Delegator
 * Summary: Public API for the chat library.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef KC_CHAT_H
#define KC_CHAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kc_chat kc_chat_t;

#define KC_CHAT_OK      0
#define KC_CHAT_ERROR  -1

/**
 * The library copies this structure during kc_chat_open(); the caller may
 * release its copy immediately after the call.
 *
 * Owned string fields are freed by kc_chat_options_free().
 *
 * @param cmd Shell command string.
 * @param end_token End token for multi-line input.
 * @param exit_cmd Exit command string.
 * @param msg Initial message.
 * @param prompt Prompt text.
 */
typedef struct kc_chat_options {
    char *cmd;
    char *end_token;
    char *exit_cmd;
    char *msg;
    char *prompt;
} kc_chat_options_t;

/**
 * Callback type for library-level signal handling.
 * Registered via kc_chat_on_signal; invoked by kc_chat_raise_signal.
 * @param ctx Chat context.
 */
typedef void (*kc_chat_signal_callback_t)(kc_chat_t *ctx);

/**
 * Initialize a new chat context.
 * @param out Pointer to receive the context pointer.
 * @param opts Options.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_open(kc_chat_t **out, const kc_chat_options_t *opts);

/**
 * Release a chat context.
 * @param ctx Context pointer.
 * @return None.
 */
void kc_chat_close(kc_chat_t *ctx);

/**
 * Execute a chat turn by delegating input to the configured command.
 * @param ctx Context pointer.
 * @param input Null-terminated input string.
 * @param output Receives the owned command output string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_exec(kc_chat_t *ctx, const char *input, char **output);

/**
 * Check whether the given input matches the configured exit command.
 * @param ctx Context pointer.
 * @param input Input string to check.
 * @return 1 if input matches exit command, 0 otherwise.
 */
int kc_chat_is_exit(kc_chat_t *ctx, const char *input);

/**
 * Release a string allocated by the chat library.
 * @param text Owned string returned by the API.
 * @return None.
 */
void kc_chat_free(char *text);

/**
 * Create an options struct initialized with default values.
 * All string fields are NULL, numeric fields match the CLI defaults.
 * @return Default-initialized options.
 */
kc_chat_options_t kc_chat_options_default(void);

/**
 * Load configuration from environment variables.
 * Overrides values in opts if the corresponding env var is set.
 * @param opts Options to update.
 * @return None.
 */
void kc_chat_options_load_env(kc_chat_options_t *opts);

/**
 * Free dynamically allocated resources within an options struct.
 * After this call, opts is safe to reuse or discard.
 * @param opts Options to clean up (may be NULL).
 * @return None.
 */
void kc_chat_options_free(kc_chat_options_t *opts);

/**
 * Register a handler for a library-level signal number.
 *
 * @param ctx Chat context.
 * @param sig Application-defined signal number.
 * @param cb Callback to invoke, or NULL to unregister.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_on_signal(kc_chat_t *ctx, int sig, kc_chat_signal_callback_t cb);

/**
 * Raise a library-level signal.
 *
 * @param ctx Chat context.
 * @param sig Signal number to raise.
 * @return KC_CHAT_OK if a handler was called, or KC_CHAT_ERROR.
 */
int kc_chat_raise_signal(kc_chat_t *ctx, int sig);

/**
 * Set the internal signal-listener context.
 *
 * @param ctx Chat context.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR if ctx is NULL.
 */
int kc_chat_listen_signals(kc_chat_t *ctx);

/**
 * Wire an OS signal to the library signal listener.
 *
 * @param ctx Chat context.
 * @param sig_id OS signal number (e.g. SIGHUP, SIGINT, SIGTERM).
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on failure.
 */
int kc_chat_listen_signal(kc_chat_t *ctx, int sig_id);

/**
 * Generic signal-listener compatible with signal() / sigaction().
 *
 * @param sig OS signal number.
 * @return None.
 */
void kc_chat_signal_listener(int sig);

/**
 * Request stop for a specific chat context.
 * @param ctx Chat context.
 * @return KC_CHAT_OK on success, KC_CHAT_ERROR on failure.
 */
int kc_chat_stop(kc_chat_t *ctx);

/**
 * Returns the build version generated at compile time.
 * @return Unix timestamp for the current build.
 */
uint64_t kc_chat_version(void);

#ifdef __cplusplus
}
#endif

#endif

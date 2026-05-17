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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kc_chat kc_chat_t;

#define KC_CHAT_OK      0
#define KC_CHAT_ERROR  -1

/**
 * Initialize a new chat context.
 * @param none Unused.
 * @return Context pointer or NULL on failure.
 */
kc_chat_t *kc_chat_open(void);

/**
 * Release a chat context.
 * @param ctx Context pointer.
 * @return None.
 */
void kc_chat_close(kc_chat_t *ctx);

/**
 * Set the command to delegate input to.
 * @param ctx Context pointer.
 * @param cmd Shell command string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on invalid input.
 */
int kc_chat_set_cmd(kc_chat_t *ctx, const char *cmd);

/**
 * Set the exit command text.
 * @param ctx Context pointer.
 * @param exit_cmd Exit command string.
 * @return KC_CHAT_OK on success, or KC_CHAT_ERROR on invalid input.
 */
int kc_chat_set_exit(kc_chat_t *ctx, const char *exit_cmd);

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

#ifdef __cplusplus
}
#endif

#endif

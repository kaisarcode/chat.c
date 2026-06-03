# CHANGELOG

## v1.1.1

- Fixed `chat_signal_listener` to restore default signal behavior (SIG_DFL)
  when no `on_signal` handler is registered. Moved `<signal.h>` include to
  be unconditional for MinGW compatibility.

## v1.1.0

- Added data-driven configuration with table-driven environment variable loading.
- Added `kc_chat_options_default()`, `kc_chat_options_load_env()`, and `kc_chat_options_free()` to the public API.
- Refactored `kc_chat_open()` to take `kc_chat_options_t`.
- CLI is now decoupled from `libchat`; configuration is initialized through options, then overridden by flags.
- Env vars: `KC_CHAT_CMD`, `KC_CHAT_END_TOKEN`, `KC_CHAT_EXIT_CMD`, `KC_CHAT_MSG`, `KC_CHAT_PROMPT`.
- Added signal listener lifecycle: `kc_chat_on_signal()`, `kc_chat_raise_signal()`, `kc_chat_listen_signals()`, `kc_chat_listen_signal()`, and `kc_chat_signal_listener()`.

## v1.0.0

- Published the stable baseline release.
- Provided an interactive chat loop delegator with command execution through pipes.
- Supported prompt, initial message, end token, and exit command configuration.
- Offered a public C API with context lifecycle management.

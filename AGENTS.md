# chat.c — Chat Loop Delegator

## Overview
Repeatedly reads input from stdin, delegates it to a configurable shell command via pipe, and displays the command's output. Provides an interactive chat loop with configurable prompt, exit command, end token for multi-line input, and initial message.

## Architecture
Three-file split: `chat.h` exposes the opaque `kc_chat_t` type and lifecycle/config/exec functions. `libchat.c` implements the library — spawns `/bin/sh -c "<cmd>"` via fork/exec with piped stdin and captured stdout. `chat.c` is the CLI layer — parses argv, runs the interactive loop (prompt, read input, delegate, display output, repeat), reports errors to stderr. Build system produces static lib, shared lib, and executable for 17 target triples.

## Directory Layout
| Path | Contents |
|------|----------|
| `src/chat.h` | Public API — opaque type, status codes, function declarations |
| `src/libchat.c` | Library implementation — context struct, set_cmd/set_exit/exec/is_exit/free |
| `src/chat.c` | CLI entry point — argv parsing, interactive loop, help/version |
| `Makefile` | Cross-compilation builder (17 targets) via CMake + Ninja |
| `CMakeLists.txt` | CMake project definition (C11, static + shared + exe) |
| `test.sh` | Shell test suite — binary check, CLI errors, delegation, exit, end token, prompt, message |
| `README.md` | Project documentation and usage examples |
| `LICENSE` | GPL v3.0 |
| `.kcsignore` | KCS exclusion list |

## Data Model
### Internal Structures
| Symbol | Type | Role |
|--------|------|------|
| `kc_chat_t` (opaque) | `struct kc_chat` | Allocated context |
| `struct kc_chat` | `{ char *cmd; char *exit_cmd; }` | Shell command string, exit command string |

### Hard Limits
| Limit | Value | Symbol |
|-------|-------|--------|
| Pipe read chunk | 4096 bytes | `buf` (local, `kc_chat_read_fd`) |
| Pipe read initial capacity | 4096 bytes | implicit in growth logic |
| Pipe read max capacity | unbounded (doubles until allocation fails) | implicit |
| Line read initial capacity | 128 bytes | implicit in `kc_chat_read_line` |

## Public API
| Function | Returns | Description |
|----------|---------|-------------|
| `kc_chat_open(void)` | `kc_chat_t *` | Allocate and return new zero-initialized context; returns NULL on failure |
| `kc_chat_close(kc_chat_t *ctx)` | `void` | Free context and owned strings; safe on NULL |
| `kc_chat_set_cmd(kc_chat_t *ctx, const char *cmd)` | `int` | Set shell command; returns `KC_CHAT_OK` or `KC_CHAT_ERROR` |
| `kc_chat_set_exit(kc_chat_t *ctx, const char *exit_cmd)` | `int` | Set exit command; returns `KC_CHAT_OK` or `KC_CHAT_ERROR` |
| `kc_chat_exec(kc_chat_t *ctx, const char *input, char **output)` | `int` | Pipe input to command, capture output; returns `KC_CHAT_OK` or `KC_CHAT_ERROR` |
| `kc_chat_is_exit(kc_chat_t *ctx, const char *input)` | `int` | Check if input matches exit command; returns 1 or 0 |
| `kc_chat_free(char *text)` | `void` | Free output string; safe on NULL |

## CLI
| Argument | Description |
|----------|-------------|
| `<command>` | Shell command to execute (positional, required) |
| `-e <token>` | End token for multi-line input |
| `-x <text>` | Exit command (default: `exit`) |
| `-m <text>` | Initial message printed at start |
| `-p <text>` | Prompt text (default: `> `) |
| `-h`, `--help` | Print usage and exit 0 |
| `-v`, `--version` | Print version (`chat 1.1.0`) and exit 0 |

### Exit Codes
| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Error (missing command, missing flag value, unknown option, OOM, read failure, exec failure) |

## Build
| Target | Description |
|--------|-------------|
| `make` (default: `native`) | Build for host arch/platform |
| `make all` | Build full 17-target cross-compilation matrix |
| `make x86_64/linux` | Linux x86_64 |
| `make x86_64/windows` | Windows x86_64 (MinGW) |
| `make i686/linux` | Linux i686 |
| `make i686/windows` | Windows i686 (MinGW) |
| `make aarch64/linux` | Linux aarch64 |
| `make aarch64/android` | Android aarch64 (NDK) |
| `make armv7/linux` | Linux armv7 (soft float) |
| `make armv7/android` | Android armv7 (NDK) |
| `make armv7hf/linux` | Linux armv7hf (hard float) |
| `make riscv64/linux` | Linux riscv64 |
| `make powerpc64le/linux` | Linux ppc64le |
| `make mips/linux` | Linux mips |
| `make mipsel/linux` | Linux mipsel |
| `make mips64el/linux` | Linux mips64el |
| `make s390x/linux` | Linux s390x |
| `make loongarch64/linux` | Linux loongarch64 |
| `make test` | Run `sh test.sh` |
| `make clean` | Remove `.build/` |

## Error Handling
| Condition | Stderr Message | Exit Code | Symbol |
|-----------|----------------|-----------|--------|
| Missing command | (prints help) | 1 | — |
| Missing value for flag | `chat: missing value for <flag>` | 1 | — |
| Unknown option | `chat: unknown option '<opt>'` | 1 | — |
| stdin read failure | `chat: failed to read input` | 1 | — |
| `kc_chat_open` returns NULL | `chat: out of memory` | 1 | — |
| `kc_chat_exec` returns error | (implicit from delegated command) | 1 | `KC_CHAT_ERROR` |
| Null input to exec | (internal, returns error) | N/A | `KC_CHAT_ERROR` |

## Constraints
- Requires POSIX `fork`/`pipe`/`dup2`/`exec`/`waitpid`; not available on Windows without compatibility layer.
- Delegates via `/bin/sh -c "<cmd>"`; command is interpreted by the system shell.
- Reads entire output into memory before returning; no streaming mode.
- CLI reads entire stdin payload per turn before delegating; no streaming.
- No thread-safety guarantees on the context object.

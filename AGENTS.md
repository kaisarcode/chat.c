# AGENTS.md

## Project Context

`chat.c` is a small C library and CLI that reads human-entered text one turn at
a time and delegates each turn to a configured shell command.

It is not a chat service, model runtime, conversation database, agent framework,
terminal emulator, or enterprise messaging platform.

Read `README.md` and `DESIGN.md` before modifying the project.

## Core Invariants

- The CLI owns prompting, line collection, exit matching, and display.
- The library owns one command invocation and stdout capture per turn.
- Each POSIX turn starts a fresh `/bin/sh -c` child.
- No command process or conversation state persists between turns.
- Input is one null-terminated string and cannot contain embedded NUL bytes.
- The library adds no newline to command stdin.
- Captured stdout is returned as one allocated null-terminated string.
- stderr remains outside the captured output stream.
- A nonzero or abnormal child exit makes the turn fail.
- Options are deep-copied into each context.
- Exit matching is exact string equality and is a CLI policy.
- No account, network service, model provider, or hosted dependency is required.

## Delegation Boundary

The configured command is trusted operator input and is executed by the platform
shell on POSIX. User turn text goes only to child stdin and must never be
concatenated into command syntax.

`chat` does not define prompts for language models, roles, message history,
tokenization, streaming protocols, tool calls, retries, memory, retrieval,
moderation, authentication, or provider APIs. Compose those behaviors through
the delegated command.

Do not add command registries, plugins, remote backends, embedded model engines,
or generic agent abstractions.

## Input and Output Semantics

Single-line mode removes the input newline. Multi-line mode joins accepted lines
with one newline between lines, excludes the end-token line, and adds no final
newline. Empty input before EOF or an immediate end token ends the CLI loop.

The exit command is checked only in single-line mode. Matching is exact and
does not trim whitespace or ignore case.

The CLI prints one prompt before each read and appends one newline after captured
output. The library itself preserves stdout bytes up to the first NUL required by
its string API and does not append presentation formatting.

## Process and Resource Model

POSIX creates two pipes and one process for each turn. The parent writes all
input, closes child stdin, then reads all stdout, then waits. This ordering can
deadlock when a child writes enough output before consuming all input.

Input lines, multi-line payloads, and captured output currently grow without a
configured limit. Child execution and pipe reads have no timeout. These limits
must remain explicit.

A concrete fix may use bounded full-duplex relay or explicit limits, but must not
add hidden queues, background daemons, remote coordination, or silent truncation.

The stop flag is currently not consulted by execution or the CLI loop. Do not
describe it as cancellation until child termination and pipe cleanup are defined
and tested.

## Platform Truth

`kc_chat_exec()` is implemented only on POSIX. Windows returns
`KC_CHAT_ERROR` without creating a command. Cross-compilation and passing error
path tests do not establish Windows chat functionality.

Do not hide shell, pipe, signal, exit-status, or text-encoding differences behind
portable claims.

## Public API and Ownership

Treat `src/libchat.h` as a compatibility boundary. Preserve deep-copy behavior,
exact exit matching, allocated output ownership, `kc_chat_free()`, return codes,
environment options, and per-turn process behavior unless explicitly instructed.

On command failure, captured stdout may still be returned and remains
caller-owned. Callers must free every non-NULL output even when the function
returns an error.

## Source Layout

Preserve exactly:

- `src/chat.c` for CLI parsing, input collection, prompts, and display;
- `src/libchat.c` for command execution and reusable context behavior;
- `src/libchat.h` for the public API;
- `src/test.c` for all tests, including process, stress, platform, and
    integration cases.

Do not create additional source, header, shell, process, input, output, or test
files. Extend only the existing four files.

## Forbidden Default Recommendations

Do not add model SDKs, hosted AI APIs, conversation databases, vector stores,
retrieval systems, agent frameworks, tool registries, messaging servers, user
accounts, OAuth, SSO, tenant models, telemetry, analytics, dashboards, cloud
dependencies, plugins, or provider abstractions.

Do not justify changes through enterprise readiness, chatbot feature parity,
hypothetical scale, managed operation, or platform growth.

## Testing

All tests remain in `src/test.c`. Behavioral changes should cover deep-copied
options, exact input bytes, no implicit newline, empty input, multi-line joining,
exit matching, partial writes, large stdout, simultaneous large input and output,
stderr separation, child launch failure, nonzero and signaled exit, output on
failure, allocation errors, stop behavior, and POSIX and Windows truth.

Do not weaken tests to accommodate an implementation change.

## Build and Completion

For documentation-only changes run `kcs .`. For behavior changes use the
repository build and tests without cleaning unless authorized.

A change is complete when turn bytes, process lifecycle, output ownership,
blocking limits, platform support, tests, and documentation agree.

The goal is one small interactive shell-delegation loop.

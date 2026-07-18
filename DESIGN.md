# chat.c Design

## Purpose

`chat.c` provides a minimal interactive loop around a shell command. It collects
one text payload, starts the command, sends the payload to stdin, captures stdout,
prints it, and repeats.

The word chat describes the user interaction shape. The library does not provide
conversation semantics or retain command state.

## Architecture

The CLI owns terminal-facing behavior. The library owns options, context
lifecycle, exact exit comparison, one POSIX child execution, output allocation,
and application-level signal callbacks.

The four source files have fixed responsibilities:

- `src/chat.c` owns argument parsing and interactive input;
- `src/libchat.c` owns per-turn delegation and output capture;
- `src/libchat.h` defines the public contract;
- `src/test.c` contains all tests.

## CLI Configuration

Environment variables may provide command, end token, exit command, initial
message, and prompt. CLI flags replace the corresponding values. Missing exit
and prompt values default to `exit` and `> `.

The current CLI identifies the command as the first positional argument and
stores only that one argument. Additional positional words are not joined into
the command. Commands containing spaces therefore require shell quoting as one
argument or an environment value.

The parser performs a second option pass across all arguments, so command
arguments that resemble chat flags are not an independent opaque argv vector.

## Single-Line Input

Single-line mode reads bytes with `fgetc()` until newline or EOF. The newline is
discarded. An empty line is a valid empty payload; EOF before any byte ends the
loop.

Before delegation, the CLI compares the payload exactly with the configured exit
command. A match exits without running the child.

## Multi-Line Input

When a non-empty end token is configured, the CLI reads complete lines until one
line exactly equals that token or stdin reaches EOF. The token is excluded.
Accepted lines are joined with one newline between them and no final newline.

An immediate token or EOF without accepted data ends the loop. Exit-command
matching is disabled in multi-line mode.

Input storage grows geometrically and has no configured maximum.

## POSIX Turn Execution

`kc_chat_exec()` validates a context, configured command, input, and output
destination. It creates stdin and stdout pipes and forks.

The child connects its stdin and stdout to those pipes and executes:

```text
/bin/sh -c <configured command>
```

stderr remains inherited. The parent writes `strlen(input)` bytes without a
terminating NUL or added newline, closes the input pipe, reads stdout until EOF,
waits for the child, and accepts only normal exit status zero.

Every turn creates a new child. Stateful commands must persist state elsewhere
or be composed through a separate resident-process tool such as `dmn`.

## Pipe Ordering

The parent completes input writing before reading stdout. A child that fills its
stdout pipe while waiting for more input can block the parent, which may itself
be blocked writing input. There is no select loop, thread, timeout, output limit,
or child-kill path.

This is a known hard edge. Any remedy must preserve exact bytes, child exit
status, deterministic descriptor cleanup, and explicit resource limits.

## Output Contract

Captured stdout grows dynamically in 4,096-byte read increments and is always
terminated with NUL. Empty stdout returns an allocated empty string.

The string API cannot represent embedded NUL output as a length-aware value,
although bytes after NUL may exist in allocated storage. Callers receive no
length and must treat output as text.

The output allocation belongs to the caller and is released with
`kc_chat_free()`. A failing child can still leave captured output in the output
pointer before `KC_CHAT_ERROR` is returned.

## Windows Behavior

Windows builds expose the same API, but `kc_chat_exec()` currently returns error
without executing a command. Interactive delegation is therefore not implemented
on Windows.

This difference must remain visible until a native implementation defines shell
selection, pipe ownership, exit status, output capture, cancellation, and runtime
tests.

## Context and Stop State

Opening deep-copies all option strings. Closing frees those copies, callback
storage, and global signal registration. Contexts do not own a persistent child
or conversation history.

`kc_chat_stop()` only sets context state. Current execution and CLI paths do not
read it. Signal callbacks are application-level dispatch and do not terminate an
active child.

## Composition

The delegated command owns all application semantics. It may invoke a local
model, formatter, database client, protocol filter, or another small tool.
`chat` remains responsible only for interactive text collection and one command
turn.

This keeps model providers, prompts, history, tools, and storage outside the
loop.

## Non-Goals

The project does not provide persistent conversations, resident child state,
language-model integration, roles, token streaming, tool calls, retrieval,
memory, user accounts, networking, a chat server, terminal emulation, job
control, command sandboxing, telemetry, hosted services, or plugins.

These exclusions define the tool rather than an unfinished roadmap.

## Change Criteria

A change must solve a concrete interactive delegation problem, preserve exact
line and turn semantics, define process and pipe ownership, bound or document
memory and blocking, propagate child status clearly, state platform behavior,
and avoid absorbing delegated application responsibilities.

Changes justified mainly by chatbot platforms, hosted AI ecosystems, enterprise
messaging, or generalized agents should be rejected.

## Core Invariants

The project is defined by CLI-owned prompts, exact exit matching, optional
end-token collection, one fresh POSIX shell child per turn, stdin text without an
implicit newline, allocated stdout text, explicit child status, and no retained
conversation state.

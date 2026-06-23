# chat.c - Chat Loop Delegator

`chat.c` provides an interactive chat loop that repeatedly reads input from stdin, delegates it to a shell command via pipe, and displays the command's output. Works as a small C library and CLI tool.

---

## CLI

### Examples

Basic delegation with single-line input:

```bash
./bin/x86_64/linux/chat cat
```

Multi-line input with end token:

```bash
./bin/x86_64/linux/chat -e END cat
```

Custom exit command:

```bash
./bin/x86_64/linux/chat -x q cat
```

Custom prompt and initial message:

```bash
./bin/x86_64/linux/chat -m 'chat ready' -p '>>> ' cat
```

---

### Parameters

| Parameter | Description |
| :--- | :--- |
| `<command>` | Shell command to execute with piped input |
| `-e <token>` | End token for multi-line input |
| `-x <text>` | Exit command (default: `exit`) |
| `-m <text>` | Initial message printed at start |
| `-p <text>` | Prompt text (default: `> `) |
| `-h`, `--help` | Show help and usage |
| `-v`, `--version` | Show version |

---

## Public API

```c
#include "chat.h"

kc_chat_t *ctx = kc_chat_open();
char *output = NULL;

kc_chat_set_cmd(ctx, "cat");
kc_chat_exec(ctx, "hello world", &output);

printf("%s\n", output);  /* prints: hello world */
kc_chat_free(output);
kc_chat_close(ctx);
```

---

## Lifecycle

- `kc_chat_open()` - allocates and returns a new context owned by the caller.
- `kc_chat_set_cmd()` - configures the shell command for delegation.
- `kc_chat_set_exit()` - configures the exit command string.
- `kc_chat_exec()` - pipes input to the configured command and captures output.
- `kc_chat_is_exit()` - checks whether input matches the exit command.
- `kc_chat_free()` - releases output strings allocated by the library.
- `kc_chat_close()` - releases the context.

---

## Build

Compiled artifacts are generated under `bin/{arch}/{platform}/` for the host architecture running the build.

```bash
make clean && make
```

### Multiarch Builds

The project is prepared to build artifacts for multiple architectures under `bin/{arch}/{platform}/`. A plain `make` builds only the current host architecture.

```bash
make all
make x86_64/linux
make x86_64/windows
make i686/linux
make i686/windows
make aarch64/linux
make aarch64/android
make armv7/linux
make armv7/android
make armv7hf/linux
make riscv64/linux
make powerpc64le/linux
make mips/linux
make mipsel/linux
make mips64el/linux
make s390x/linux
make loongarch64/linux
```

---

## Beta Notice

This is a beta project tested only on Debian x86_64. It was created out of a personal need for these libraries, but no guarantees are provided regarding its stability or future support. You are free to test it, use it, and modify it as you please.

If you'd like to reach out, you can send an email to kaisar@kaisarcode.com. Please note that I do not accept pull requests; the goal is to avoid long-term dependency on platforms like GitHub, and I do not maintain fixed infrastructure to guarantee long-term stability for these projects.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.

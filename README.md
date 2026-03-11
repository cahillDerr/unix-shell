# Unix Shell

A Unix shell implementation written in C that supports core shell functionality including piping, I/O redirection, background processes, and built-in commands.

## Tech Stack
- **Language:** C
- **Build:** Make
- **Environment:** Linux / macOS

## Features
- **Piping** — chain commands with `|` (e.g. `ls | grep foo`)
- **I/O Redirection** — input (`<`) and output (`>`) redirection
- **Background processes** — run commands with `&` without blocking the shell
- **Built-in commands** — `cd`, `exit`, and other shell builtins handled natively
- **Command parsing** — tokenizes and interprets user input correctly across all supported operators

## How to Build & Run

```bash
make all       # compile everything
make shell     # compile the shell only
make test      # compile and run all tests
make clean     # clean build artifacts
```

Then run the shell:
```bash
./shell
```

## Implementation Details

- Uses `fork()` and `execvp()` for process creation and execution
- Pipes implemented with `pipe()` and `dup2()` for file descriptor management
- Background processes tracked to avoid zombie processes
- Redirection handled by duplicating file descriptors before `exec`

# Simple Shell (LSH)

A minimal Unix-like shell written in C.

## Features

- Execute external commands (`ls`, `pwd`, `echo`, etc.)
- Built-in commands:
  - `cd`
  - `help`
  - `exit`
- Input/output redirection:
  - `>` overwrite output file
  - `>>` append output file
  - `<` read input from file
- Basic pipelines using `|`

## Project Structure

- `main.c` - shell implementation
- `lsh` - compiled binary (generated after build)

## Requirements

This project uses POSIX APIs (`fork`, `execvp`, `pipe`, `waitpid`, etc.), so build and run it in:

- Linux, or
- WSL (Windows Subsystem for Linux)

## Build

From the project directory:

```bash
gcc -std=c11 -Wall -Wextra -o lsh main.c
```

## Run

```bash
./lsh
```

You will get a prompt like:

```text
>
```

## Quick Test Commands

Run these inside your shell prompt.

### Built-ins

```text
help
cd ..
pwd
exit
```

### External commands

```text
echo hello
ls
ls -la
```

### Output redirection

```text
echo first line > out.txt
echo second line >> out.txt
cat out.txt
```

### Input redirection

```text
cat < out.txt
```

### Pipelines

```text
ls | wc -l
echo hello world | wc -w
cat out.txt | grep first
```

### Error handling

```text
notacommand
```

Expected: shell prints an error and continues.

## Notes

- If `gcc` says `fatal error: no input files`, include the source file in the command:
  - `gcc -std=c11 -Wall -Wextra -o lsh main.c`
- Running this directly in native Windows PowerShell with MinGW may fail due to missing POSIX headers. Use WSL/Linux.

## Author

Nithin J

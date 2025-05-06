# Wish Shell

A simple shell implementation written in C.

## Overview

Wish (Wisconsin Shell) is a lightweight command-line interface that provides basic shell functionality. This implementation offers essential shell capabilities with a clean, minimalist interface.

## Features

- Command execution with path searching
- Built-in commands: 
  - `cd [directory]` - Change directory
  - `exit` - Exit the shell
  - `path [directory1] [directory2] ...` - Set search path for executables
- I/O redirection with `>` operator
- Parallel command execution with `&` operator
- Support for both interactive and batch modes
- Error handling with standardized error messages
- File I/O redirection (for batch mode)

## Getting Started

### Prerequisites

- GCC or another C compiler
- Make (optional, for build automation)

### Building

To build the shell:

```bash
gcc -o wish wish.c
```

### Running

Run in interactive mode:

```bash
./wish
```

Run in batch mode (provide a batch file):

```bash
./wish batch_file
```

You can also specify an output file:

```bash
./wish batch_file output_file
```

## Usage

### Interactive Mode

When launched without arguments, the shell runs in interactive mode:
- The prompt `wish>` appears, waiting for your commands
- Enter commands like you would in any shell
- Use built-in commands (`cd`, `exit`, `path`) or any system commands

### Batch Mode

Pass a file path as an argument to run commands from that file:
- One command per line
- No prompt is displayed
- All output goes to stdout (or specified output file)

### Command Path Resolution

The shell maintains a list of directories to search for executable files:
- Default search path includes `/bin` and `/usr/bin`
- You can modify the search path using the `path` command
- `path` with no arguments clears the search path
- Examples:
  - `path /usr/local/bin /bin /usr/bin` - Sets search path to these three directories
  - `path` - Clears all search paths (you won't be able to execute any commands afterwards)

### Output Redirection

The shell supports redirecting command output to files:
- Use the `>` operator followed by a filename
- Example: `ls > output.txt` - Redirects the output of `ls` to output.txt
- Errors are properly handled:
  - No filename provided: `ls >` will produce an error
  - Multiple redirection operators: `ls > file1 > file2` will produce an error
  - Redirection at the start: `> file` will produce an error

### Parallel Command Execution

The shell supports running multiple commands in parallel:
- Use the `&` operator to separate commands
- Example: `ls & pwd & echo hello` - Runs all three commands in parallel
- The shell waits for all parallel commands to complete before accepting new input
- Parallel commands can be combined with redirection
  - Example: `ls > file1 & pwd > file2` - Redirects output of parallel commands to different files
- You can run up to 16 commands in parallel

## Code Structure

The WISH shell is implemented in `wish.c` with the following key components:

- **Main Shell Loop**: Processes input commands in `wish_shell()`
- **Command Execution**: Handles both built-in and external commands
- **Redirection Handling**: Parses and processes output redirection
- **Path Management**: Manages the search path for executable files
- **Error Handling**: Consistent error reporting throughout the shell

## Documentation

The codebase contains comprehensive comments that explain:
- Purpose and functionality of each function
- Data structures and algorithms used
- Error handling strategies
- Command execution flow
- Complex parsing logic, especially for redirection

## License

This project is open source and available under the MIT License.

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

## Limitations

- Limited error information
- No command history
- No background processing

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the MIT License.
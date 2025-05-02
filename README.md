# Wish Shell

A simple shell implementation written in C.

## Overview

Wish (Wisconsin Shell) is a lightweight command-line interface that provides basic shell functionality. This implementation offers essential shell capabilities with a clean, minimalist interface.

## Features

- Command execution
- Built-in commands: 
  - `cd` - Change directory
  - `exit` - Exit the shell
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
- Use built-in commands (`cd`, `exit`) or any system commands

### Batch Mode

Pass a file path as an argument to run commands from that file:
- One command per line
- No prompt is displayed
- All output goes to stdout (or specified output file)

## Limitations

- No support for pipes
- No wildcards or path expansion
- Limited error information
- No command history
- No background processing

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the MIT License.
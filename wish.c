#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOKENS_NUMBER 64
#define DELIM " \t\n\r"
#define ERROR_MSG "An error has occurred\n"

/**
 * Executes the built-in 'cd' (change directory) command
 * @param args Array of command arguments where args[0] is "cd" and args[1] is
 * the target directory
 * @return EXIT_SUCCESS if the command was handled, EXIT_FAILURE otherwise
 */
int execute_cd(char **args) {
  if (!strcmp(args[0], "cd")) {
    int arg_count = 0;
    // Count the number of arguments
    while (args[arg_count] != NULL) {
      arg_count++;
    }
    // Check if exactly one argument was provided (cd + directory)
    if (arg_count == 2) {
      // Attempt to change directory and report error if it fails
      if (chdir(args[1]) != 0) {
        fprintf(stderr, ERROR_MSG);
      }
    } else {
      // Wrong number of arguments for cd command
      fprintf(stderr, ERROR_MSG);
    }
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/**
 * Executes the built-in 'exit' command
 * @param args Array of command arguments where args[0] is "exit"
 * @return EXIT_SUCCESS if the command was handled, EXIT_FAILURE otherwise
 */
int execute_exit(char **args) {
  // Check if command is "exit"
  if (!strcmp(args[0], "exit")) {
    if (args[1] != NULL) {
      // Error: exit command should not have any arguments
      fprintf(stderr, ERROR_MSG);
      return EXIT_SUCCESS;
    } else {
      // Exit the shell with success status
      exit(EXIT_SUCCESS);
    }
    // This line is reached only if exit failed somehow
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/**
 * Checks and executes built-in shell commands
 * @param args Array of command arguments
 * @return EXIT_SUCCESS if a built-in command was executed, EXIT_FAILURE
 * otherwise
 */
int execute_builtin_command(char **args) {
  // Try to execute as exit command
  if (!execute_exit(args))
    return EXIT_SUCCESS;

  // Try to execute as cd command
  if (!execute_cd(args))
    return EXIT_SUCCESS;

  // Not a built-in command
  return EXIT_FAILURE;
}

/**
 * Executes a command using fork and execvp
 * @param args Array of arguments for the command
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure or exit
 */
int execute_command(char **args) {
  // Create a child process to execute the command
  // This allows the shell to continue running after command execution
  pid_t child_pid = fork();

  // Handle possible fork outcomes
  if (child_pid == -1) {
    // Fork failed - system couldn't create a new process
    fprintf(stderr, "Fork failed\n");
    return EXIT_FAILURE;
  } else if (child_pid == 0) {
    // Child process code path
    // Replace current process image with the command to be executed
    execvp(args[0], args);
    // If execvp returns, it means an error occurred (command not found or not
    // executable)
    fprintf(stderr, ERROR_MSG);
    exit(EXIT_FAILURE); // Exit child process on failure
  } else {
    // Parent process code path
    // Wait for child process to complete before returning control to shell
    wait(NULL);
    return EXIT_SUCCESS;
  }
}

/**
 * Parses a command line into an array of tokens
 * @param line The input command line to parse
 * @return Array of string tokens (needs to be freed by caller)
 */
char **parse_line(char *line) {
  // Allocate space for tokens array (maximum TOKENS_NUMBER tokens)
  char **tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
  int token_count = 0; // Tracks the number of tokens found

  // Check if memory allocation succeeded
  if (!tokens) {
    fprintf(stderr, "Memory allocation error\n");
    return NULL;
  }

  // Split the line into tokens using delimiters defined in DELIM
  char *token = strtok(line, DELIM);

  // Process all tokens in the input line
  while (token != NULL) {
    tokens[token_count] = token;
    token_count++;
    // Get next token (NULL tells strtok to continue from last position)
    token = strtok(NULL, DELIM);
  }
  // Null-terminate the array of tokens
  tokens[token_count] = NULL;
  return tokens;
}

/**
 * Main shell loop - reads and processes user commands
 * @param output Stream to write shell output to
 * @param input Stream to read shell input from
 */
void wish_shell(FILE *output, FILE *input) {
  char *line = NULL;
  size_t buffer_size = 0;
  bool running = true; // Indicates whether the shell should continue running

  while (running) {
    line = NULL;
    buffer_size = 0;

    // Print shell prompt at interactive mode only
    if (input == stdin) {
      // Display prompt only when reading from terminal
      fprintf(output, "wish> ");
    }

    // Get input line from user
    if (getline(&line, &buffer_size, input) == -1) {
      // Handle EOF or error condition
      free(line);
      break;
    }

    // Parse input line into command arguments
    char **args = parse_line(line);

    // Handle empty commands or parsing errors
    if (args == NULL || args[0] == NULL) {
      free(args);
      free(line);
      continue;
    }

    // Try to execute as a built-in command
    bool is_builtin_command = !execute_builtin_command(args);

    // If not a built-in command, try to execute as an external command
    if (!is_builtin_command)
      execute_command(args);

    // Free allocated memory
    free(args);
    free(line);
  }
}

/**
 * Entry point of the shell
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return EXIT_SUCCESS on normal termination
 */
int main(int argc, char *argv[]) {
  // Initialize input/output streams
  FILE *output = stdout;
  FILE *input = stdin;

  // Process command-line arguments for batch mode
  if (argc == 2) {
    // One argument: batch file for input
    input = fopen(argv[1], "r");
    if (input == NULL) {
      // Failed to open input file
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
  } else if (argc == 3) {
    // Two arguments: input file and output file
    input = fopen(argv[1], "r");
    if (input == NULL) {
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
    output = fopen(argv[2], "w+");
    if (output == NULL) {
      fclose(input);
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
  } else if (argc > 3) {
    // Too many arguments
    fprintf(stderr, ERROR_MSG);
    exit(EXIT_FAILURE);
  }

  // Start the shell with configured input/output
  wish_shell(output, input);

  // Clean up: close the input file if it was opened
  if (input != stdin) {
    fclose(input);
  }
  // Clean up: close the output file if it was opened
  if (output != stdout) {
    fclose(output);
  }

  return EXIT_SUCCESS;
}

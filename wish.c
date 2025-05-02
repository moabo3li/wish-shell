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

int execute_cd(char **args) {
  if (!strcmp(args[0], "cd")) {
    int position = 0;
    while (args[position] != NULL) {
      position++;
    }
    // Check the argument is valid
    if (position == 2) {
      // Change dir
      if (chdir(args[1]) != 0) {
        fprintf(stderr, ERROR_MSG);
      }
    } else {
      fprintf(stderr, ERROR_MSG);
    }
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int execute_exit(char **args) {
  // Check if argument is exit
  if (!strcmp(args[0], "exit")) {
    if (args[1] != NULL) {
      fprintf(stderr, ERROR_MSG);
      return EXIT_SUCCESS;
    } else {
      exit(EXIT_SUCCESS);
    }
    // Continue processing if we didn't exit
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int execute_builtin_command(char **args) {
  // Check if argument is exit
  if (!execute_exit(args))
    return EXIT_SUCCESS;

  // Check if argument is cd
  if (!execute_cd(args))
    return EXIT_SUCCESS;

  return EXIT_FAILURE;
}

/**
 * Executes a command using fork and execvp
 * @param args Array of arguments for the command
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure or exit
 */
int execute_command(char **args) {
  // Fork to execute the command
  pid_t fork_pid = fork();

  // Ensure that the fork did not fail
  if (fork_pid == -1) {
    fprintf(stderr, "Fork failed\n");
    return EXIT_FAILURE;
  } else if (fork_pid == 0) {
    // Execute command
    execvp(args[0], args);
    // If execvp fails, print error message
    fprintf(stderr, ERROR_MSG);
    return EXIT_FAILURE;
  } else {
    // Wait until the command finish
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
  // Allocate space for tokens array
  char **tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
  int position = 0;

  // Check if allocation succeeded
  if (!tokens) {
    fprintf(stderr, "Memory allocation error\n");
    return NULL;
  }

  // Split the line into tokens
  char *token = strtok(line, DELIM);

  while (token != NULL) {
    tokens[position] = token;
    position++;
    token = strtok(NULL, DELIM);
  }
  // Null-terminate the array of tokens
  tokens[position] = NULL;
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
  bool bsignal = true;

  while (bsignal) {
    line = NULL;
    buffer_size = 0;

    // Print shell prompt at interactive mode only
    if (input == stdin) {
      // Check if the signal is set
      fprintf(output, "wish> ");
    }

    // Get input line from user
    if (getline(&line, &buffer_size, input) == -1) {
      // Handle EOF or error condition
      free(line);
      break;
    }

    // Parse input to tokens
    char **args = parse_line(line);

    // Handle empty commands
    if (args == NULL || args[0] == NULL) {
      free(args);
      free(line);
      continue;
    }
    // Execute if builtin commands
    bool builtincommand = !execute_builtin_command(args);

    // if not found execute system commands
    if (!builtincommand)
      execute_command(args);

    // Free the allocated memory
    free(args);
    free(line);
  }
}

/**
 * Entry point of the shell
 */
int main(int argc, char *argv[]) {

  FILE *output = stdout;
  FILE *input = stdin;
  if (argc == 2)
    input = fopen(argv[1], "r");
  else if (argc == 3) {
    input = fopen(argv[1], "r");
    output = fopen(argv[2], "w+");
  }

  wish_shell(output, input);

  // Close the input file if it was opened
  if (input != stdin) {
    fclose(input);
  }
  // Close the output file if it was opened
  if (output != stdout) {
    fclose(output);
  }

  return EXIT_SUCCESS;
}

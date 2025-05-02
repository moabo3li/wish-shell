#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOKENS_NUMBER 64
#define DELIM " \t\n"

/**
 * Executes a command using fork and execvp
 * @param args Array of arguments for the command
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int execute_command(char **args) {
  // Fork to execute the command
  pid_t fork_pid = fork();

  // Ensure that the fork did not fail
  if (fork_pid == -1) {
    fprintf(stderr, "Fork failed\n");
    return EXIT_FAILURE;
  } else if (fork_pid == 0) {
    execvp(args[0], args);
    fprintf(stderr, "Execute failed\n");
    return EXIT_FAILURE;
  } else {
    wait(NULL);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
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
  
  while (true) {
    line = NULL;
    buffer_size = 0;
    
    // Print shell prompt
    fprintf(output, "wish> ");
    
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

    // Check if argument is exit
    if (!strcmp(args[0], "exit")) {
      free(args);
      free(line);
      return;
    }

    // Execute command
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

  wish_shell(output, input);

  return EXIT_SUCCESS;
}

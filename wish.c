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

char *PATH[TOKENS_NUMBER] = {NULL}; // Initialize all elements to NULL

/**
 * Initializes default path directories
 */
void initialize_path()
{
  PATH[0] = strdup("/bin");
  PATH[1] = strdup("/usr/bin");
  PATH[2] = NULL;
}

/**
 * Executes the built-in 'cd' (change directory) command
 * @param args Array of command arguments where args[0] is "cd" and args[1] is
 * the target directory
 * @return EXIT_SUCCESS if the command was handled, EXIT_FAILURE otherwise
 */
int execute_cd(char **args)
{
  if (!strcmp(args[0], "cd"))
  {
    int arg_count = 0;
    // Count the number of arguments
    while (args[arg_count] != NULL)
    {
      arg_count++;
    }
    // Check if exactly one argument was provided (cd + directory)
    if (arg_count == 2)
    {
      // Attempt to change directory and report error if it fails
      if (chdir(args[1]) != 0)
      {
        fprintf(stderr, ERROR_MSG);
      }
    }
    else
    {
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
int execute_exit(char **args)
{
  // Check if command is "exit"
  if (!strcmp(args[0], "exit"))
  {
    if (args[1] != NULL)
    {
      // Error: exit command should not have any arguments
      fprintf(stderr, ERROR_MSG);
      return EXIT_SUCCESS;
    }
    else
    {
      // Exit the shell with success status
      exit(EXIT_SUCCESS);
    }
    // This line is reached only if exit failed somehow
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/**
 * Executes the built-in 'path' command to set search directories
 * @param args Array of command arguments where args[0] is "path" followed by
 * directory paths
 * @return EXIT_SUCCESS if the command was handled, EXIT_FAILURE otherwise
 */
int execute_path(char **args)
{
  if (!strcmp(args[0], "path"))
  {
    // First, clear the existing path by setting all entries to NULL
    int path_count = 0;
    while (PATH[path_count] != NULL)
    {
      free(PATH[path_count]); // Free the previously allocated memory
      PATH[path_count] = NULL;
      path_count++;
    }

    // If there are arguments, add each one to the PATH array
    if (args[1] != NULL)
    {
      path_count = 0;
      int args_count = 1;
      while (args[args_count] != NULL && path_count < TOKENS_NUMBER - 1)
      {
        // Create a copy of the path string to avoid issues when args memory is
        // freed
        PATH[path_count] = strdup(args[args_count]);
        path_count++;
        args_count++;
      }
    }

    // Ensure the PATH array is NULL-terminated
    PATH[path_count] = NULL;
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
int execute_builtin_command(char **args)
{
  // Try to execute as exit command
  if (!execute_exit(args))
    return EXIT_SUCCESS;

  // Try to execute as cd command
  if (!execute_cd(args))
    return EXIT_SUCCESS;

  // Try to execute as path command
  if (!execute_path(args))
    return EXIT_SUCCESS;

  // Not a built-in command
  return EXIT_FAILURE;
}

/**
 * Constructs a full executable path by combining directory path with command
 * name
 * @param path Directory path to search in
 * @param command Command to execute
 * @return Newly allocated string containing the full path (caller must free)
 */
char *create_executable_path(char *path, char *command)
{
  // Allocate memory for the full path (path + / + command + null terminator)
  char *full_path = malloc(strlen(path) + strlen(command) + 2);
  if (full_path == NULL)
  {
    fprintf(stderr, ERROR_MSG);
    exit(EXIT_FAILURE);
  }
  // Construct the full path
  strcpy(full_path, path);
  strcat(full_path, "/");
  strcat(full_path, command);
  return full_path;
}

/**
 * Executes a command using fork and execv
 * @param args Array of arguments for the command
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int execute_command(char **args)
{
  // Create a child process to execute the command
  pid_t child_pid = fork();

  // Handle possible fork outcomes
  if (child_pid == -1)
  {
    // Fork failed - system couldn't create a new process
    fprintf(stderr, ERROR_MSG);
    return EXIT_FAILURE;
  }
  else if (child_pid == 0)
  {
    // Child process code path
    // Try each path in PATH array until command is found and executed
    int path_count = 0;
    char *executable_path;
    while (PATH[path_count] != NULL)
    {
      // Construct the full path for the executable
      executable_path = create_executable_path(PATH[path_count], args[0]);
      execv(executable_path, args);
      // If execv returns, the command wasn't found in this path directory
      free(executable_path);
      path_count++;
    }
    // If we reach here, command wasn't found in any path directory
    fprintf(stderr, ERROR_MSG);
    exit(EXIT_FAILURE); // Exit child process on failure
  }
  else
  {
    // Parent process code path
    // Wait for child process to complete before returning control to shell
    wait(NULL);
    return EXIT_SUCCESS;
  }
}

/**
 * Parses a command line into an array of tokens (words)
 * @param line The input command line to parse
 * @return Array of string tokens (needs to be freed by caller)
 */
char **parse_line(char *line)
{
  // Allocate space for tokens array (maximum TOKENS_NUMBER tokens)
  char **tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
  int token_count = 0; // Tracks the number of tokens found

  // Check if memory allocation succeeded
  if (!tokens)
  {
    fprintf(stderr, ERROR_MSG);
    return NULL;
  }

  // Split the line into tokens using delimiters defined in DELIM (spaces, tabs,
  // etc.)
  char *token = strtok(line, DELIM);

  // Process all tokens in the input line
  while (token != NULL && token_count < TOKENS_NUMBER - 1)
  {
    tokens[token_count] = token;
    token_count++;
    // Get next token (NULL tells strtok to continue from last position)
    token = strtok(NULL, DELIM);
  }

  // Null-terminate the array of tokens for easier processing
  tokens[token_count] = NULL;
  return tokens;
}

/**
 * Main shell loop - reads and processes user commands
 * @param output Stream to write shell output to
 * @param input Stream to read shell input from
 */
void wish_shell(FILE *output, FILE *input)
{
  char *line = NULL;
  size_t buffer_size = 0;
  bool running = true; // Shell continues running until EOF or exit command

  while (running)
  {
    line = NULL;
    buffer_size = 0;

    // Print shell prompt in interactive mode only (when input is from terminal)
    if (input == stdin)
    {
      fprintf(output, "wish> ");
      fflush(output); // Ensure prompt is displayed immediately
    }

    // Get input line from user using getline for dynamic allocation
    if (getline(&line, &buffer_size, input) == -1)
    {
      // Handle EOF (Ctrl+D) or read error by exiting the loop
      free(line);
      break;
    }

    // Parse input line into array of command arguments
    char **args = parse_line(line);

    // Skip empty commands or commands that failed to parse
    if (args == NULL || args[0] == NULL)
    {
      free(args);
      free(line);
      continue;
    }

    // First try to handle as a built-in command (cd, exit, path)
    bool is_builtin_command = !execute_builtin_command(args);

    // If not a built-in command, execute as external command
    if (!is_builtin_command)
      execute_command(args);

    // Free allocated memory to prevent leaks
    free(args);
    free(line);
  }
}

// handle input and output redirection
void handle_redirection(int argc, char **argv, FILE *input, FILE *output)
{
  // Process command-line arguments for batch mode
  if (argc == 2)
  {
    // One argument: batch file for input
    input = fopen(argv[1], "r");
    if (input == NULL)
    {
      // Failed to open input file
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
  }
  else if (argc == 3)
  {
    // Two arguments: input file and output file
    input = fopen(argv[1], "r");
    if (input == NULL)
    {
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
    output = fopen(argv[2], "w+");
    if (output == NULL)
    {
      fclose(input);
      fprintf(stderr, ERROR_MSG);
      exit(EXIT_FAILURE);
    }
  }
  else if (argc > 3)
  {
    // Too many arguments
    fprintf(stderr, ERROR_MSG);
    exit(EXIT_FAILURE);
  }
}
// Close the input/output streams if they were opened
void close_streams(FILE *input, FILE *output)
{
  if (input != stdin)
  {
    fclose(input);
  }
  if (output != stdout)
  {
    fclose(output);
  }
}

int main(int argc, char *argv[])
{
  // Initialize input/output streams
  FILE *output = stdout;
  FILE *input = stdin;

  // Initialize default path directories
  initialize_path();

  // Handle input and output redirection based on command-line arguments
  handle_redirection(argc, argv, input, output);

  // Start the shell with configured input/output
  wish_shell(output, input);

  // Close the input/output streams if they were opened
  close_streams(input, output);

  return EXIT_SUCCESS;
}

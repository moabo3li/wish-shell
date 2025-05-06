/**
 * WISH - Wisconsin Shell
 *
 * A simple Unix shell implementation with support for:
 * - Basic command execution
 * - Built-in commands: exit, cd, path
 * - I/O redirection with '>' operator
 * - Parallel command execution with '&' operator
 * - Batch mode execution from input files
 *
 * This shell searches for commands in the directories specified in the PATH array
 * and executes them in child processes. It handles errors gracefully and provides
 * appropriate error messages when commands fail.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOKENS_NUMBER 64                    // Maximum number of tokens in a command
#define MAX_PARALLEL_PROCESSES 16          // Maximum number of parallel processes
#define DELIM " \t\n\r"                     // Delimiters for tokenizing input
#define REDIRECTION_DELIM ">"               // Redirection operator
#define PARALLEL_DELIM "&"                  // Parallel command separator
#define ERROR_MSG "An error has occurred\n" // Standard error message

// Array of PATH directories where commands will be searched
char *PATH[TOKENS_NUMBER] = {NULL}; // Initialize all elements to NULL

// Global file handles for shell I/O operations
FILE *OUTPUT;    // Output stream
FILE *INPUT;     // Input stream
FILE *ERROUTPUT; // Error stream

bool SHELL_RUNNING = true;  // Controls the main shell loop execution

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
                fprintf(ERROUTPUT, ERROR_MSG);
            }
        }
        else
        {
            // Wrong number of arguments for cd command
            fprintf(ERROUTPUT, ERROR_MSG);
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
        fprintf(ERROUTPUT, ERROR_MSG);
        exit(EXIT_FAILURE);
    }
    // Construct the full path
    strcpy(full_path, path);
    strcat(full_path, "/");
    strcat(full_path, command);
    return full_path;
}

/**
 * Handles output redirection in command arguments
 * @param args Array of command arguments
 * @return EXIT_SUCCESS if redirection was handled properly, EXIT_FAILURE otherwise
 */
int handle_redirection(char **args)
{
    int current_position = 0;
    bool redirection_found = false;
    char *output_file_path = NULL;

    // Search through arguments for redirection operator
    while (args[current_position] != NULL && !redirection_found)
    {
        // Check if current argument is a redirection symbol
        if (!strcmp(args[current_position], REDIRECTION_DELIM))
        {
            // Error case: redirection at start of command (e.g., "> file")
            if (current_position == 0)
            {
                return EXIT_FAILURE;
            }
            
            // Remove the redirection symbol from arguments
            args[current_position] = NULL;
            current_position++;
            
            // Get the output file name
            if (args[current_position] != NULL)
            {
                // Store output filename for later use
                output_file_path = args[current_position];
                
                // Remove the filename from arguments
                args[current_position] = NULL;
                current_position++;
                
                // Error case: multiple redirections (e.g., "ls > file1 > file2")
                if (args[current_position] != NULL)
                {
                    args[current_position] = NULL;
                    return EXIT_FAILURE;
                }
            }
            else
            {
                // Error case: missing filename (e.g., "ls >")
                return EXIT_FAILURE;
            }
            redirection_found = true;
        }
        current_position++;
    }

    // Perform the actual redirection if an output file was specified
    if (output_file_path != NULL)
    {
        // Open the output file (create if doesn't exist, truncate if exists)
        int file_descriptor = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_descriptor == -1)
        {
            // Failed to open the output file
            fprintf(ERROUTPUT, ERROR_MSG);
            return EXIT_FAILURE;
        }
        
        // Redirect standard output to the file
        if (dup2(file_descriptor, STDOUT_FILENO) == -1)
        {
            // Failed to redirect stdout
            close(file_descriptor);
            fprintf(ERROUTPUT, ERROR_MSG);
            return EXIT_FAILURE;
        }
        
        // Close the file descriptor as it's now duplicated to stdout
        close(file_descriptor);
    }

    return EXIT_SUCCESS;
}

/**
 * Executes a command using fork and execv
 * @param args Array of arguments for the command
 * @param process_id Pointer to store the process ID (for parallel execution)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int execute_command(char **args, pid_t *process_id)
{
    // First try to handle as a built-in command (cd, exit, path)
    if (!execute_builtin_command(args))
    {
        // If it's a built-in command, execute it and return success
        // No need to track process ID for built-in commands
        return EXIT_SUCCESS;
    }

    // Create a child process to execute the external command
    pid_t child_pid = fork();

    // Handle possible fork outcomes
    if (child_pid == -1)
    {
        // Fork failed - system couldn't create a new process
        fprintf(ERROUTPUT, ERROR_MSG);
        return EXIT_FAILURE;
    }
    else if (child_pid == 0)
    {
        // Child process code path
        int path_count = 0;
        char *executable_path;
        
        // Handle any redirection in the child process 
        // (important to do this in the child so it doesn't affect the parent)
        bool redirection_success = !handle_redirection(args);

        // If redirection succeeded, search for the command in PATH directories
        while (redirection_success && PATH[path_count] != NULL)
        {
            // Construct the full path for the executable
            executable_path = create_executable_path(PATH[path_count], args[0]);

            // Try to execute the command
            execv(executable_path, args);
            
            // If execv returns, the command wasn't found in this path directory
            free(executable_path);
            path_count++;
        }

        // If we reach here, command wasn't found in any path directory
        fprintf(ERROUTPUT, ERROR_MSG);
        exit(EXIT_FAILURE); // Exit child process on failure
    }
    else
    {
        // Parent process code path
        // Save child process PID for later waitpid call in parallel execution
        *process_id = child_pid;
        return EXIT_SUCCESS;
    }
}

/**
 * Parses tokens for special delimiters and handles complex token embedding
 * @param tokens Array of initial tokens
 * @param token_count Pointer to the number of tokens in the array (will be updated)
 * @param delimiter The delimiter to look for ('>' for redirection or '&' for parallel)
 * @return Processed array of tokens with proper delimiter handling
 *
 * This function analyzes each token to detect special operators:
 * - If a token matches the delimiter exactly, it's preserved as is
 * - If a token contains the delimiter embedded (like "echo>file" or "cmd&cmd2"),
 *   it splits the token into separate parts with the delimiter in between
 */
char **parse_subtokens(char **tokens, int *token_count, char *delimiter)
{
    char **parsed_tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
    int parsed_count = 0; // tracks the number of tokens found

    char *subtoken;

    for (int i = 0; i < *token_count; i++)
    {
        // Check if the token is exactly the delimiter
        if (!strcmp(tokens[i], delimiter))
        {
            parsed_tokens[parsed_count++] = tokens[i];
            parsed_tokens[parsed_count] = NULL;
            continue;
        }
        
        // Make a copy to compare with after tokenization
        char *token_copy = malloc(strlen(tokens[i]) + 1);
        strcpy(token_copy, tokens[i]);

        // Try to split by the delimiter
        subtoken = strtok(tokens[i], delimiter);

        // If token wasn't split, keep it as is
        if (!strcmp(subtoken, token_copy))
        {
            parsed_tokens[parsed_count++] = tokens[i];
            parsed_tokens[parsed_count] = NULL;
            free(token_copy);
            continue;
        }

        free(token_copy);

        // Handle the split token parts
        do
        {
            parsed_tokens[parsed_count++] = subtoken;
            parsed_tokens[parsed_count++] = delimiter; // Insert delimiter as a separate token
            subtoken = strtok(NULL, delimiter); // Get next part
        } while (subtoken != NULL);
        
        // Remove the last NULL that was assigned
        parsed_tokens[--parsed_count] = NULL;
    }
    
    // Update the token count to reflect the new total
    *token_count = parsed_count;
    return parsed_tokens;
}

/**
 * Parses a command line into an array of tokens (words)
 * @param line The input command line to parse
 * @return Array of string tokens (needs to be freed by caller)
 */
char **parse_line(char *line)
{
    // Allocate space for tokens array (maximum TOKENS_NUMBER tokens)
    char **initial_tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
    int token_count = 0; // Tracks the number of tokens found

    // Check if memory allocation succeeded
    if (!initial_tokens)
    {
        fprintf(ERROUTPUT, ERROR_MSG);
        return NULL;
    }

    // Split the line into tokens using basic whitespace delimiters
    char *token = strtok(line, DELIM);
    while (token != NULL && token_count < TOKENS_NUMBER - 1)
    {
        initial_tokens[token_count++] = token;
        token = strtok(NULL, DELIM);
    }

    // Null-terminate the array of tokens for easier processing
    initial_tokens[token_count] = NULL;

    // Process the special operators in two steps:
    
    // Step 1: Parse and handle redirection operator ('>')
    char **redirection_parsed = parse_subtokens(initial_tokens, &token_count, REDIRECTION_DELIM);
    
    // Step 2: Parse and handle parallel execution operator ('&')
    char **final_tokens = parse_subtokens(redirection_parsed, &token_count, PARALLEL_DELIM);
    
    // Free the intermediate array
    free(initial_tokens);
    
    return final_tokens;
}

/**
 * Main shell loop - reads and processes user commands
 * @param output Stream to write shell output to
 * @param input Stream to read shell input from
 */
void wish_shell()
{
    char *line = NULL;
    size_t buffer_size = 0;

    while (SHELL_RUNNING)
    {
        line = NULL;
        buffer_size = 0;

        // Print shell prompt in interactive mode only (when input is from terminal)
        if (INPUT == stdin)
        {
            fprintf(OUTPUT, "wish> ");
            fflush(OUTPUT); // Ensure prompt is displayed immediately
        }

        // Get input line from user using getline for dynamic allocation
        if (getline(&line, &buffer_size, INPUT) == -1)
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

        // Arrays and counters for parallel command execution
        pid_t parallel_processes[MAX_PARALLEL_PROCESSES]; // Array to store process IDs
        int process_count = 0;                            // Counter for parallel processes
        int arg_position = 0;                             // Current position in args array
        int command_arg_count = 0;                        // Counter for current command's arguments
        
        // Temporary array to hold the current command to be executed
        char **current_command = malloc(sizeof(char *) * TOKENS_NUMBER);

        // Process all arguments, creating commands separated by PARALLEL_DELIM ('&')
        while (args[arg_position] != NULL)
        {
            // Copy current argument to the command array
            current_command[command_arg_count] = malloc(strlen(args[arg_position]) + 1);
            strcpy(current_command[command_arg_count], args[arg_position]);
            current_command[command_arg_count + 1] = NULL;
            
            // Check if the current argument is a parallel delimiter ('&')
            if (!strcmp(args[arg_position], PARALLEL_DELIM))
            {
                // Handle empty command before delimiter
                if (!command_arg_count)
                {
                    break;
                }
                
                // Null-terminate the current command
                current_command[command_arg_count] = NULL;
                
                // Execute the command and store its process ID
                execute_command(current_command, &parallel_processes[process_count++]);
                
                // Free the memory for the delimiter token
                free(current_command[command_arg_count]);
                
                // Reset command argument counter to prepare for next command
                command_arg_count = -1;
            }
            
            // Move to the next argument
            arg_position++;
            command_arg_count++;
        }
        
        // Execute the last command if there are any pending arguments
        if (command_arg_count)
        {
            execute_command(current_command, &parallel_processes[process_count++]);
        }

        // Free the allocated memory for current_command
        for (int i = 0; i < command_arg_count; i++)
        {
            free(current_command[i]);
        }
        free(current_command);

        // Wait for all processes to complete
        int status;
        for (int i = 0; i < process_count; ++i)
        {
            // Wait for each child process to complete
            waitpid(parallel_processes[i], &status, 0);
        }
        
        // Free allocated memory to prevent leaks
        free(args);
        free(line);
    }
}

/**
 * Configures shell input and output redirection based on command-line arguments
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 *
 * This function sets up the shell's I/O streams based on command-line arguments:
 * - No arguments: Use standard input/output (interactive mode)
 * - One argument: Use specified file as input (batch mode)
 * - Two arguments: Use first file as input, second file as output
 */
void handle_shell_redirection(int argc, char **argv)
{
    // Initialize input/output streams
    INPUT = stdin;
    OUTPUT = stdout;
    ERROUTPUT = stderr;
    // Process command-line arguments for batch mode
    if (argc == 2)
    {
        // One argument: batch file for input
        INPUT = fopen(argv[1], "r");
        if (INPUT == NULL)
        {
            // Failed to open input file
            fprintf(ERROUTPUT, ERROR_MSG);
            exit(EXIT_FAILURE);
        }
    }
    else if (argc == 3)
    {
        // Two arguments: input file and output file
        INPUT = fopen(argv[1], "r");
        if (INPUT == NULL)
        {
            fprintf(ERROUTPUT, ERROR_MSG);
            exit(EXIT_FAILURE);
        }
        OUTPUT = fopen(argv[2], "w+");
        if (OUTPUT == NULL)
        {
            fclose(INPUT);
            fprintf(ERROUTPUT, ERROR_MSG);
            exit(EXIT_FAILURE);
        }
    }
    else if (argc > 3)
    {
        // Too many arguments
        fprintf(ERROUTPUT, ERROR_MSG);
        exit(EXIT_FAILURE);
    }
}

/**
 * Closes any opened file streams before program termination
 * This function ensures proper cleanup of file resources
 */
void close_streams()
{
    if (INPUT != stdin)
        fclose(INPUT);

    if (OUTPUT != stdout)
        fclose(OUTPUT);
    if (ERROUTPUT != stderr)
        fclose(ERROUTPUT);
}

int main(int argc, char *argv[])
{
    // Handle input and output redirection based on command-line arguments
    handle_shell_redirection(argc, argv);

    // Initialize default path directories
    initialize_path();

    // Start the shell with configured input/output
    wish_shell();

    // Close the input/output streams if they were opened
    close_streams();

    return EXIT_SUCCESS;
}

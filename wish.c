/**
 * WISH - Wisconsin Shell
 * 
 * A simple Unix shell implementation with support for:
 * - Basic command execution
 * - Built-in commands: exit, cd, path
 * - I/O redirection with '>' operator
 * - Batch mode execution from input files
 * 
 * This shell searches for commands in the directories specified in the PATH array
 * and executes them in child processes. It handles errors gracefully and provides
 * appropriate error messages when commands fail.
 */

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOKENS_NUMBER 64       // Maximum number of tokens in a command
#define DELIM " \t\n\r"        // Delimiters for tokenizing input
#define REDIRECTION_DELIM ">"  // Redirection operator
#define ERROR_MSG "An error has occurred\n"  // Standard error message

// Array of PATH directories where commands will be searched
char *PATH[TOKENS_NUMBER] = {NULL}; // Initialize all elements to NULL

// Global file handles for shell I/O operations
FILE *OUTPUT;    // Standard output stream
FILE *INPUT;     // Standard input stream
FILE *ERROUTPUT; // Standard error stream

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
    int redirection_position = 0;
    bool found = false;

    while (args[redirection_position] != NULL && !found)
    {
        if (!strcmp(args[redirection_position], REDIRECTION_DELIM))
        {
            if (redirection_position == 0)
            {
                return EXIT_FAILURE;
            }
            args[redirection_position] = NULL;
            redirection_position++;
            if (args[redirection_position] != NULL)
            {
                OUTPUT = fopen(args[redirection_position], "w+");
                args[redirection_position] = NULL;
                redirection_position++;
                if (args[redirection_position] != NULL)
                {
                    args[redirection_position] = NULL;
                    return EXIT_FAILURE;
                }
            }
            else
            {
                return EXIT_FAILURE;
            }
            found = true;
        }
        redirection_position++;
    }
    return EXIT_SUCCESS;
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
        fprintf(ERROUTPUT, ERROR_MSG);
        return EXIT_FAILURE;
    }
    else if (child_pid == 0)
    {
        // Child process code path
        // Try each path in PATH array until command is found and executed
        int path_count = 0;
        char *executable_path;
        bool is_handled = !handle_redirection(args);
        while (is_handled && PATH[path_count] != NULL)
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
        // Wait for child process to complete before returning control to shell
        wait(NULL);
        return EXIT_SUCCESS;
    }
}

/**
 * Parses tokens for redirection symbols and handles complex redirection cases
 * @param tmptokens Array of initial tokens
 * @param tmptoken_count Number of tokens in the array
 * @return Processed array of tokens with proper redirection handling
 *
 * This function analyzes each token to detect redirection operators:
 * - If a token is exactly ">" (REDIRECTION_DELIM), it's preserved as is
 * - If a token contains ">" embedded within (like "echo>file"), it splits the token
 *   into separate tokens: "echo", ">", "file"
 * The function returns a new array with all tokens properly separated
 */
char **parse_redirection(char **tmptokens, int tmptoken_count)
{
    char **tokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
    int token_count = 0; // tracks the number of tokens found

    char *subtoken;

    for (int i = 0; i < tmptoken_count; i++)
    {
        if (!strcmp(tmptokens[i], REDIRECTION_DELIM))
        {
            tokens[token_count++] = tmptokens[i];
            tokens[token_count] = NULL;
            continue;
        }
        char *cmptoken = malloc(strlen(tmptokens[i]) + 1);
        strcpy(cmptoken, tmptokens[i]);

        subtoken = strtok(tmptokens[i], REDIRECTION_DELIM);

        if (!strcmp(subtoken, cmptoken))
        {
            tokens[token_count++] = tmptokens[i];
            tokens[token_count] = NULL;
            free(cmptoken);
            continue;
        }

        free(cmptoken);

        do
        {
            tokens[token_count++] = subtoken;
            // Add >
            tokens[token_count++] = REDIRECTION_DELIM;
            // Get next token (NULL tells strtok to continue from last position)
            subtoken = strtok(NULL, REDIRECTION_DELIM);
        } while (subtoken != NULL);
        tokens[--token_count] = NULL;
    }
    return tokens;
}

/**
 * Parses a command line into an array of tokens (words)
 * @param line The input command line to parse
 * @return Array of string tokens (needs to be freed by caller)
 */
char **parse_line(char *line)
{
    // Allocate space for tokens array (maximum TOKENS_NUMBER tokens)
    char **tmptokens = malloc(TOKENS_NUMBER * (sizeof(char *)));
    int tmptoken_count = 0; // Tracks the number of tokens found

    // Check if memory allocation succeeded
    if (!tmptokens)
    {
        fprintf(ERROUTPUT, ERROR_MSG);
        return NULL;
    }

    // Split the line into tokens using delimiters defined in DELIM (spaces, tabs,
    // etc.)
    char *token = strtok(line, DELIM);
    // Process all tokens in the input line
    while (token != NULL && tmptoken_count < TOKENS_NUMBER - 1)
    {
        tmptokens[tmptoken_count++] = token;
        // Get next token (NULL tells strtok to continue from last position)
        token = strtok(NULL, DELIM);
    }

    // Null-terminate the array of tokens for easier processing
    tmptokens[tmptoken_count] = NULL;
    // Parse and handle the redirection
    char **tokens = parse_redirection(tmptokens, tmptoken_count);
    free(tmptokens);
    return tokens;
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
    bool running = true; // Shell continues running until EOF or exit command

    while (running)
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

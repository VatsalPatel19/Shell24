#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 5
#define MAX_COMMAND_LENGTH 100
#define MAX_PIPES 6

pid_t background_pid = -1;
// Global variable to track background process IDs
pid_t background_processes[MAX_PIPES];

void execute_command(char **args, int argc) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        wait(NULL);
    }
}

void handle_newt_command() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        execlp("xterm", "xterm", "-e", "./shell24", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }
}

void concatenate_files(char *file_names[], int num_files) {
    for (int i = 0; i < num_files; ++i) {
        FILE *fp = fopen(file_names[i], "r");
        if (fp == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        int c;
        while ((c = fgetc(fp)) != EOF) {
            putchar(c);
        }
        fclose(fp);
    }
}

void execute_piped_commands(char *commands[], int num_pipes) {
    int pipefds[num_pipes - 1][2]; // File descriptors for pipes

    // Create pipes
    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipefds[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    int current_pipe = 0; // Index for current pipe

    // Iterate through each command in the pipeline
    for (int i = 0; i < num_pipes; i++) {
        char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

        // Tokenize the command
        char *token = strtok(commands[i], " ");
        int argc = 0;

        // Store command arguments
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Terminate the argument list

        // Fork a child process
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            // Redirect input for all commands except the first one
            if (i > 0) {
                if (dup2(pipefds[current_pipe - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output for all commands except the last one
            if (i < num_pipes - 1) {
                if (dup2(pipefds[current_pipe][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close pipe descriptors
            for (int j = 0; j < num_pipes - 1; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent process
            // Close write end of the pipe after forking
            if (i < num_pipes - 1) {
                close(pipefds[current_pipe][1]);
            }
            current_pipe++;
        }
    }

    // Close all pipe descriptors
    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_pipes; i++) {
        wait(NULL);
    }
}

void execute_piped_commands_wrapper(char *commands[], int num_pipes) {
    execute_piped_commands(commands, num_pipes);
}

void execute_command_wrapper(char *command) {
    char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

    // Tokenize the command
    char *token = strtok(command, " ");
    int argc = 0;

    // Store command arguments
    while (token != NULL && argc < MAX_ARGS) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL; // Terminate the argument list

    // Execute the command
    execute_command(args, argc);
}

void concatenate_files_wrapper(char *commands[], int num_commands) {
    char *file_names[MAX_ARGS];
    int num_files = 0;

    // Parse commands and file names
    for (int i = 0; i < num_commands; i++) {
        char *token = strtok(commands[i], " ");
        while (token != NULL) {
            if (strcmp(token, "#") == 0) {
                // Start concatenation mode
                token = strtok(NULL, " ");
                continue;
            }
            file_names[num_files++] = token;
            token = strtok(NULL, " ");
        }
    }

    // Call function to concatenate files
    concatenate_files(file_names, num_files);
}

void execute_conditional_commands(char *commands[], int num_commands) {
    int status;
    pid_t pid;

    for (int i = 0; i < num_commands; i++) {
        int argc = 0;
        char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

        // Tokenize the command
        char *token = strtok(commands[i], " ");
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Terminate the argument list

        // Execute the command
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent process
            waitpid(pid, &status, 0);

            // If the command fails, break the loop
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                break;
            }
        }
    }
}

void handle_background_processing(char *command) {
    // Execute the command in the background
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

        // Tokenize the command
        char *token = strtok(command, " ");
        int argc = 0;

        // Store command arguments
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Terminate the argument list

        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        background_pid = pid; // Update global variable with the background process PID
        printf("[%d] %s\n", pid, command); // Print the background process ID
    }
}

void handle_foreground_processing() {
    if (background_pid != -1) {
        // Bring the last background process into the foreground
        int status;
        if (waitpid(background_pid, &status, WUNTRACED) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        background_pid = -1; // Reset the background process ID
    } else {
        printf("No background process to bring into foreground.\n");
    }
}

void execute_sequential_commands(char *commands[], int num_commands) {
    for (int i = 0; i < num_commands; i++) {
        char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

        // Tokenize the command
        char *token = strtok(commands[i], " ");
        int argc = 0;

        // Store command arguments
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Terminate the argument list

        // Execute the command
        execute_command(args, argc);
    }
}

void handle_redirection(char *command) {
    char *args[MAX_ARGS + 1]; // Additional space for NULL terminator

    // Tokenize the command
    char *token = strtok(command, " ");
    int argc = 0;
    int fd_in = STDIN_FILENO;
    int fd_out = STDOUT_FILENO;
    int append = 0;
    char *filename = NULL;

    // Store command arguments
    while (token != NULL && argc < MAX_ARGS) {
        if (strcmp(token, "<") == 0) {
            // Redirect input
            token = strtok(NULL, " ");
            filename = token;
            fd_in = open(filename, O_RDONLY);
            if (fd_in == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(token, ">") == 0) {
            // Redirect output (truncate)
            token = strtok(NULL, " ");
            filename = token;
            fd_out = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_out == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(token, ">>") == 0) {
            // Redirect output (append)
            token = strtok(NULL, " ");
            filename = token;
            fd_out = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (fd_out == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            append = 1;
        } else {
            args[argc++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[argc] = NULL; // Terminate the argument list

    // Perform redirection
    if (fd_in != STDIN_FILENO) {
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO) {
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }

    // Execute the command
    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
}

// Function to handle SIGCHLD signal
void sigchld_handler(int signum) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Remove the finished process from the list of background processes
        for (int i = 0; i < MAX_PIPES; i++) {
            if (background_processes[i] == pid) {
                background_processes[i] = 0;
                break;
            }
        }
    }
}

int main() {
    char command[MAX_COMMAND_LENGTH];
    char *commands[MAX_PIPES]; // For piped commands

    while (1) {
        printf("shell24$ ");
        fflush(stdout); // Ensure the prompt is displayed

        fgets(command, sizeof(command), stdin);

        // Remove trailing newline character
        command[strcspn(command, "\n")] = '\0';

        pid_t pid = fork(); // Fork a child process

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            int num_pipes = 0; // Number of pipes detected

            if (strcmp(command, "newt") == 0) {
                handle_newt_command();
            } else {
                char *token = strtok(command, "|");
                int command_index = 0;

                // Tokenize the command based on pipe symbol "|"
                while (token != NULL && command_index < MAX_PIPES) {
                    commands[command_index++] = token;
                    token = strtok(NULL, "|");
                }

                num_pipes = command_index;

                // Check if '#' or '&&' is present in any of the commands
                int concatenate_mode = 0;
                int conditional_mode = 0;

                for (int i = 0; i < num_pipes; i++) {
                    if (strstr(commands[i], "#") != NULL) {
                        concatenate_mode = 1;
                    }
                    if (strstr(commands[i], "&&") != NULL) {
                        conditional_mode = 1;
                    }
                }

                // Check if '&' is present in any of the commands
                int background_mode = 0;

                for (int i = 0; i < num_pipes; i++) {
                    if (strstr(commands[i], "&") != NULL) {
                        background_mode++;
                    }
                }

                // Check if ';' is present in any of the commands
                int sequential_mode = 0;

                for (int i = 0; i < num_pipes; i++) {
                    if (strstr(commands[i], ";") != NULL) {
                        sequential_mode++;
                    }
                }

                if (conditional_mode) {
                    // Call function to execute conditional commands
                    char *conditionals[MAX_PIPES]; // For conditional commands
                    int conditional_index = 0;

                    // Tokenize the command based on "&&"
                    token = strtok(command, "&&");
                    while (token != NULL && conditional_index < MAX_PIPES) {
                        conditionals[conditional_index++] = token;
                        token = strtok(NULL, "&&");
                    }

                    // Execute the conditional commands
                    execute_conditional_commands(conditionals, conditional_index);
                } else if (concatenate_mode) {
                    // Call function to concatenate files
                    concatenate_files_wrapper(commands, num_pipes);
                } else if (background_mode) {
                    // Call function to execute in background
                    handle_background_processing(commands[0]);
                } else if (sequential_mode) {
                    // Tokenize the command based on semicolon ";"
                    token = strtok(command, ";");
                    int command_index = 0;

                    // Tokenize the command based on semicolon ";"
                    while (token != NULL && command_index < MAX_PIPES) {
                        commands[command_index++] = token;
                        token = strtok(NULL, ";");
                    }

                    int num_commands = command_index;

                    // Execute sequential commands
                    execute_sequential_commands(commands, num_commands);
                } else if (strcmp(command, "fg") == 0) {
                    // Bring the last background process into the foreground
                    handle_foreground_processing();
                } else if (num_pipes > 1) {
                    // Execute piped commands if there are any
                    execute_piped_commands_wrapper(commands, num_pipes);
                } else {
                    // Handle redirection and execute the command
                    handle_redirection(command);
                }
            }

            exit(EXIT_SUCCESS); // Terminate child process
        } else {
            // Parent process
            wait(NULL); // Wait for child process to finish
        }
    }

    return 0;
}

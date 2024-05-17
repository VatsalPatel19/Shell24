Shell24: A Modular C Shell Program
shell24 is a C program that functions as a shell, accepting user commands and executing them using fork(), exec(), and other relevant system calls. The program operates in an infinite loop, awaiting user input and processing it according to specified rules and conditions.

Rules and Conditions
Creation of New Shells: The newt command (shell24$newt) must create a new copy of shell24, without any upper limit on the number of new terminal sessions.
Command Argument Count (argc): The argc of any command or program should be between 1 and 5, inclusive.
Special Characters Handling:
#: Text file concatenation (up to 5 operations).
|: Piping (up to 6 operations).
>, <, >>: Redirection.
&&, ||: Conditional execution (up to 5 operators, possibly combined).
&: Background processing.
;: Sequential execution of commands (up to 5 commands).
Implementation Notes
Modular Approach: Utilize modular programming for better code organization.
Error Handling: Display appropriate error messages based on specifications.
Comments: Include comments throughout the program to explain the code's functionality.

Example Usage
shell24$ date
shell24$ ls -1 -l -t ~/chapter5/dir1
shell24$ cat input1.txt input2.txt
shell24$ ls -l -t | wc
shell24$ ls | grep *.c | wc | wc -w
shell24$ cat new.txt >> sample.txt
shell24$ ex1 && ex2 && ex3 && ex4
shell24$ c1 && c2 || c3 && c4
shell24$ ex1 &
shell24$ fg
shell24$ ls -l -t ; date ; ex1 ;
Notes
You are not required to combine special characters (e.g., $, &, >>) in a single command.
Implement fork() and exec() along with other relevant system calls for command execution.

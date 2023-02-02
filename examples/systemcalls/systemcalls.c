#include "systemcalls.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
	bool bSucceed = false;
/*
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

	if (cmd != NULL)
	{
		char *LocalCmd = NULL;
		if (cmd[0] == '-')
		{
			// See https://man7.org/linux/man-pages/man3/system.3.html#BUGS
			int iCmdLen = strlen(cmd);
			LocalCmd = malloc(iCmdLen + 2);
			strcpy(&LocalCmd[1], cmd); 
			LocalCmd[0] = ' ';
		}

		int iRetVal = system(LocalCmd != NULL ? LocalCmd : cmd);

		if (iRetVal == 0)
		{
			bSucceed = true;	
		}

		if (LocalCmd != NULL)
		{
			free(LocalCmd);
			LocalCmd = NULL;
		}

	}

    return bSucceed;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
	bool bSucceed = false;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

	if (command[0] != NULL)
	{
		pid_t ChildId = fork();

		if (ChildId >= 0) 
		{
			if (ChildId == 0)
			{
				execv(command[0], command);
				exit(1);
			}
			else
			{
				int iStatus = 0;
				if (wait(&iStatus) != ChildId);
				{
					bSucceed = WIFEXITED(iStatus) && (WEXITSTATUS(iStatus) == 0);
				}
			}
		}
	}

    va_end(args);

    return bSucceed;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
	bool bSucceed = false;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

	if (command[0] != NULL)
	{
		pid_t ChildId = fork();

		if (ChildId >= 0) 
		{
			if (ChildId == 0)
			{

				if (outputfile != NULL)
				{
					int iRedirect = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
					if (iRedirect != -1)
					{
						dup2(iRedirect, STDOUT_FILENO);
						close(iRedirect);
					}
				}
				

				execv(command[0], command);
				exit(1);
			}
			else
			{
				int iStatus = 0;
				if (wait(&iStatus) == ChildId)
				{
					bSucceed = WIFEXITED(iStatus) && (WEXITSTATUS(iStatus) == 0);
				}
			}
		}
	}

    va_end(args);

    return bSucceed;
}

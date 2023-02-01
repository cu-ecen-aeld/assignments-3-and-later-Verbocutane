#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int iOutputFile;

	openlog ("writer", LOG_CONS | LOG_PID, LOG_USER);


	if (argc != 3)
	{
		syslog (LOG_ERR, "Invalid number of argument");
		closelog();
		exit(1);
	}

	iOutputFile = open (argv[1], O_WRONLY | O_CREAT, 0666);

	if (iOutputFile == -1)
	{
		syslog (LOG_ERR, "Error creating output file \"%s\" : %s", argv[1], strerror(errno));
		closelog();
		exit(1);
	}

	syslog (LOG_DEBUG, "Writing \"%s\" to \"%s\"", argv[2], argv[1]);
 
	if (write(iOutputFile, argv[2], strlen(argv[2])) == -1)
	{
		syslog (LOG_ERR, "Error writing to output file \"%s\" : %s", argv[1], strerror(errno));
		closelog();
		exit(1);
	}

	return 0;
}



#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>


int iOutputFile = 0;
int ListenSocket = 0;
int peersock = 0;
char *Alocated = NULL;
bool bSigRec = false;

void CloseAll(bool bExit);
void termination_handler (int signum);

int main(int argc, char *argv[])
{
	struct addrinfo *result = NULL, hints;
	struct sigaction new_action;
	char PortNumber[8] = "";

	if (argc >= 2)
	{
		if ((argc == 2) && (argv[1][0] == '-') && (argv[1][1] == 'd') && (argv[1][2] == '\0'))
		{
			pid_t Pid = fork();
			if (Pid > 0)
			{
				printf ("Daemon started (PID %d)\n", Pid);
				exit(EXIT_SUCCESS);
			}
			else if (Pid == -1)
			{
				printf ("Error in \"fork\" : %s\n", strerror(errno));
				CloseAll(true);
			}
		}
		else
		{
			printf ("Invalid argument\n");
			CloseAll(true);
		}
	}

	openlog ("aesdsocket", LOG_CONS | LOG_PID, LOG_USER);

	iOutputFile = open ("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_TRUNC, 0666);

	if (iOutputFile == -1)
	{
		syslog (LOG_ERR, "Error creating output file \"/var/tmp/aesdsocketdata\" : %s", strerror(errno));
		CloseAll(true);
	}

	memset(&new_action, 0, sizeof(new_action));
	new_action.sa_handler = termination_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction (SIGINT, &new_action, NULL);
	sigaction (SIGTERM, &new_action, NULL);


	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	snprintf(PortNumber, sizeof(PortNumber), "%d", 9000);

	int iResult = getaddrinfo(NULL, PortNumber, &hints, &result);
	if (iResult == 0)
	{
		// Create a SOCKET for the server to listen for client connections
		int ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket != -1)
		{
			const int enable = 1;
			if (setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int)) < 0)
				syslog (LOG_ERR, "Error in \"setsockopt\" : %s", strerror(errno));


			// Setup the TCP listening socket
			if (bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen) != -1)
			{
				freeaddrinfo(result);
				result = NULL;

				if ( listen( ListenSocket, SOMAXCONN ) != -1 )
				{
					int iResult, AlocatedSize = 0;
					struct sockaddr_in addr;
					char NetBuffer[512], FileBuffer[512];
					socklen_t len;


					len = sizeof(addr);
					do
					{
						peersock = accept(ListenSocket, (struct sockaddr *)&addr, &len);
						if (peersock < 0)
						{
							syslog (LOG_ERR, "Error in \"listen\" : %s", strerror(errno));
							CloseAll(true);
						}

						syslog (LOG_INFO, "Accepted connection from %s", inet_ntoa(addr.sin_addr));

						do
						{
							iResult = recv(peersock, NetBuffer, sizeof(NetBuffer)-1, 0);
							if (iResult > 0)
							{
								int RemainingStringSize;
								char *EndLine, *StartLine = NetBuffer;
								NetBuffer[iResult] = '\0';

								do
								{

									EndLine = strchr(StartLine, '\n');
									if (EndLine != NULL)
									{
										if (Alocated != NULL)
										{
											if (write(iOutputFile, Alocated, AlocatedSize) == -1)
											{
												syslog (LOG_ERR, "Error in \"write\" : %s", strerror(errno));
												CloseAll(true);
											}
											free(Alocated);
											Alocated = NULL;
											AlocatedSize = 0;
										}


										if (write(iOutputFile, StartLine, (uintptr_t)EndLine - (uintptr_t)StartLine + 1) == -1)
										{
											syslog (LOG_ERR, "Error in \"write\" : %s", strerror(errno));
											CloseAll(true);
										}

										StartLine = EndLine + 1;



										lseek(iOutputFile, 0, SEEK_SET);
										do
										{
											iResult = read(iOutputFile, FileBuffer, sizeof(FileBuffer));
											if (iResult < 0)
											{
												syslog (LOG_ERR, "Error in \"read\" : %s", strerror(errno));
												CloseAll(true);
											}
											else if (iResult > 0)
											{
												iResult = send(peersock, FileBuffer, iResult, MSG_NOSIGNAL);
												if (iResult < 0)
												{
													syslog (LOG_ERR, "Error in \"send\" : %s", strerror(errno));
													CloseAll(true);
												}
											}
										}while(iResult > 0);
									}
								}while(EndLine != NULL);

								RemainingStringSize = strlen(StartLine);
								if (RemainingStringSize > 0)
								{
									char *CopyDest;
									if (Alocated != NULL)
									{
										Alocated = realloc(Alocated, AlocatedSize + RemainingStringSize);
										CopyDest = Alocated + AlocatedSize;
										AlocatedSize += RemainingStringSize;
									}
									else
									{
										Alocated = malloc(RemainingStringSize);
										CopyDest = Alocated;
										AlocatedSize = RemainingStringSize;
									}

									memcpy(CopyDest, StartLine, RemainingStringSize);
								}
							}
							else if (iResult == -1)
							{
								syslog (LOG_ERR, "Error in \"recv\" : %s", strerror(errno));
								CloseAll(true);
							}
							else
							{
								syslog (LOG_INFO, "Closed connection from %s", inet_ntoa(addr.sin_addr));
								break;
							}

						}while(true);
					}while(!bSigRec);
				}
				else
				{
					syslog (LOG_ERR, "Error in \"listen\" : %s", strerror(errno));
					CloseAll(true);
				}
			}
			else
			{
				syslog (LOG_ERR, "Error in \"bind\" : %s", strerror(errno));
				CloseAll(true);
			}
		}
		else
		{
			syslog (LOG_ERR, "Error in \"socket\" : %s", strerror(errno));
			CloseAll(true);
		}
	}
	else
	{
		syslog (LOG_ERR, "Error in \"getaddrinfo\" : %s", strerror(errno));
		CloseAll(true);
	}

	syslog (LOG_INFO, "Program exit");
	CloseAll(false);

	return 0;
}


void CloseAll(bool bExit)
{
	if (iOutputFile != 0)
	{
		close(iOutputFile);
	}

	if (remove("/var/tmp/aesdsocketdata") == -1)
		syslog (LOG_ERR, "Error in \"remove\" : %s", strerror(errno));


	closelog();
	if (Alocated != NULL)
	{
		free(Alocated);
		Alocated = NULL;
	}
	if (peersock != 0)
	{
		close(peersock);
	}
	if (ListenSocket != 0)
	{
		close(ListenSocket);
	}

	if (bExit)
		exit(bSigRec ? 0 : -1);
}


void termination_handler (int iSig)
{
	bSigRec = true;
}




#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

int max_fd = 0, g_id = 0; 
int id[65536], currmsg[65536], sock_fd = -1;

fd_set fdread, fdwrite, currentsock;
char buf[4096 * 42], msg[4096 * 42 + 42], str[4096 * 42];

// ==> Send the message stock on fdwrite by sprintf, to every client except one
void sendAll(int except) 
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &fdwrite) && except != fd)
			send(fd, msg, strlen(msg), 0);
}

// ==> Classic fatal Error
void fatal() 
{
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	close(sock_fd);
	exit(1);
}

int main(int argc, char **argv)
{
	// ========== ESTABLISHING CONNEXION 📡 =========== //

	// ==> Wrong number of arguments check
	if(argc != 2)
	{
		write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
		exit(1);
	}

	// ==> Create every server details (port, IP)
	uint16_t port = atoi(argv[1]);
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(2130706433);
	serveraddr.sin_port = htons(port);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // find the socket server
		fatal();
	if (bind(sock_fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) // bind the socket to the good port with sockaddr struct
		fatal();
	if (listen(sock_fd, 128) == -1) // listen to server socket
		fatal();
	
	// set to zero everything we need
	bzero(id, sizeof(id));
	FD_ZERO(&currentsock);
	FD_SET(sock_fd, &currentsock);
	max_fd = sock_fd;

	// ========== CONNEXION SET ✅ =========== //

	// ========== TREAT ENTRANCE MESSAGE =========== //
	while(1)
	{
		fdwrite = fdread = currentsock;
		if (select(max_fd + 1, &fdread, &fdwrite, 0, 0) <= 0)
			continue;
		// for all clients
		for (int fd = 0; fd <= max_fd; fd++) 
		{
			// if the client is ready to be read
			if (FD_ISSET(fd, &fdread))
			{
				// if the actuel client (ready to be read) is the server one (means a new connexion)
				if (fd == sock_fd)
				{
					// create a struct to stock new client
					struct sockaddr_in clientaddr;
					socklen_t len = sizeof(clientaddr);

					// accept the client
					int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
					if (clientfd == -1)
						continue;
					
					// put the client into active_sockets
					FD_SET(clientfd, &currentsock);
					id[clientfd] = g_id++;
					currmsg[clientfd] = 0;
					if(max_fd < clientfd)
					max_fd = clientfd;

					// sending message to all clients
					sprintf(msg, "server: client %d just arrived\n", id[clientfd]);
					sendAll(fd);
					break;
				}
				// else the sock is ready to be read but it's not the server one (means the client is left or send a message to treat)
				else
				{
					// catch the message of the socket
					int ret = recv(fd, buf, 4096 * 42, 0);

					// if the lenght of message is <= than 0, it's mean the client left
					if (ret <= 0)
					{
						sprintf(msg, "server: client %d just left\n", id[fd]);
						sendAll(fd);

						// clear this client from active_socket & close the client socket.
						FD_CLR(fd, &currentsock);
						close(fd);
						break;
					}
					// else there is a message to treat
					else
					{
						// for every character of the message
						for (int i = 0, j = 0; i < ret; i++, j++)
						{
							// copy it into str
							str[j] = buf[i];

							// if there is a \n, we send a line with all the text to \n
							if (str[j] == '\n')
							{
								str[j+1] = '\0';
								if(currmsg[fd])
									sprintf(msg, "%s", str);
								else
									sprintf(msg, "client %d: %s", id[fd], str);
								currmsg[fd] = 0;
								sendAll(fd);
								j = -1;
							}
							// if this is the last character of the message and it's not ending by a \n (means we need to send it without \n)
							else if (i == (ret - 1))
							{
								str[j+1] = '\0';
								if (currmsg[fd])
									sprintf(msg, "%s", str);
								else
									sprintf(msg, "client %d: %s", id[fd], str);
								currmsg[fd] = 1;
								sendAll(fd);
								break;
							}
						}
					}
				}
			}
		}
	}
	return(0);
}
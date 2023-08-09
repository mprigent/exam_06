#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

int max_fd = 0, g_id = 0, sock_fd = -1;
int id[65536], curmsg[65536];

fd_set fd_read, fd_write, cursock;
char buff[4096 * 42], msg[4096 * 42 + 42], str[4096 * 42];

// ==> Send the message stock on fd_write by sprintf, to every client except one
void sendAll(int except) 
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &fd_write) && except != fd)
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
	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
		exit(1);
	}

	// ==> Create every server details (port, IP)
	uint16_t port = atoi(argv[1]);
	struct sockaddr_in serv;
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(2130706433);
	serv.sin_port = htons(port);

	// find the socket server
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		fatal();
	// bind the socket to the good port with sockaddr struct
	if (bind(sock_fd, (struct sockaddr*)&serv, sizeof(serv)) == -1)
		fatal();
	// listen to server socket
	if (listen(sock_fd, 128) == -1)
		fatal();
	
	// set to zero everything we need
	bzero(id, sizeof(id));
	FD_ZERO(&cursock);
	FD_SET(sock_fd, &cursock);
	max_fd = sock_fd;

	// ========== CONNEXION SET ✅ =========== //

	// ========== TREAT ENTRANCE MESSAGE =========== //
	while (1)
	{
		fd_write = fd_read = cursock;
		if (select(max_fd + 1, &fd_read, &fd_write, 0, 0) <= 0)
			continue;
		
		// for all clients
		for (int fd = 0; fd <= max_fd; fd++) 
		{
			// if the client is ready to be read
			if (FD_ISSET(fd, &fd_read))
			{
				// if the actuel client (ready to be read) is the server one (means a new connexion)
				if (fd == sock_fd)
				{
					// create a struct to stock new client
					struct sockaddr_in clientaddr;
					socklen_t len = sizeof(clientaddr);

					// accept the client
					int client_fd = accept(fd, (struct sockaddr*)&clientaddr, &len);
					if (client_fd == -1)
						continue;
					
					// put the client into active_sockets
					FD_SET(client_fd, &cursock);
					id[client_fd] = g_id++;
					curmsg[client_fd] = 0;
					if (max_fd < client_fd)
					max_fd = client_fd;

					// sending message to all clients
					sprintf(msg, "server: client %d just arrived\n", id[client_fd]);
					sendAll(fd);
					break;
				}
				// else the sock is ready to be read but it's not the server one (means the client is left or send a message to treat)
				else
				{
					// catch the message of the socket
					int ret = recv(fd, buff, 4096 * 42, 0);

					// if the lenght of message is <= than 0, it's mean the client left
					if (ret <= 0)
					{
						sprintf(msg, "server: client %d just left\n", id[fd]);
						sendAll(fd);

						// clear this client from active_socket & close the client socket.
						FD_CLR(fd, &cursock);
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
							str[j] = buff[i];

							// if there is a \n, we send a line with all the text to \n
							if (str[j] == '\n')
							{
								str[j+1] = '\0';
								if(curmsg[fd])
									sprintf(msg, "%s", str);
								else
									sprintf(msg, "client %d: %s", id[fd], str);
								curmsg[fd] = 0;
								sendAll(fd);
								j = -1;
							}
							// if this is the last character of the message and it's not ending by a \n (means we need to send it without \n)
							else if (i == (ret - 1))
							{
								str[j+1] = '\0';
								if (curmsg[fd])
									sprintf(msg, "%s", str);
								else
									sprintf(msg, "client %d: %s", id[fd], str);
								curmsg[fd] = 1;
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
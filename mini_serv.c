#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>

typedef struct s_clients
{
	int id;
	char msg[1024];
}	t_clients;

t_clients clients[1024];
char buff_write[120000], buff_read[1200000];
int max_fd = 0;
fd_set fd_write, fd_read, active;

void ft_error(char *str)
{
	if (str)
		write(2, str, strlen(str));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void send_all(int except)
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &fd_write) && fd != except)
			send(fd, buff_write, strlen(buff_write), 0);
}

int main (int argc, char **argv) 
{
	if (argc != 2)
	{
		ft_error("Wrong number of arguments");
	}

	int sock_fd, connfd, len;
	struct sockaddr_in servaddr;
	int next_id = 0; 

	// socket create and verification 
	sock_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock_fd == -1)
		ft_error(0);

	// Utilisé pour select
	max_fd = sock_fd;

	// On initialise tout
	bzero(&servaddr, sizeof(servaddr)); 
	bzero(&clients, sizeof(clients));
	bzero(&buff_write, sizeof(buff_write));
	bzero(&buff_read, sizeof(buff_read));
	FD_ZERO(&active);

	FD_SET(sock_fd, &active);

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		ft_error(0);
	if (listen(sock_fd, 10) != 0)
		ft_error(0);
	len = sizeof(servaddr);

	// Boucle principale
	while (1)
	{
		fd_read = fd_write = active;
		if (select(max_fd + 1, &fd_read, &fd_write, NULL, NULL) < 0)
			continue;

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &fd_read) == 0)
				continue ;
			// new connection
			if (fd == sock_fd)
			{
				// On va accepter la connexion!
				connfd = accept(sock_fd, (struct sockaddr *)&servaddr, &len);
				if (connfd < 0)
					continue;

				clients[connfd].id = next_id++;
				sprintf(buff_write, "server: client %d just arrived\n", clients[connfd].id);
				send_all(connfd);
				FD_SET(connfd, &active);
				if (connfd > max_fd)
					max_fd = connfd;
				bzero(&buff_write, sizeof(buff_write));
			}
			// client sent shit
			else
			{
				int res = recv(fd, &buff_read, 65536, 0);
				// Client is disconnecting
				if (res <= 0) // Le client se deconnecte !
				{
					sprintf(buff_write, "server: client %d just left\n", clients[fd].id);
					send_all(fd);
					FD_CLR(fd, &active);
					close(fd);
				}
				// Client sent a message
				else
				{
					for (int i = 0, j = strlen(clients[fd].msg); i < res; i++, j++)
					{
						clients[fd].msg[j] = buff_read[i];
						if (clients[fd].msg[j] == '\n')
						{
							clients[fd].msg[j] = '\0';
							sprintf(buff_write, "client %d: %s\n", clients[fd].id, clients[fd].msg);
							send_all(fd);
							bzero(&clients[fd].msg, sizeof(clients[fd].msg));
							j = -1;
						}
					}
				}
			}
			break;
		}
	}	
}


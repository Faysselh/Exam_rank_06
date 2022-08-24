#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

int sock_arr[65536];
int max_sock = 0;
int next_id = 0;
fd_set active_socks, readfds, writefds;
char buf_for_read[42 * 4096], buf_for_write[42 * 4096 + 42];
char *buf_str[42 * 4096];

void	fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit (1);
}

void	send_all(int except_sock)
{
	int len = strlen(buf_for_write);
	for (int i = 0; i <= max_sock; i++)
	{
		if (FD_ISSET(i, &writefds) && i != except_sock)
		{
			if(send(i, buf_for_write, len, 0) < 0)
				fatal_error();
		}
	}
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int		main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(av[1]);
	bzero(&sock_arr, sizeof(sock_arr));
	FD_ZERO(&active_socks);

	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock < 0)
		fatal_error();
	max_sock = server_sock;
	FD_SET(server_sock, &active_socks);

	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = (1 << 24) | 127;
		addr.sin_port = (port >> 8) | (port << 8);
	if ((bind(server_sock, (const struct sockaddr *) &addr, sizeof(addr))) < 0)
		fatal_error();
	if (listen(server_sock, SOMAXCONN) < 0)
		fatal_error();
	while (1)
	{
		readfds = writefds = active_socks;
		if (select(max_sock + 1, &readfds, &writefds, NULL, NULL) < 0)
			fatal_error();
		for (int i = 0; i <= max_sock; i++)
		{
			if (FD_ISSET(i, &readfds) && i == server_sock)
			{
				int client_sock = accept(i, (struct sockaddr *) &addr, &addr_len);
				if (client_sock < 0)
					fatal_error();
				max_sock = (client_sock > max_sock) ? client_sock : max_sock;
				sock_arr[client_sock] = next_id++;
				buf_str[client_sock] = NULL;
				FD_SET(client_sock, &active_socks);

				sprintf(buf_for_write, "server: client %d just arrived\n", sock_arr[client_sock]);
				send_all(client_sock);
				break ;
			}
			if (FD_ISSET(i, &readfds) && i != server_sock)
			{
				int read_res = recv(i, buf_for_read, 42 * 4096, 0);
				if (read_res <= 0)
				{
					sprintf(buf_for_write, "server: client %d just left\n", sock_arr[i]);
					send_all(i);
					close (i);
					free(buf_str[i]);
					buf_str[i] = NULL;
					FD_CLR(i, &active_socks);
					break ;
				}
				else
				{
					buf_for_read[read_res] = '\0';
					
					buf_str[i] = str_join(buf_str[i], buf_for_read);
					char *s = NULL;
					
					while (extract_message(&buf_str[i], &s))
					{
						sprintf(buf_for_write, "client %d: %s", sock_arr[i], s);
						send_all(i);
						free(s);
						s = NULL;
					}
				}
			}
		
		}
	}
	return (0);
}

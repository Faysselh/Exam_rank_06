/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv_good.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fayel-ha <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/05 19:49:55 by fayel-ha          #+#    #+#             */
/*   Updated: 2022/06/05 21:16:59 by fayel-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int	sock_arr[65536];
int	max_sock = 0;
int next_id = 0;
fd_set active_socks, readfds, writefds;
char buf_for_read[42*4096], buf_str[42*4096], buf_for_write[42*4096 + 42];

void	fatal_error()
{
	write (2, "Fatal error\n", 12);
	exit (1);
}

void	send_all(int except_sock)
{
	int len = strlen(buf_for_write);
	for (int i = 0; i <= max_sock; i++)
	{
		if (FD_ISSET(i, &writefds) && i != except_sock)
			send (i, buf_for_write, len, 0);
	}
}

int		main(int ac, char **av)
{
	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit (1);
	}
	int port = atoi(av[1]); //(void) port;

	bzero(&sock_arr, sizeof(sock_arr));
	FD_ZERO(&active_socks);

	//start server
	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0)
		fatal_error();
	max_sock = server_sock;
	FD_SET(server_sock, &active_socks);
	
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = (1 << 24) | 127; // 127.0.0.1
		addr.sin_port = (port >> 8) | (port << 8);
	
	if ((bind(server_sock, (const struct sockaddr *) &addr, sizeof(addr))) < 0)
		fatal_error();
	if (listen(server_sock, SOMAXCONN) < 0)
		fatal_error();

	while (1)
	{
		readfds = writefds = active_socks;
		if (select(max_sock + 1, &readfds, &writefds, NULL, NULL) < 0)
			continue ;
		for (int i = 0; i <= max_sock; i++)
		{
			if (FD_ISSET(i, &readfds) && i == server_sock)
			{
				int client_sock = accept(server_sock, (struct sockaddr *)&addr, &addr_len);
				if (client_sock < 0)
					continue ;

				max_sock = (client_sock > max_sock) ? client_sock : max_sock;
				sock_arr[client_sock] = next_id++;
				FD_SET(client_sock, &active_socks);

				sprintf(buf_for_write, "server: client %d just arrived\n", sock_arr[client_sock]);
				send_all(client_sock);
				break ;
			}
			
			if (FD_ISSET(i, &readfds) && i != server_sock)
			{
				int read_res = recv(i, buf_for_read, 42*4096, 0);

				if (read_res <= 0)
				{
					sprintf(buf_for_write, "server: client %d just left\n", sock_arr[i]);
					send_all(i);
					FD_CLR(i, &active_socks);
					close(i);
					break ;
				}
				else
				{
					for (int j = 0, n = 0; j < read_res; j++, n++)
					{
						buf_str[n] = buf_for_read[j];
						if (buf_str[n] == '\n')
						{
							buf_str[n] = '\0';
							sprintf(buf_for_write, "client %d: %s\n", sock_arr[i], buf_str);
							send_all(i);
							n = -1;
						}
					}
				}
			}
		
		}
	}
	return (0);
}

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
	struct sockaddr_in si_server;
	si_server.sin_family = AF_INET;
	si_server.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &si_server.sin_addr);
	int si_server_len = sizeof(si_server);

	int sfd_comnct = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	char buffer[MAX_BUFFER_SIZE];

	while(1)
	{
		char cmd[16];
		scanf("%s", cmd);
		sprintf(buffer, "Command: %s\n", cmd);
		sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_server, si_server_len);

		recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_server, &si_server_len);
		printf(buffer);
	}

	close(sfd_comnct);

	return 0;
}

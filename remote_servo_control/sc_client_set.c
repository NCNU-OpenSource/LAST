#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>

#define INIT_PAN_ANGLE 90
#define INIT_TILT_ANGLE 0

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
	initscr();
	noecho();
	keypad(stdscr, TRUE);

	int w, h;
	if(argc == 5)
	{
		w = atoi(argv[1]);
		h = atoi(argv[2]);
		resizeterm(h, w);
	}
	getmaxyx(stdscr, h, w);
	box(stdscr, '|', '-');
	refresh();

	mmask_t mm_old;
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, &mm_old);
	MEVENT me;
	int c;
	int pan_angle = INIT_PAN_ANGLE, tilt_angle = INIT_TILT_ANGLE;
	int x = (w - 1) * pan_angle / 360, y = (h - 1) * tilt_angle / 180;

	struct sockaddr_in si_server;
	si_server.sin_family = AF_INET;
	si_server.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &si_server.sin_addr);
	int si_server_len = sizeof(si_server);

	int sfd_comnct = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	char buffer[MAX_BUFFER_SIZE];

	while(1)
	{

		pan_angle = 360 * x / (w - 1);

		tilt_angle = 180 * y / (h - 1);

		sprintf(buffer, "Command: set\nPan angle: %3d\nTilt angle: %3d\n", pan_angle, tilt_angle);
		sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_server, si_server_len);

		recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_server, &si_server_len);
		sscanf(buffer, "Pan angle: %3d\nTilt angle: %3d\n", &pan_angle, &tilt_angle);

		mvprintw(1, 1, "Pan angle: %3d", pan_angle);
		mvprintw(2, 1, "Tilt angle: %3d", tilt_angle);
		move(y, x);
		refresh();

		c = getch();

		if(c == KEY_MOUSE)
		{
			if(getmouse(&me) == OK)
			{
				if(0 <= me.x && me.x <= w - 1)
					x = me.x;

				if(0 <= me.y && me.y <= h - 1)
					y = me.y;
			}
		}
		else
		{
			switch(c)
			{
				case KEY_UP:
				case 'W':
				case 'w':
					if(y > 0)
						y--;
					break;

				case KEY_DOWN:
				case 'S':
				case 's':
					if(y < h - 1)
						y++;
					break;

				case KEY_LEFT:
				case 'A':
				case 'a':
					if(x > 0)
						x--;
					break;

				case KEY_RIGHT:
				case 'D':
				case 'd':
					if(x < w - 1)
						x++;
					break;
			}

			if(c == 'q')
			{
				break;
			}
		}
	}

	close(sfd_comnct);
	endwin();

	return 0;
}

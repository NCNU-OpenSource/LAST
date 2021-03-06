#include <stdlib.h>
#include <ncurses.h>
#include "pigpio.h"

#define SERVO_GPIO_PAN 4
#define SERVO_GPIO_TILT 17

#define PW_ANGLE_180 2400
#define PW_ANGLE_90 1500
#define PW_ANGLE_0 600
#define PW_DIFF (PW_ANGLE_180 - PW_ANGLE_0)

#define INIT_PAN_ANGLE 90
#define INIT_TILT_ANGLE 0

void rotate(int servo_gpio, int angle);

int main(int argc, char *argv[])
{
	gpioInitialise();

	initscr();
	noecho();
	keypad(stdscr, TRUE);

	int w, h;
	if(argc == 3)
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
	int x = (w - 1) * pan_angle / 180, y = (h - 1) * tilt_angle / 180;
	while(1)
	{

		pan_angle = 180 * x / (w - 1);
		rotate(SERVO_GPIO_PAN, pan_angle);
		mvprintw(1, 1, "Pan angle: %3d", pan_angle);

		tilt_angle = 180 * y / (h - 1);
		rotate(SERVO_GPIO_TILT, tilt_angle);
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
				gpioServo(SERVO_GPIO_PAN, 0);
				gpioServo(SERVO_GPIO_TILT, 0);
				break;
			}
		}
	}

	endwin();

	gpioTerminate();

	return 0;
}

void rotate(int servo_gpio, int angle)
{
	int pw = PW_DIFF * angle / 180 + PW_ANGLE_0;
	gpioServo(servo_gpio, pw);

	return;
}

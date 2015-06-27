#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pigpio.h"

#define SERVO_GPIO_PAN 4
#define SERVO_GPIO_TILT 17

#define PW_ANGLE_180 2400
#define PW_ANGLE_90 1500
#define PW_ANGLE_0 600
#define PW_DIFF (PW_ANGLE_180 - PW_ANGLE_0)

#define MAX_BUFFER_SIZE 1024

void rotate(int servo_gpio, int angle);

int get_angle(int servo_gpio);

int gt_180;
void pan_tilt_rotate_360(int servo_gpio_pan, int pan_angle, int servo_gpio_tilt, int tilt_angle);

int main(int argc, char *argv[])
{
	gpioInitialise();

	struct sockaddr_in si_server;
	si_server.sin_family = AF_INET;
	si_server.sin_port = htons(atoi(argv[1]));
	si_server.sin_addr.s_addr = htonl(INADDR_ANY);

	int sfd_comnct = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bind(sfd_comnct, (struct sockaddr*)&si_server, sizeof(si_server));

	char buffer[MAX_BUFFER_SIZE];
	struct sockaddr_in si_client;
	int si_client_len = sizeof(si_client);

	char cmd[16];
	int pan_angle = 90, tilt_angle = 0;
	pan_tilt_rotate_360(SERVO_GPIO_PAN, pan_angle, SERVO_GPIO_TILT, tilt_angle);

	while(1)
	{
		recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_client, &si_client_len);
		printf("From IP: %s\n%s", inet_ntoa(si_client.sin_addr), buffer);
		sscanf(buffer, "Command: %s\nPan angle: %3d\nTilt angle: %3d\n", &cmd, &pan_angle, &tilt_angle);
		if(strcmp(cmd, "get") == 0)
		{
			pan_angle = get_angle(SERVO_GPIO_PAN);
			tilt_angle = get_angle(SERVO_GPIO_TILT);
			if(gt_180)
			{
				pan_angle += 180;
				tilt_angle = 180 - tilt_angle;
			}

			sprintf(buffer, "Pan angle: %3d\nTilt angle: %3d\n", pan_angle, tilt_angle);
		}
		else
			if(strcmp(cmd, "set") == 0)
			{
				pan_tilt_rotate_360(SERVO_GPIO_PAN, pan_angle, SERVO_GPIO_TILT, tilt_angle);

				sprintf(buffer, "Pan angle: %3d\nTilt angle: %3d\n", pan_angle, tilt_angle);
			}
			else
			{
				sprintf(buffer, "Invalid command: %s\n", cmd);
			}

		sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_client, si_client_len);

	}

	close(sfd_comnct);
	gpioTerminate();

	return 0;
}

void rotate(int servo_gpio, int angle)
{
	int pw = PW_DIFF * angle / 180 + PW_ANGLE_0;
	gpioServo(servo_gpio, pw);

	return;
}

int get_angle(int servo_gpio)
{
	int pw = gpioGetServoPulsewidth(servo_gpio);
	return 180 * (pw - PW_ANGLE_0) / PW_DIFF;
}

void pan_tilt_rotate_360(int servo_gpio_pan, int pan_angle, int servo_gpio_tilt, int tilt_angle)
{
	rotate(servo_gpio_tilt, tilt_angle);

	if(pan_angle > 180)
	{
		rotate(servo_gpio_tilt, 180 - tilt_angle);
		pan_angle -= 180;
		gt_180 = 1;
	}
	else
	{
		gt_180 = 0;
	}

	rotate(servo_gpio_pan, pan_angle);

	return;
}

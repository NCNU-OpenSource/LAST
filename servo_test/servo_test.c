#include <stdlib.h>
#include "pigpio.h"

#define PW_ANGLE_180 2400
#define PW_ANGLE_90 1500
#define PW_ANGLE_0 600

int main(int argc, char *argv[])
{
	gpioInitialise();

	int pin = atoi(argv[1]);
	while(1)
	{
		gpioServo(pin, PW_ANGLE_0);
		time_sleep(1);

		gpioServo(pin, PW_ANGLE_90);
		time_sleep(1);

		gpioServo(pin, PW_ANGLE_180);
		time_sleep(1);

		int pw = PW_ANGLE_180;
		while(pw > PW_ANGLE_0)
		{
			pw -= 10;
			gpioServo(pin, pw);
			time_sleep(0.02);
		}
		while(pw < PW_ANGLE_180)
		{
			pw += 10;
			gpioServo(pin, pw);
			time_sleep(0.02);
		}
	}

	gpioTerminate();

	return 0;
}

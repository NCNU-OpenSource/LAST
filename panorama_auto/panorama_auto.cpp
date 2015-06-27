#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include <iostream>
#include <vector>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <pthread.h>
#include "pigpio.h"

using namespace cv;
using namespace std;

#define SERVO_GPIO_PAN 4
#define SERVO_GPIO_TILT 17

#define PW_ANGLE_180 2400
#define PW_ANGLE_90 1500
#define PW_ANGLE_0 600
#define PW_DIFF (PW_ANGLE_180 - PW_ANGLE_0)

#define INIT_PAN_ANGLE 90
#define INIT_TILT_ANGLE 0

void rotate(int servo_gpio, int angle);
int get_angle(int servo_gpio);

int gt_180;
void pan_tilt_rotate_360(int servo_gpio_pan, int pan_angle, int servo_gpio_tilt, int tilt_angle);

typedef struct
{
	int total;
	int unit;
	int tilt_angle;
	int delay;
} pano_step_param;

bool step_fin;
bool fin;

void* pano_step_rotate(void* p_psp);

int main(int argc, char* argv[])
{
	pano_step_param psp;

	psp.total = atoi(argv[1]);
	psp.tilt_angle = atoi(argv[2]);
	string opt(argv[3]);
	int v = atoi(argv[4]);
	if(opt == "-n")
	{
		psp.unit = psp.total / v;
	}
	else
		if(opt == "-a")
		{
			psp.unit = v;
		}
	psp.delay = 2;
	pthread_t thrd_psr;
	pthread_create(&thrd_psr, NULL, pano_step_rotate, &psp);

	VideoCapture cam(0);
	vector<Mat> imgs;
	int num = 0;
	while(!fin)
	{
		Mat img;
		cam >> img;
		if(step_fin)
		{
			char buff[32];
			sprintf(buff, "Part %d", num++);
			imshow(string(buff), img);
			waitKey(10);
			imgs.push_back(img);
			step_fin = false;
		}

	}

	waitKey(0);

	cout << "Start stitching" << endl;

	Stitcher stt = Stitcher::createDefault();
	Mat pano;
	Stitcher::Status status = stt.stitch(imgs, pano);
	if(status == Stitcher::OK)
	{
		cout << "Success" << endl;
		imshow("pano", pano);
		imwrite("pano.png", pano);
	}
	else
	{
		cout << "Fail" << endl;
	}

	waitKey(0);

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


void* pano_step_rotate(void* p_psp)
{
	pano_step_param psp = *(pano_step_param*)p_psp; 

	gpioInitialise();

	int crnt_angle = 0;
	while(crnt_angle < psp.total)
	{
		pan_tilt_rotate_360(SERVO_GPIO_PAN, crnt_angle, SERVO_GPIO_TILT, psp.tilt_angle);
		time_sleep(psp.delay);
		step_fin = true;
		crnt_angle += psp.unit;
	}

	pan_tilt_rotate_360(SERVO_GPIO_PAN, psp.total, SERVO_GPIO_TILT, psp.tilt_angle);
	time_sleep(psp.delay);
	step_fin = true;

	rotate(SERVO_GPIO_PAN, 90);
	rotate(SERVO_GPIO_TILT, 0);
	time_sleep(1);

	gpioTerminate();

	fin = true;

	pthread_exit(NULL);
}

#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <pthread.h>

#define MAX_BUFFER_SIZE 1024

using namespace cv;
using namespace std;

typedef struct
{
	int start;
	int end;
	int unit;
	int tilt_angle;
	int delay;
} pano_step_param;

bool step_fin;
bool fin;

char* ip;
char* port;
void* pano_step_rotate(void* p_psp);

bool g_av, g_ah;

void vh_flip(Mat& img, bool v, bool h);

int main(int argc, char* argv[])
{
	pano_step_param psp;

	psp.start = atoi(argv[1]);
	psp.end = atoi(argv[2]);
	psp.tilt_angle = atoi(argv[3]);
	string opt(argv[4]);
	int v = atoi(argv[5]);
	string url(argv[6]);
	ip = argv[7];
	port = argv[8];

	if(opt == "-n")
	{
		psp.unit = (psp.end - psp.start) / v;
	}
	else
		if(opt == "-a")
		{
			psp.unit = v;
		}
	psp.delay = 2;
	pthread_t thrd_psr;
	pthread_create(&thrd_psr, NULL, pano_step_rotate, &psp);

	VideoCapture vc(0);
	vector<Mat> imgs;
	int num = 0;
	while(!fin)
	{
		Mat img;
		vc >> img;
		vh_flip(img, g_av, g_ah);
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

void vh_flip(Mat& img, bool v, bool h)
{
	switch(v << 1 | h)
	{
		case 0:
			break;
		case 1:
			flip(img, img, 1);
			break;
		case 2:
			flip(img, img, 0);
			break;
		case 3:
			flip(img, img, -1);
			break;
	}

	return;
}

void* pano_step_rotate(void* p_psp)
{
	pano_step_param psp = *(pano_step_param*)p_psp; 

	struct sockaddr_in si_server;
	si_server.sin_family = AF_INET;
	si_server.sin_port = htons(atoi(port));
	inet_aton(ip, &si_server.sin_addr);
	int si_server_len = sizeof(si_server);

	int sfd_comnct = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	char buffer[MAX_BUFFER_SIZE];

	int crnt_angle = psp.start;
	int pan_angle, tilt_angle;
	while(crnt_angle < psp.end)
	{
		sprintf(buffer, "Command: set\nPan angle: %3d\nTilt angle: %3d\n", crnt_angle, psp.tilt_angle);
		sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_server, si_server_len);
		recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_server, (socklen_t*)&si_server_len);
		sscanf(buffer, "Pan angle: %3d\nTilt angle: %3d\n", &pan_angle, &tilt_angle);
		printf("Pan angle: %3d\nTilt angle: %3d\n", pan_angle, tilt_angle);
		sleep(psp.delay);
		if((tilt_angle > 90 && pan_angle <= 180) || (tilt_angle < 90 && pan_angle > 180))
		{
			cout << pan_angle << endl;
			g_av = true;
			g_ah = true;
		}
		else
		{
			g_av = false;
			g_ah = false;
		}
		step_fin = true;
		crnt_angle += psp.unit;
	}

	sprintf(buffer, "Command: set\nPan angle: %3d\nTilt angle: %3d\n", psp.end, psp.tilt_angle);
	sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_server, si_server_len);
	recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_server, (socklen_t*)&si_server_len);
	printf(buffer);
	sscanf(buffer, "Pan angle: %3d\nTilt angle: %3d\n", &pan_angle, &tilt_angle);
	sleep(psp.delay);
	if((tilt_angle > 90 && pan_angle <= 180) || (tilt_angle < 90 && pan_angle > 180))
	{
		cout << pan_angle << endl;
		g_av = true;
		g_ah = true;
	}
	else
	{
		g_av = false;
		g_ah = false;
	}
	step_fin = true;

	sprintf(buffer, "Command: set\nPan angle: %3d\nTilt angle: %3d\n", 90, 0);
	sendto(sfd_comnct, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&si_server, si_server_len);
	recvfrom(sfd_comnct, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_server, (socklen_t*)&si_server_len);
	printf(buffer);
	sleep(1);

	fin = true;

	pthread_exit(NULL);
}

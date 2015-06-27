#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <cmath>

using namespace cv;
using namespace std;

Point g_roi_p1, g_roi_p2;
bool g_new_roi;
vector<Rect> g_rois;
Rect g_screen;
Size g_zoom_unit;
Rect g_zoom_area;
bool g_show_zoom_area;

int g_mv, g_mh;
int g_blur_ksize;
int g_hsv_enable;
int g_h_offset, g_s_scale, g_v_scale;
int g_z_enable;
int g_draw_rects;
int g_draw_contours;
int g_r_enable;
int g_angle;
int g_adj_size;

class bgs_param
{
public:
	int nframes;
	int history;

	bgs_param()
	{
		nframes = 1;
	}
};
bgs_param g_bgsp;

class morph_param
{
public:
	int shape;
	int ksize;
	int dilate_iterations;
	int erode_iterations;

	morph_param()
	{
		shape = MORPH_RECT;
	}
};
morph_param g_mp;

bool init_video_capture(VideoCapture& vc, string type, string source);

long time_diff_nano(struct timespec t_start, struct timespec t_end);

double cal_avg_fps(VideoCapture& vc, int count);

void set_roi(int event, int x, int y, int flags, void* userdata);

void close_r_window(int r_enable, void* userdata);

void close_z_window(int z_enable, void* userdata);

void create_ui();

void vh_flip(Mat& img, bool v, bool h);

template<typename T>
void do_bgs(string alg, bgs_param& param, T& bgs, const Mat& frame, Mat& fgmask);

void process_fgmask(Mat& fgmask, morph_param mp, vector<Rect>& rois);

void adjust_hsv(Mat& img, int h_offset, double s_scale, double v_scale);

void draw_rois(Mat& frame);

void bound_fg(Mat& frame, Mat& fgmask, vector<Rect>& rects, bool draw_rects = true, bool draw_contours = false);

string get_time_string(time_t t, char* format);

void rotate_image(Mat& src, Mat& dst, double angle, double scale = 1.0, bool adj_size = false);

void zoom(Mat& src, Mat& dst, Rect area, double scale = 0);

int main(int argc, char* argv[])
{
	VideoCapture vc;
	string type(argv[1]), source(argv[2]);
	if(!init_video_capture(vc, type, source))
	{
		return -1;
	}

	g_screen = Rect(0, 0, vc.get(CV_CAP_PROP_FRAME_WIDTH), vc.get(CV_CAP_PROP_FRAME_HEIGHT));

	g_zoom_unit = Size(g_screen.width / 16, g_screen.height / 16);

	g_zoom_area = Rect(Point(0, 0), g_zoom_unit);

	double avg_fps = cal_avg_fps(vc, 64);
	
	create_ui();
	
	time_t timestamp;
	string ts;
	Point ts_pos;
	int ts_font_face = FONT_HERSHEY_SIMPLEX;
	double ts_font_scale = 0.5;
	Scalar ts_color = CV_RGB(0, 0, 0);
	int ts_thickness = 1;
	int ts_baseline;
	Size ts_size;

	Mat org_frame, prep_frame, fgmask;
	BackgroundSubtractorMOG2 bgs_mog2(500, 16, false);
	VideoWriter vw;
	bool no_motion_event = true;
	while((char)waitKey(10) != 'q')
	{
		vc >> org_frame;
		time(&timestamp);
		ts = get_time_string(timestamp, "%c");

		vh_flip(org_frame, g_mv, g_mh);

		blur(org_frame, prep_frame, Size(g_blur_ksize * 2 + 1, g_blur_ksize * 2 + 1));

		do_bgs("MOG2", g_bgsp, bgs_mog2, prep_frame, fgmask);

		process_fgmask(fgmask, g_mp, g_rois);

		if(g_hsv_enable)
		{
			adjust_hsv(org_frame, g_h_offset, g_s_scale, g_v_scale);
		}

		if(g_z_enable)
		{
			Mat z_frame;
			zoom(org_frame, z_frame, g_zoom_area);
			imshow("Zoom", z_frame);
		}

		draw_rois(org_frame);

		ts_size = getTextSize(ts, ts_font_face, ts_font_scale, ts_thickness, &ts_baseline);
		ts_pos.x = 0;
		ts_pos.y = ts_size.height;

		putText(org_frame, ts, ts_pos, ts_font_face, ts_font_scale, ts_color, ts_thickness);

		vector<Rect> rects;
		bound_fg(org_frame, fgmask, rects, g_draw_rects, g_draw_contours);

		if(rects.size() > 0)
		{
			if(no_motion_event)
			{
				vw.open("motion_" + ts + ".avi", CV_FOURCC('X', 'V', 'I', 'D'), avg_fps, org_frame.size());
				no_motion_event = false;
			}
			vw << org_frame;
		}
		else
		{
			no_motion_event = true;
		}

		imshow("Motion Detection", org_frame);

		if(g_r_enable)
		{
			Mat r_frame;
			rotate_image(org_frame, r_frame, g_angle, 1.0, g_adj_size);
			imshow("Rotation", r_frame);
		}

	}

	return 0;
}

bool init_video_capture(VideoCapture& vc, string type, string source)
{
	if(type == "--video" || type == "--url")
	{
		vc.open(source);
	}
	else
		if(type == "--camera")
		{
			vc.open(atoi(source.c_str()));
		}

	return vc.isOpened();
}

long time_diff_nano(struct timespec t_start, struct timespec t_end)
{
	return (t_end.tv_sec - t_start.tv_sec) * 1000000000 + (t_end.tv_nsec - t_start.tv_nsec);
}

double cal_avg_fps(VideoCapture& vc, int count)
{
	long sum = 0;
	struct timespec t_start, t_end;
	Mat tmp_frame;
	for(int i = 0; i < count; i++)
	{
		clock_gettime(CLOCK_REALTIME, &t_start);
		vc >> tmp_frame;
		clock_gettime(CLOCK_REALTIME, &t_end);
		sum += time_diff_nano(t_start, t_end);
	}
	return 1000000000.0 * count / sum;
}

void set_roi(int event, int x, int y, int flags, void* userdata)
{
	if(flags & EVENT_FLAG_CTRLKEY)
	{
		g_show_zoom_area = true;
	}
	else
	{
		g_show_zoom_area = false;
	}

	switch(event)
	{
		case EVENT_LBUTTONDOWN:
			if(flags & EVENT_FLAG_CTRLKEY)
			{
				Rect exp_area = g_zoom_area;
				exp_area += g_zoom_unit;

				int half_w = exp_area.width / 2;
				int half_h = exp_area.height / 2;

				if(x - half_w >= 0 && x + half_w < g_screen.width && y - half_h >= 0 && y + half_h < g_screen.height)
				{
					g_zoom_area = exp_area;
					g_zoom_area.x = x - half_w;
					g_zoom_area.y = y - half_h;
				}
			}
			else
			{
				g_roi_p1 = Point(x, y);
			}
			break;

		case EVENT_MOUSEMOVE:
			if(flags & EVENT_FLAG_CTRLKEY)
			{
				int half_w = g_zoom_area.width / 2;
				int half_h = g_zoom_area.height / 2;

				if(x - half_w >= 0 && x + half_w < g_screen.width)
				{
					g_zoom_area.x = x - half_w;
				}

				if(y - half_h >= 0 && y + half_h < g_screen.height)
				{
					g_zoom_area.y = y - half_h;
				}
			}
			else
				if(flags & EVENT_FLAG_LBUTTON)
				{
					if(0 <= x && x < g_screen.width)
					{
						g_roi_p2.x = x;
					}

					if(0 <= y && y < g_screen.height)
					{
						g_roi_p2.y = y;
					}

					g_new_roi = true;
				}
			break;

		case EVENT_LBUTTONUP:
			if(g_new_roi)
			{
				g_rois.push_back(Rect(g_roi_p1, g_roi_p2));
				g_new_roi = false;
			}
			break;

		case EVENT_RBUTTONUP:
			for(int i = 0; i < g_rois.size(); i++)
			{
				if(g_rois[i].contains(Point(x, y)))
				{
					g_rois.erase(g_rois.begin() + i);
					i--;
				}
			}
			break;

		case EVENT_RBUTTONDOWN:
			if(flags & EVENT_FLAG_CTRLKEY)
			{
				Rect shr_area = g_zoom_area; 
				shr_area -= g_zoom_unit;

				int half_w = shr_area.width / 2;
				int half_h = shr_area.height / 2;

				if(shr_area.area() > 0 && x - half_w >= 0 && x + half_w < g_screen.width && y - half_h >= 0 && y + half_h < g_screen.height)
				{
					g_zoom_area = shr_area;
					g_zoom_area.x = x - half_w;
					g_zoom_area.y = y - half_h;
				}
			}
			break;
	}

	return;
}

void close_r_window(int r_enable, void* userdata)
{
	if(!r_enable)
	{
		destroyWindow("Rotation");
	}

	return;
}

void close_z_window(int z_enable, void* userdata)
{
	if(!z_enable)
	{
		destroyWindow("Zoom");
	}

	return;
}

void create_ui()
{
	namedWindow("Motion Detection");
	setMouseCallback("Motion Detection", set_roi);

	namedWindow("Parameters1");

	createTrackbar("history", "Parameters1", &g_bgsp.history, 100);
	setTrackbarPos("history", "Parameters1", 50);

	createTrackbar("vertical flipping", "Parameters1", &g_mv, 1);
	setTrackbarPos("vertical flipping", "Parameters1", 0);

	createTrackbar("horizontal flipping", "Parameters1", &g_mh, 1);
	setTrackbarPos("horizontal flipping", "Parameters1", 0);

	createTrackbar("hsv enable", "Parameters1", &g_hsv_enable, 1);
	setTrackbarPos("hsv enable", "Parameters1", 0);

	createTrackbar("hue", "Parameters1", &g_h_offset, 180);
	setTrackbarPos("hue", "Parameters1", 0);

	createTrackbar("saturation", "Parameters1", &g_s_scale, 8);
	setTrackbarPos("saturation", "Parameters1", 1);

	createTrackbar("value", "Parameters1", &g_v_scale, 8);
	setTrackbarPos("value", "Parameters1", 1);

	createTrackbar("blur ksize", "Parameters1", &g_blur_ksize, 10);
	setTrackbarPos("blur ksize", "Parameters1", 2);

	namedWindow("Parameters2");

	createTrackbar("se ksize", "Parameters2", &g_mp.ksize, 10);
	setTrackbarPos("se ksize", "Parameters2", 1);

	createTrackbar("dilate iterations", "Parameters2", &g_mp.dilate_iterations, 16);
	setTrackbarPos("dilate iterations", "Parameters2", 1);

	createTrackbar("erode iterations", "Parameters2", &g_mp.erode_iterations, 16);
	setTrackbarPos("erode iterations", "Parameters2", 1);

	createTrackbar("draw rects", "Parameters2", &g_draw_rects, 1);
	setTrackbarPos("draw rects", "Parameters2", 1);

	createTrackbar("draw contours", "Parameters2", &g_draw_contours, 1);
	setTrackbarPos("draw contours", "Parameters2", 0);

	createTrackbar("enable rotate", "Parameters2", &g_r_enable, 1, close_r_window);
	setTrackbarPos("enable rotate", "Parameters2", 0);

	createTrackbar("adjust size", "Parameters2", &g_adj_size, 1);
	setTrackbarPos("adjust size", "Parameters2", 0);

	createTrackbar("rotation angle", "Parameters2", &g_angle, 360);
	setTrackbarPos("rotation angle", "Parameters2", 0);

	createTrackbar("enable zoom", "Parameters2", &g_z_enable, 1, close_z_window);
	setTrackbarPos("enable zoom", "Parameters2", 0);

	return;
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

template<typename T>
void do_bgs(string alg, bgs_param& param, T& bgs, const Mat& frame, Mat& fgmask)
{
	if(alg == "MOG2")
	{
		if(param.nframes * 2 < 500)
		{
			bgs(frame, fgmask);
			param.nframes++;
		}
		else
		{
			bgs(frame, fgmask, 1.0 / ((param.history > 0? param.history : 1) * 10));
		}
	}
	return;
}

void process_fgmask(Mat& fgmask, morph_param mp, vector<Rect>& rois)
{

	Mat se = getStructuringElement(mp.shape, Size(mp.ksize * 2 + 1, mp.ksize * 2 + 1));
	dilate(fgmask, fgmask, se, Point(-1, -1), mp.dilate_iterations);
	erode(fgmask, fgmask, se, Point(-1, -1), mp.erode_iterations);

	Mat rois_mask(fgmask.size(), fgmask.type(), Scalar(0));
	for(int i = 0; i < rois.size(); i++)
	{
		rois_mask(rois[i]) = Scalar(255);
	}

	fgmask &= rois_mask;

	return;
}

void adjust_hsv(Mat& img, int h_offset, double s_scale, double v_scale)
{
	Mat img_hsv;
	cvtColor(img, img_hsv, CV_BGR2HSV);
	for(MatIterator_<Vec3b> iter = img_hsv.begin<Vec3b>(); iter != img_hsv.end<Vec3b>(); iter++)
	{
		if((*iter)[0] + h_offset <= 180)
		{
			(*iter)[0] += h_offset;
		}

		if(s_scale > 0 && (*iter)[1] * s_scale <= 255)
		{
			(*iter)[1] *= s_scale;
		}
		else
		{
			(*iter)[1] = 255;
		}

		if(v_scale > 0 && (*iter)[2] * v_scale <= 255)
		{
			(*iter)[2] *= v_scale;
		}
		else
		{
			(*iter)[2] = 255;
		}
	}
	cvtColor(img_hsv, img, CV_HSV2BGR);

	return;
}

void bound_fg(Mat& frame, Mat& fgmask, vector<Rect>& rects, bool draw_rects, bool draw_contours)
{
	vector<vector<Point> > contours;
	findContours(fgmask.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	for(int i = 0; i < contours.size(); i++)
	{
		Rect rect = boundingRect(contours[i]);
		bool outside = true;
		for(int j = 0; j < rects.size(); j++)
		{
			Rect outside_rect = rect | rects[j];
			if(outside_rect == rects[j])
			{
				outside = false;
				break;
			}
			if(outside_rect == rect)
			{
				rects.erase(rects.begin() + j);
				j--;
			}
		}
		if(outside)
		{
			rects.push_back(rect);
		}
	}


	if(draw_rects)
	{
		for(int i = 0; i < rects.size(); i++)
		{
			rectangle(frame, rects[i], CV_RGB(0, 255, 0));
		}
	}

	if(draw_contours)
	{
		drawContours(frame, contours, -1, CV_RGB(255, 0, 0));
	}

	return;
}

void draw_rois(Mat& frame)
{
	for(int i = 0; i < g_rois.size(); i++)
	{
		rectangle(frame, g_rois[i], CV_RGB(0, 0, 255));
	}

	if(g_new_roi)
	{
		rectangle(frame, g_roi_p1, g_roi_p2, CV_RGB(0, 0, 255));
	}

	if(g_show_zoom_area)
	{
		rectangle(frame, g_zoom_area, CV_RGB(255, 255, 255));
	}

	return;
}

string get_time_string(time_t t, char* format)
{
	char buffer[32];
	strftime(buffer, 32, format, localtime(&t));
	return string(buffer);
}

void rotate_image(Mat& src, Mat& dst, double angle, double scale, bool adj_size)
{
	int org_w = src.size().width;
	int org_h = src.size().height;
	Point2f center(org_w / 2.0, org_h / 2.0);
	Mat rm = getRotationMatrix2D(center, angle, scale);
	if(adj_size)
	{
		Rect br = RotatedRect(center, src.size(), angle).boundingRect();
		rm.at<double>(0, 2) += (br.width - org_w) / 2;
		rm.at<double>(1, 2) += (br.height - org_h) / 2;
		warpAffine(src, dst, rm, br.size());
	}
	else
	{
		double side_len = sqrt((double)org_w * org_w + (double)org_h * org_h);
		rm.at<double>(0, 2) += (side_len - org_w) / 2;
		rm.at<double>(1, 2) += (side_len - org_h) / 2;
		warpAffine(src, dst, rm, Size(side_len, side_len));
	}

	return;
}

void zoom(Mat& src, Mat& dst, Rect area, double scale)
{
	dst = src(area);
	if(scale == 0)
	{
		resize(dst, dst, src.size());
	}
	else
	{
		resize(dst, dst, Size(0, 0), scale, scale);
	}

	return;
}

// Prototype3.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgproc.hpp>
#include <wtypes.h>
#include <math.h>
#include <boost\filesystem.hpp>
#include <opencv2\features2d.hpp>
//#include <boost/thread.hpp> 
//#include <boost/bind.hpp>
//#include <boost/thread/future.hpp>
//#include <pthread.h>

//#include <opencv2\xfeatures2d.hpp>

using namespace std;
using namespace cv;
namespace bfs = boost::filesystem;
//using namespace boost::posix_time;
//namespace xf = xfeatures2d;

typedef struct relcord
{
	int tx;
	int ty;
	Point bridge;

	relcord()
	{
		tx = 0;
		ty = 0;
		bridge.x = 0;
		bridge.y = 0;
	}
}relcord;

int initCamera(VideoCapture &mscam)
{
	//video stream parameters
	mscam.set(CAP_PROP_FPS, 40);
	mscam.set(CAP_PROP_FRAME_WIDTH, 640);
	mscam.set(CAP_PROP_FRAME_HEIGHT, 480);

	//using the default camera
	mscam.open(0);
	if (!mscam.isOpened())
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

Size getScreenRes()
{
	Size temp(0, 0);
	RECT screen;
	const HWND hScreen = GetDesktopWindow();
	GetWindowRect(hScreen, &screen);
	temp.width = screen.right;
	temp.height = screen.bottom;
	return temp;
}

void getOffset(Size screenres, Size currentres, Point2d &vp2cur)
{
	vp2cur.x = (screenres.width - currentres.width) / 2;
	vp2cur.y = (screenres.height - currentres.height) / 2;
}

relcord displace(relcord origin, int xhor, int yver, int tilesize)
{
	relcord temp;
	if (origin.bridge.x + xhor < 0)
	{
		int rem = origin.bridge.x + xhor;
		int mul = ceil((rem*-1.0) / tilesize);
		temp.tx = origin.tx - mul;
		temp.bridge.x = (mul*tilesize) + rem;
	}
	else if (origin.bridge.x + xhor > tilesize - 1)
	{
		int rem = origin.bridge.x + xhor;
		int mul = floor((rem*1.0) / tilesize);
		temp.tx = origin.tx + mul;
		temp.bridge.x = rem - (mul*tilesize);
	}
	else
	{
		temp.tx = origin.tx;
		temp.bridge.x = origin.bridge.x + xhor;
	}

	if (origin.bridge.y + yver < 0)
	{
		int rem = origin.bridge.y + yver;
		int mul = ceil((rem*-1.0) / tilesize);
		temp.ty = origin.ty - mul;
		temp.bridge.y = (mul*tilesize) + rem;
	}
	else if (origin.bridge.y + yver > tilesize - 1)
	{
		int rem = origin.bridge.y + yver;
		int mul = floor((rem*1.0) / tilesize);
		temp.ty = origin.ty + mul;
		temp.bridge.y = rem - (mul*tilesize);
	}
	else
	{
		temp.ty = origin.ty;
		temp.bridge.y = origin.bridge.y + yver;
	}
	return temp;
}

Mat getCanvas(string tiledir, relcord canvastlc, int reqwidth, int reqheight, int tilesize, int gridspan[])
{
	relcord br = displace(canvastlc, reqwidth - 1, reqheight - 1, tilesize);
	if (canvastlc.tx < gridspan[0])
	{
		gridspan[0] = canvastlc.tx;
	}
	if (br.tx > gridspan[1])
	{
		gridspan[1] = br.tx;
	}
	if (canvastlc.ty < gridspan[2])
	{
		gridspan[2] = canvastlc.ty;
	}
	if (br.ty > gridspan[3])
	{
		gridspan[3] = br.ty;
	}
	Mat et = Mat::zeros(Size(tilesize, tilesize), CV_8UC3);
	//build all rows
	vector<Mat> crows;
	for (int i = canvastlc.ty; i <= br.ty; i++)
	{
		vector<Mat> crowbuffer;
		for (int j = canvastlc.tx; j <= br.tx; j++)
		{
			ostringstream ss;
			ss << "\\" << j;
			if (!bfs::exists(tiledir + ss.str()))
			{
				bfs::create_directory(tiledir + ss.str());
			}
			ss << "\\" << i << ".bmp";
			Mat temp = imread(tiledir + ss.str());
			if (temp.empty())
			{
				imwrite(tiledir + ss.str(), et);
				temp = imread(tiledir + ss.str());
			}
			crowbuffer.push_back(temp);
		}
		if (crowbuffer.size() == 1)
		{
			crows.push_back(crowbuffer[0]);
		}
		else
		{
			Mat hcres;
			hconcat(crowbuffer, hcres);
			crows.push_back(hcres);
		}
	}
	//assemble all rows
	if (crows.size() == 1)
	{
		return crows[0];
	}
	else
	{
		Mat canvas;
		vconcat(crows, canvas);
		return canvas;
	}
}

bool getFrame(VideoCapture mscam, Mat &current)
{
	bool flag = false;
	Mat frame;
	flag = mscam.read(frame);
	Mat result = Mat::zeros(current.size(), current.type());
	if (flag && frame.cols >= current.cols && frame.rows >= current.rows)
	{
		Rect roi((frame.cols - current.cols) / 2, (frame.rows - current.rows) / 2, current.cols, current.rows);
		result = frame(roi);
		result.copyTo(current);
		return true;
	}
	else
	{
		result.copyTo(current);
		return false;
	}
}

void updateCanvas(Mat canvas, string tiledir, int tilesize, relcord canvastlc)
{
	int rc = canvas.rows / tilesize;
	int cc = canvas.cols / tilesize;

	int fy = canvastlc.ty;
	for (int i = 0; i < rc; i++)
	{
		int fx = canvastlc.tx;
		for (int j = 0; j < cc; j++)
		{
			Mat temp = canvas(Rect

			(j*tilesize, i*tilesize, tilesize, tilesize));
			ostringstream ss;
			ss << "\\" << fx << "\\" << fy << ".bmp";
			imwrite(tiledir + ss.str(), temp);
			fx++;
		}
		fy++;
	}
}

void saveScan(string tiledir, int gridspan[], int tilesize)
{
	Mat blank = Mat::zeros(Size(tilesize, tilesize), CV_8UC3);
	vector<Mat> rows;
	for (int i = gridspan[2]; i <= gridspan[3]; i++)
	{
		vector<Mat> buffer;
		for (int j = gridspan[0]; j <= gridspan[1]; j++)
		{
			ostringstream ss;
			ss << "\\" << j << "\\" << i << ".bmp";
			Mat temp = imread(tiledir + ss.str());
			if (temp.empty())
			{
				buffer.push_back(blank);
			}
			else
			{
				buffer.push_back(temp);
			}
		}
		Mat crow;
		hconcat(buffer, crow);
		rows.push_back(crow);
		buffer.clear();
	}
	Mat output;
	vconcat(rows, output);
	imwrite("result.bmp", output);
	bfs::remove_all(tiledir);
}

bool placeTM(Mat current, Mat &window, relcord wtlc, Size microtile, Point2d &tvec, int tilesize)
{
	Mat c;
	cvtColor(current, c, CV_BGR2GRAY);
	Canny(c, c, 100, 200, 5);

	Mat w;
	cvtColor(window, w, CV_BGR2GRAY);
	Mat mask;
	threshold(w, mask, 0, 255, CV_THRESH_BINARY);
	erode(mask, mask, Mat());
	Canny(w, w, 100, 200, 5);
	Mat scene = Mat::zeros(window.size(), CV_8UC1);
	w.copyTo(scene, mask);

	Mat MM;
	matchTemplate(scene, c, MM, CV_TM_CCORR_NORMED);
	Point match;
	double mv = 0.0;
	minMaxLoc(MM, NULL, &mv, NULL, &match);

	if (mv >= 0.5)
	{
		relcord ctlc = displace(wtlc, match.x, match.y, tilesize);
		Point topleft;
		topleft.x = microtile.width - (ctlc.bridge.x % microtile.width);
		topleft.y = microtile.height - (ctlc.bridge.y % microtile.height);
		int xmul = (current.cols - topleft.x) / microtile.width;
		int ymul = (current.rows - topleft.y) / microtile.height;
		Point botright;
		botright.x = (xmul*microtile.width) + topleft.x;
		botright.y = (ymul*microtile.height) + topleft.y;
		Mat mask = Mat::zeros(current.size(), CV_8UC1);
		rectangle(mask, topleft, botright, Scalar(255), -1, 8);
		Rect X = Rect(match.x, match.y, current.cols, current.rows);
		current.copyTo(window(X), mask);
		tvec.x = match.x - current.cols;
		tvec.y = match.y - current.rows;
		return true;
	}
	else
	{
		return false;
	}
}

int getSD(Mat image)
{
	cvtColor(image, image, CV_BGR2GRAY);
	Laplacian(image, image, CV_8UC1, 1);
	Scalar m, sd;
	meanStdDev(image, m, sd, noArray());
	return int(sd[0]);
}

int main()
{
	//Set tile buffer directory
	string exeprojpath = "E:\\MANUAL SLIDE SCANNING SYSTEM BY PRANJAL DT. 6.11.2017";
	string tiledir = exeprojpath + "\\Tiles";
	if (bfs::exists(tiledir))
	{
		bfs::remove_all(tiledir);
	}
	bfs::create_directory(tiledir);

	//Initialize gridspan
	int gridspan[4];
	for (int i = 0; i < 4; i++)
	{
		gridspan[i] = 0;
	}

	//Set current's resolution
	const Size currentres(400, 300);
	const Size microtile(40, 30);

	//Initialize the camera
	VideoCapture mscam;
	int index = initCamera(mscam);
	if (index < 0)
	{
		bfs::remove_all(tiledir);
		cout << "Camera not found\n\n";
		getchar();
		return 1;
	}
	else
	{
		cout << "Using the default camera\n\n";
	} //wait for a frame to be read

	//Obtain the screen display resolution
	Size screenres = getScreenRes();
	if (screenres.width < 800 || screenres.height < 600)
	{
		bfs::remove_all(tiledir);
		cout << "Use a display resolution of [800 x 600] or higher\n\n";
		getchar();
		return 1;
	}

	//Set tile size
	const int tilesize = 256;

	//Calculate requisite offsets
	Point2d vp2cur;
	getOffset(screenres, currentres, vp2cur);

	//Setup the canvas
	relcord canvastlc;
	canvastlc.bridge.x = 40;
	canvastlc.bridge.y = 30;
	/**/int reqwidth = canvastlc.bridge.x + screenres.width;
	/**/int reqheight = canvastlc.bridge.y + screenres.height;
	/**/Mat canvas = getCanvas(tiledir, canvastlc, reqwidth, reqheight, tilesize, gridspan);

	//Initializations
	const string win = "MWSI";
	namedWindow(win, CV_WINDOW_AUTOSIZE);
	Mat current = Mat::zeros(currentres, CV_8UC3);
	Rect curonvp(vp2cur.x, vp2cur.y, currentres.width, currentres.height);
	Mat vp, viewport;

	/**/Rect vpfromcanvas(canvastlc.bridge.x, canvastlc.bridge.y, screenres.width, screenres.height);
	vp = canvas(vpfromcanvas);
	vp.copyTo(viewport);

	//Loop 1
	bool cameraFlag = false;
	Mat temp = Mat::zeros(currentres, CV_8UC3);
	int fscore = 0;
	while (1)
	{
		cameraFlag = getFrame(mscam, current);
		if (!cameraFlag)
		{
			break;
		}
		fscore = getSD(current);
		ostringstream ss;
		ss << fscore;
		current.copyTo(temp);
		rectangle(temp, Point(0, 0), Point(temp.cols - 1, temp.rows - 1), Scalar(0, 0, 255), 1, 8);
		putText(temp, ss.str(), Point(10, 30), CV_FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, 8);
		temp.copyTo(viewport(curonvp));
		imshow(win, viewport);
		int key;
		key = waitKey(1);
		if ((char)key == 's')
		{
			current.copyTo(canvas(Rect(canvastlc.bridge.x + vp2cur.x, canvastlc.bridge.y + vp2cur.y, currentres.width, currentres.height)));
			updateCanvas(canvas, tiledir, tilesize, canvastlc);
			break;
		}
		else if ((char)key == 'q')
		{
			cout << "\nUser action: [Q] Exit program.";
			destroyWindow(win);
			bfs::remove_all(tiledir);
			getchar();
			return 0;
			//break;
		}
	}
	if (!cameraFlag)
	{
		destroyWindow(win);
		bfs::remove_all(tiledir);
		cout << "Camera disconnected. Program will terminate.\n\n";
		getchar();
		return 1;
	}


	//Loop 2
	cameraFlag = false;
	Point shift;
	relcord wtlc;
	Mat minicanvas;
	Mat window = Mat::zeros(Size(currentres.width * 3, currentres.height * 3), current.type());
	bool sflag = false;
	Point2d tvec;
	fscore = 0;
	while (1)
	{
		cameraFlag = getFrame(mscam, current);
		if (!cameraFlag)
		{
			break;
		}
		fscore = getSD(current);
		ostringstream ss;
		ss << fscore;
		shift.x = vp2cur.x - currentres.width;
		shift.y = vp2cur.y - currentres.height;
		wtlc = displace(canvastlc, shift.x, shift.y, tilesize);
		//wtlc = tgroup.create_thread(&displace, canvastlc, shift.x, shift.y, tilesize);
		minicanvas = getCanvas(tiledir, wtlc, window.cols, window.rows, tilesize, gridspan);
		window = minicanvas(Rect(wtlc.bridge.x, wtlc.bridge.y, window.cols, window.rows));
		sflag = placeTM(current, window, wtlc, microtile, tvec, tilesize);
		if (sflag)
		{
			updateCanvas(minicanvas, tiledir, tilesize, wtlc);
			canvastlc = displace(canvastlc, tvec.x, tvec.y, tilesize);
			reqwidth = canvastlc.bridge.x + screenres.width;
			reqheight = canvastlc.bridge.y + screenres.height;
			canvas = getCanvas(tiledir, canvastlc, reqwidth, reqheight, tilesize, gridspan);
			vpfromcanvas = Rect(canvastlc.bridge.x, canvastlc.bridge.y, screenres.width, screenres.height);
		}

		vp = canvas(vpfromcanvas);
		vp.copyTo(viewport);
		putText(current, ss.str(), Point(10, 30), CV_FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, 8);
		if (sflag)
		{
			rectangle(current, Point(0, 0), Point(current.cols - 1, current.rows - 1), Scalar(0, 255, 0), 1, 8);
		}
		else
		{
			rectangle(current, Point(0, 0), Point(current.cols - 1, current.rows - 1), Scalar(0, 0, 255), 1, 8);
		}
		current.copyTo(viewport(curonvp));
		imshow(win, viewport);
		int key;
		key = waitKey(1);
		if ((char)key == 'f')
		{
			updateCanvas(canvas, tiledir, tilesize, canvastlc);
			break;
		}
		else if ((char)key == 'q')
		{
			cout << "\nUser action: [Q] Exit program.";
			destroyWindow(win);
			//bfs::remove_all(tiledir);
			getchar();
			//return 0;
			break;
		}
	}
	if (!cameraFlag)
	{
		destroyWindow(win);
		bfs::remove_all(tiledir);
		cout << "Camera disconnected. Program will terminate.\n\n";
		getchar();
		return 1;
	}
	destroyAllWindows();

	saveScan(tiledir, gridspan, tilesize);

	cout << "\nProgram run successful. Next step -> Assembly to TIFF."; getchar();

	return 0;
}


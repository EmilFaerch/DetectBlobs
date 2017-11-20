#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <iostream><
#include <thread>
#include <mutex>

using namespace cv;
using namespace std;

/* Function Headers */
// Initializing and Processing ----------------------
Mat desktopCapture(Mat input);
void takeBackground();
void increaseContrast(Mat temp, int amount);
void makeBinary(Mat temp, int threshold);
void showOutputLoop();
// Find objects (Ships/Squadrons) -------------
void findObject(Mat objTemplate, string name, double rotation, int team);
void findAllOnce();

/* References */
Mat input, background, output;

Mat p1_ship = imread("p1_ship.PNG", 0); 
Mat p1_sq1 = imread("p1_sq1.PNG", 0);
Mat p1_sq2 = imread("p1_sq2.PNG", 0);

Mat p2_ship = imread("p2_ship.PNG", 0);
Mat p2_sq1 = imread("p2_sq1.PNG", 0);
Mat p2_sq2 = imread("p2_sq2.PNG", 0);

double team1_minMatchValue = 0.75;
double team2_minMatchValue = 0.7;
double bestMatch = 0;

recursive_mutex mtx, mtxloop;

int main(int, char)
{
	Sleep(2000);

	takeBackground();

	thread threadOutput(showOutputLoop);

	cout << "Looking for P1 Ship" << endl;
	thread p1_ship(findObject, p1_ship, "P1 Ship", 0, 1);
	cout << "Looking for P1 SQ1" << endl;
	thread p1_sq1(findObject, p1_sq1, "P1 SQ1", 0, 1);
	cout << "Looking for P1 SQ2" << endl;
	thread p1_sq2(findObject, p1_sq2, "P2 SQ2", 0, 2);

	cout << "Looking for P2 Ship" << endl;
	thread p2_ship(findObject, p2_ship, "P2 Ship", 0, 1);
	cout << "Looking for P2 SQ1" << endl;
	thread p2_sq1(findObject, p2_sq1, "P2 SQ1", 0, 1);
	cout << "Looking for P2 SQ2" << endl;
	thread p2_sq2(findObject, p2_sq2, "P2 SQ2", 0, 2);

//	findAllOnce();

	waitKey(0);

	p1_ship.join();
	p1_sq1.join();
	p1_sq2.join();

	p2_ship.join();
	p2_sq1.join();
	p2_sq2.join();

	threadOutput.join();

	return 0;
}

void takeBackground(){
	background = desktopCapture(background);
	cvtColor(background, background, CV_RGB2GRAY);

	imshow("[BG] Press Space to continue", background);
	waitKey(0);

	input = desktopCapture(input);
	cvtColor(input, input, CV_RGB2GRAY);

//	increaseContrast(background, 10);
//	GaussianBlur(background, background, Size(3, 3), 1.5, 1.5);
//	medianBlur(background, background, 5);

//	increaseContrast(input, 10);
//	GaussianBlur(input, input, Size(3, 3), 1.5, 1.5);
//	medianBlur(input, input, 5);

	input = input - background; 

//	increaseContrast(input, 10);
//	GaussianBlur(input, input, Size(3, 3), 1.5, 1.5);

//	makeBinary(input, 17);
	threshold(input, input, 15, 255, THRESH_OTSU);

	GaussianBlur(p1_ship, p1_ship, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_ship, p2_ship, Size(3, 3), 1.5, 1.5);
	GaussianBlur(p1_sq1, p1_sq1, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_sq1, p2_sq1, Size(3, 3), 1.5, 1.5);
	GaussianBlur(p1_sq2, p1_sq2, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_sq2, p2_sq2, Size(3, 3), 1.5, 1.5);

	input.copyTo(output);
}


void findObject(Mat objTemplate, string name, double rotation, int team){
	double minMatchValue = team1_minMatchValue;
		if (team == 2) minMatchValue = team2_minMatchValue;

//	mtx.lock();
	Mat result;
	Mat temp; objTemplate.copyTo(temp);

	if (rotation != 0){
		Point2f tempCenter(temp.cols / 2.0, temp.rows / 2.0);
		Mat rot = cv::getRotationMatrix2D(tempCenter, rotation, 0.9);
		warpAffine(temp, temp, rot, temp.size());
	}

	matchTemplate(input, temp, result, CV_TM_CCORR_NORMED);

	double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

	matchLoc = maxLoc;

//	if (maxVal > bestMatch) bestMatch = maxVal;

	if (maxVal > minMatchValue){

		rectangle(output, matchLoc, Point(matchLoc.x + temp.cols, matchLoc.y + temp.rows), Scalar::all(255), 2, 8, 0);
		putText(output, name, Point(matchLoc.x + temp.cols + 5, matchLoc.y + temp.rows - 5), 1, 1, Scalar(255, 0, 0, 255));

		cout << name + " (" + to_string(maxVal) + " percent match) found at: " + to_string(matchLoc.x) + ", " + to_string(matchLoc.y) << " - rotation: " << rotation << endl;
//		findObject(objTemplate, name, 0, team);
	}
	else
	{
		//	cout << "Not found where Rotation = " << rotation << " - Best match: " << maxVal << " - temp.size: " << temp.size() << endl;

		if (rotation < 360){
			rotation += 1;
			findObject(objTemplate, name, rotation, team);
		}
		else
		{
			cout << "Did not find a match for " + name + " - best match: " << bestMatch << endl;
		}
	}
//	mtx.unlock();
}

void findAllOnce(){
	bestMatch = 0;
	findObject(p1_ship, "P1 Ship", 0, 1);
	bestMatch = 0;
	findObject(p1_sq1, "P1 SQ1", 0, 1);
	bestMatch = 0;
	findObject(p1_sq2, "P1 SQ2", 0, 1);

	bestMatch = 0;
	findObject(p2_ship, "P2 Ship", 0, 2);
	bestMatch = 0;
	findObject(p2_sq1, "P2 SQ1", 0, 2);
	bestMatch = 0;
	findObject(p2_sq2, "P2 SQ2", 0, 2);
}

void showOutputLoop(){
	while (true){
		imshow("Output", output);
		if (waitKey(30) > 0) break;
	}
}

void makeBinary(Mat temp, int threshold){
	for (int y = 0; y < temp.rows; y++){
		for (int x = 0; x < temp.cols; x++){
			if (temp.at<uchar>(y, x) <= threshold) temp.at<uchar>(y, x) = 0;
			else temp.at<uchar>(y, x) = 255;
		}
	}

	erode(temp, temp, Mat());
	dilate(temp, temp, Mat());
}

void increaseContrast(Mat temp, int amount){
	for (int y = 0; y < temp.rows; y++){
		for (int x = 0; x < temp.cols; x++)
		{
			if (temp.at<uchar>(y, x) * amount > 255) temp.at<uchar>(y, x) = 255;
			else temp.at<uchar>(y, x) = temp.at<uchar>(y, x) * amount;
		}
	}
}

Mat desktopCapture(Mat input)
{
	// Found this snippet online -- make sure you understand it, censor might ask us wtf it is. 
	// We don't have to be able to explain everything, just what it does.
	HWND desktop = GetDesktopWindow();
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(desktop);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(desktop, &windowsize);

	// SOURCE CAPTURE
	srcheight = windowsize.bottom / 1.1;
	srcwidth = windowsize.right / 1.6;

	// DISPLAY WINDOW
	height = windowsize.bottom / 1.37;
	width = windowsize.right / 2;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, -2, -30, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(desktop, hwindowDC);

	return src;
}
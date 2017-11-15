#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <iostream><
#include <thread>

using namespace cv;
using namespace std;

/* Function Headers */
Mat desktopCapture(Mat input);
void increaseContrast(Mat input, int amount);
void findObject(Mat objTemplate, string name, double rotation);
void findAll();

/* References */
Mat input, output;
Mat result = input.clone();

// Sorte Figurer
Mat p1_ship = imread("p1_ship.PNG", 0); // Kors
Mat p1_sq1 = imread("p1_sq1.PNG", 0);	// Ruder
Mat p1_sq2 = imread("p1_sq2.PNG", 0);	// Cirkel

// Hvide Figurer
Mat p2_ship = imread("p2_ship.PNG", 0);	// Kors
Mat p2_sq1 = imread("p2_sq1.PNG", 0);	// Ruder
Mat p2_sq2 = imread("p2_sq2.PNG", 0);	// Cirkel

double minMatchValue = 0.70;
double bestMatch = 0;

int main(int, char)
{
	Sleep(2000);

	input = desktopCapture(input);
	cvtColor(input, input, CV_RGB2GRAY);

	increaseContrast(input, 4);
	increaseContrast(p1_ship, 4); increaseContrast(p1_sq1, 4); increaseContrast(p1_sq2, 4);
	increaseContrast(p2_ship, 4); increaseContrast(p2_sq1, 4); increaseContrast(p2_sq2, 4);

	output = input.clone();

	findAll();

	waitKey(0);

	return 0;
}

void findObject(Mat objTemplate, string name, double rotation){
	Mat temp;
	objTemplate.copyTo(temp);
	int background = temp.at<uchar>(0, 0);

	if (rotation != 0){
		Point2f tempCenter(temp.cols / 2.0, temp.rows / 2.0);
		Mat rot = cv::getRotationMatrix2D(tempCenter, rotation, 0.9);
		warpAffine(temp, temp, rot, temp.size());
		
		for (int y = 0; y < temp.rows; y++){
			for (int x = 0; x < temp.cols; x++){
				if (temp.at<uchar>(y, x) == 0){
					temp.at<uchar>(y, x) = background;
				}
			}
		}
	}

	matchTemplate(input, temp, result, 5);

	double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

	matchLoc = maxLoc;

	if (maxVal > bestMatch) bestMatch = maxVal;

	if (maxVal > minMatchValue){

		rectangle(output, matchLoc, Point(matchLoc.x + temp.cols, matchLoc.y + temp.rows), Scalar::all(255), 2, 8, 0);
		putText(output, name, Point(matchLoc.x + temp.cols + 5, matchLoc.y + temp.rows - 5), 1, 1, Scalar(255, 0, 0, 255));

		cout << name + " (" + to_string(maxVal) + " percent match) found at: " + to_string(matchLoc.x) + ", " + to_string(matchLoc.y) << " - rotation: " << rotation << endl;
	}
	else 
	{
//		cout << "Not found where Rotation = " << rotation << " - Best match: " << maxVal << " - temp.size: " << temp.size() << endl;

		if (rotation < 360){
			rotation += 1;
			findObject(objTemplate, name, rotation);
		}
		else
		{
			cout << "Did not find a match for " + name + " - best match: " << bestMatch << endl;
		}
	}

	imshow("Output", output);
	imshow("Template", temp);

//	waitKey(0);
}

void findAll(){
	bestMatch = 0;
	findObject(p1_ship, "P1 Ship (Sort Rude)", 0);
	bestMatch = 0;
	findObject(p1_sq1, "P1 SQ1 (Hvid Rude)", 0);
	bestMatch = 0;
	findObject(p1_sq2, "P1 SQ2 (Hvid O)", 0);

	//bestMatch = 0;
	//findObject(p2_ship, "P2 Ship (asd Kors)", 0);
	//bestMatch = 0;
	//findObject(p2_sq1, "P2 SQ1 (Hvid Ruder)", 0);
	//bestMatch = 0;
	//findObject(p2_sq2, "P2 SQ2 (Hvidt O)", 0);
}

void increaseContrast(Mat input, int amount){
	for (int y = 0; y < input.rows; y++){
		for (int x = 0; x < input.cols; x++){
			if (!(input.at<uchar>(y, x) * amount > 255)) input.at<uchar>(y, x) = input.at<uchar>(y, x) * amount;
		}
	}
}

// Found this snippet online -- make sure you understand it, censor might ask us wtf it is. 
// We don't have to be able to explain everything, just what it does.
Mat desktopCapture(Mat input)
{
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
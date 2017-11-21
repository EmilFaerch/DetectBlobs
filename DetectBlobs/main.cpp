#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <iostream>
#include <thread>
#include <SFML\Network.hpp>

using namespace cv;
using namespace std;

/* Function Headers */
// Initializing and Processing ------
Mat desktopCapture(Mat input);
void takeBackground();
void findContours(string subject);
void clientOutput();
void showOutputLoop(); // -----------

// Find objects (Ships/Squadrons) 
//void findObject(Mat objTemplate, string name, double rotation, int team);
//void findAllOnce();


/* References */
Mat input, background, output;
/* No more template matching ------------
Mat p1_ship = imread("p1_ship.PNG", 0); 
Mat p1_sq1 = imread("p1_sq1.PNG", 0);
Mat p1_sq2 = imread("p1_sq2.PNG", 0);

Mat p2_ship = imread("p2_ship.PNG", 0);
Mat p2_sq1 = imread("p2_sq1.PNG", 0);
Mat p2_sq2 = imread("p2_sq2.PNG", 0);

double team1_minMatchValue = 0.75;
double team2_minMatchValue = 0.7;
double bestMatch = 0;
--------------------------------------- */

double p1_shipAmin = 5900, p1_shipAmax = 6200;
double p1_sq1Amin = 950, p1_sq1Amax = 1050;
double p1_sq2Amin = 620, p1_sq2Amax = 650;

double p2_shipAmin = 4250, p2_shipAmax = 4370;
double p2_sq1Amin = 400, p2_sq1Amax = 500;
double p2_sq2Amin = 130, p2_sq2Amax = 190;

int main(int, char)
{
	input = imread("input image.PNG", 0);
	output = Mat::zeros(input.rows, input.cols, CV_8UC1);
	thread threadOutput(showOutputLoop);

	// -v- Uncomment -----^ testing purposes
	//Sleep(2000);
	//takeBackground();
	//thread threadOutput(showOutputLoop);

	findContours("regular - light: ");

	input = imread("input image rotated.PNG", 0);

	findContours("rotated - dark: ");

	clientOutput();

/*
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
*/

//	waitKey(0); // Needed for when we're multithreading

/*
	p1_ship.join();
	p1_sq1.join();
	p1_sq2.join();

	p2_ship.join();
	p2_sq1.join();
	p2_sq2.join();
	*/

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

	GaussianBlur(background, background, Size(3, 3), 1.5, 1.5);
	GaussianBlur(input, input, Size(3, 3), 1.5, 1.5);
//	medianBlur(input, input, 5);

	input = input - background; 

	erode(input, input, Mat());
	dilate(input, input, Mat());

	threshold(input, input, 15, 255, THRESH_OTSU);

//	imwrite("input image rotated.png", input);

	//GaussianBlur(p1_ship, p1_ship, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_ship, p2_ship, Size(3, 3), 1.5, 1.5);
	//GaussianBlur(p1_sq1, p1_sq1, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_sq1, p2_sq1, Size(3, 3), 1.5, 1.5);
	//GaussianBlur(p1_sq2, p1_sq2, Size(3, 3), 1.5, 1.5); GaussianBlur(p2_sq2, p2_sq2, Size(3, 3), 1.5, 1.5);

	input.copyTo(output);
}

void findContours(string subject){
	/// Find contours
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(input, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	vector<Rect> boundRect(contours.size());

	cout << subject << endl;
	for (int i = 0; i < contours.size(); i++){
		boundRect[i] = boundingRect(Mat(contours[i]));
		int area = contourArea(contours[i]);

		Point center;
		center.x = int(boundRect[i].x);
		center.y = int(boundRect[i].y) - 5;

		drawContours(output, contours, i, Scalar::all(255));
		rectangle(output, boundRect[i].tl(), boundRect[i].br(), Scalar::all(200), 2, 8, 0);

		if (area > p1_shipAmin && area < p1_shipAmax){
			putText(output, "P1 Ship", center, 1, 1, Scalar::all(200));
		}
		else if (area > p1_sq1Amin && area < p1_sq1Amax){
			putText(output, "P1 SQ1", center, 1, 1, Scalar::all(200));
		}
		else if (area > p1_sq2Amin && area < p1_sq2Amax){
			putText(output, "P1 SQ2", center, 1, 1, Scalar::all(200));
		}

		else if (area > p2_shipAmin && area < p2_shipAmax){
			putText(output, "P2 Ship", center, 1, 1, Scalar::all(200));
		}
		else if (area > p2_sq1Amin && area < p2_sq1Amax){
			putText(output, "P2 SQ1", center, 1, 1, Scalar::all(200));
		}
		else if (area > p2_sq2Amin && area < p2_sq2Amax){
			putText(output, "P2 SQ2", center, 1, 1, Scalar::all(200));
		}
		else{
			cout << "Unidentified object at " << boundRect[i].x + (boundRect[i].width / 2) << ", " << boundRect[i].y + (boundRect[i].height / 2) << " - Area: " << area << endl;
			putText(output, "Unidentified", center, 1, 1, Scalar::all(200));
		}
	}
}

// For template matching; not used anymore
/*
void findObject(Mat objTemplate, string name, double rotation, int team){
	double minMatchValue = team1_minMatchValue;
		if (team == 2) minMatchValue = team2_minMatchValue;

	Mat result;
	Mat temp; objTemplate.copyTo(temp);

	if (rotation != 0){
		Point2f tempCenter(temp.cols / 2.0, temp.rows / 2.0);
		Mat rot = cv::getRotationMatrix2D(tempCenter, rotation, 0.9);
		warpAffine(temp, temp, rot, temp.size());
	}

	matchTemplate(input, temp, result, 5);

	double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

	matchLoc = maxLoc;

	if (maxVal > minMatchValue){

		rectangle(output, matchLoc, Point(matchLoc.x + temp.cols, matchLoc.y + temp.rows), Scalar::all(255), 2, 8, 0);
		putText(output, name, Point(matchLoc.x + temp.cols + 5, matchLoc.y + temp.rows - 5), 1, 1, Scalar(255, 0, 0, 255));

		cout << name + " (" + to_string(maxVal) + " percent match) found at: " + to_string(matchLoc.x) + ", " + to_string(matchLoc.y) << " - rotation: " << rotation << endl;
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
			cout << "Did not find a match for " + name + "."<< endl;
		}
	}
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
*/

void showOutputLoop(){
	while (true){
		imshow("Output", output);
		if (waitKey(30) > 0) break;
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

void clientOutput() {

	int PositionX = 5;
	int PositionY = 10;
	int prevPostionX, prevPostionY;

	string posX = std::to_string(PositionX);
	string posY = std::to_string(PositionY);
	string space = " ";


	char buffer[2000];
	sf::IpAddress myIP = sf::IpAddress::getLocalAddress();
	sf::TcpSocket socket;
	cout << myIP << endl;
	std::string text = "connected to client";
	socket.connect(myIP, 5000);
	std::size_t received;

	sf::Packet packet;

	socket.setBlocking(false);

	while (1) {

		prevPostionX = PositionX;
		prevPostionY = PositionY;

		string open = posX + space + posY;

		try{
		packet << open;
		socket.send(packet);

		if (prevPostionX != PositionX && prevPostionY != PositionY) {}

			socket.receive(packet);

			socket.send(text.c_str(), text.length() + 1);
			socket.receive(buffer, sizeof(buffer), received);
			std::cout << buffer << std::endl;
		}
		catch (exception e)
		{}



	}
	system("pause");
}

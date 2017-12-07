#include <Windows.h>
#include <thread>
#include <iostream>
#include <SFML\Network.hpp>
#include <SFML\Window.hpp>
#include <SFML\System.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace cv;
using namespace std;

/* Function Headers */
Mat desktopCapture(Mat input);
void takeBackground();
void updateInput();
void findObjects(string subject);
string scanCommand();
void startServer();
bool clientConnected();
void clientSendInfo(int PositionX, int PositionY, string object, int rotation);
void showOutputLoop(); 
void buttonScan();
void setButtonCoords();

// Threads for sending Ship info to client
thread threadSearch, t1, t2, t3, t4, t5, t6;

/* NETWORKING */
sf::IpAddress myIP = "127.0.0.1";
int port = 5000;
sf::TcpListener listener;
sf::TcpSocket someSocket;
bool listening = true, connected = false;

/* Materials */
Mat input, background, blobs, output;

// Grid Area for Unity Offset --------------------------------------------
// X
int areaX1 = 250, areaX2 = 375, areaX3 = 0, areaX4 = 500, areaX5 = 625;
int offX1 = -160, offX2 = -80, offX3 = 0, offX4 = 75, offX5 = 140;
// Y
int areaY1 = 160, areaY2 = 280, areaY3 = 400, areaY4 = 520, areaY5 = 625;
int offY1 = -120, offY2 = -90, offY3 = -25, offY4 = 85, offY5 = 140;
// -------------------------------------------------------------------------

// Object features; only the squads have circularity, cause we use area for the ships
int squadArea = 2500; // BLOBs above this Area are Ships
double p1_sq1CircMin = 0.50, p1_sq1CircMax = 0.65;
double p1_sq2CircMin = 0.80, p1_sq2CircMax = 0.90;
double p2_sq1CircMin = 0.30, p2_sq1CircMax = 0.45;
double p2_sq2CircMin = 0.46, p2_sq2CircMax = 0.80;
// -------------------------------------------------------

// Buttons -----------------------------------------------------------
int offsetX = 30; int scanY = 450; int btnLength = 60;
Point scanStart, scanStop; bool scanEnabled = false;
Point btnLeftStart, btnLeftStop;  bool leftEnabled = false;
Point btnRightStart, btnRightStop;  bool rightEnabled = false;
Point btnConfirmStart, btnConfirmStop;  bool confirmEnabled = false;
Point btnCancelStart, btnCancelStop;  bool cancelEnabled = false;
// --------------------------------------------------------------------

bool player1 = true;

int main(int, char)
{
	takeBackground();
	setButtonCoords();

	output = Mat::zeros(background.rows, background.cols, CV_8UC1);
	blobs = Mat::zeros(background.rows, background.cols - ((offsetX + btnLength) * 2), CV_8UC1);

	thread serverConnection(startServer);
	thread updateImage(updateInput);
	thread threadOutput(showOutputLoop);

	waitKey(0); // Needed for multithreading, so the program doesn't close / crash (on exit)

	serverConnection.join();
	updateImage.join();
	threadOutput.join();
	threadSearch.join();

	return 0;
}

void takeBackground(){
//	background = imread("bg.PNG", 0);
	Sleep(2000);
	background = desktopCapture(background);
	cvtColor(background, background, CV_RGB2GRAY);
	GaussianBlur(background, background, Size(3, 3), 1.5, 1.5);

	imwrite("bg.PNG", background);
	imshow("[BG] Press Space to continue", background);
	waitKey(0);
}

void updateInput(){
	//input = imread("input image.png", 0);
	//imwrite("input image.png", input);

	//cout << "Done updating input picture!" << endl;

	while (true){
		waitKey(1000);

		input = desktopCapture(input);
		cvtColor(input, input, CV_RGB2GRAY);

		GaussianBlur(input, input, Size(3, 3), 1.5, 1.5);

		input = input - background;

		erode(input, input, Mat());
		dilate(input, input, Mat());

		threshold(input, input, 15, 255, THRESH_OTSU);

		input.copyTo(output);

		int val = offsetX + btnLength;
		
		for (int y = 0; y < blobs.rows; y++){
			for (int x = val; x < blobs.cols; x++){
				blobs.at<uchar>(y, x - val) = input.at<uchar>(y, x);
			}
		}

		imshow("Blobs", blobs);

		buttonScan();
	}
}

void findObjects(string subject){

	if (!clientConnected) return; // Avoid recursion if we're not connected

	cout << subject << endl;

	bool usingT1 = false, usingT2 = false, usingT3 = false, usingT4 = false, usingT5 = false, usingT6 = false;

	try{
		/// Find contours
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(blobs, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
		vector<Rect> boundRect(contours.size());
		vector<vector<Point> > hull(contours.size());

		for (int i = 0; i < contours.size(); i++){
			double contArea = contourArea(contours[i]);
			convexHull(contours[i], hull[i]);
			double hullArea = contourArea(hull[i]);
			if (contArea > 150){

				boundRect[i] = boundingRect(Mat(contours[i]));
				double perimeter = arcLength(contours[i], true);
				double circularity = (4 * M_PI * contArea) / pow(perimeter, 2);

				double convexRatio = contArea / hullArea;
				bool convex = false;

				if (convexRatio >= 0.9) convex = true;

				Point center;
				center.x = boundRect[i].x + (boundRect[i].width / 2);
				center.y = boundRect[i].y + (boundRect[i].height / 2);

				int unityX, unityY; // Offset coordinates for Unity

				if		(center.x >= areaX5) unityX = center.x + offX5;							// [-|-|-|-|x]
				else if (center.x > areaX4 && center.x < areaX5) unityX = center.x + offX4;		// [-|-|-|x|-]
				else if (center.x > areaX2 && center.x < areaX4) unityX = center.x + offX3;		// [-|-|x|-|-]
				else if (center.x < areaX2 && center.x > areaX1) unityX = center.x + offX2;		// [-|x|-|-|-]
				else if (center.x <= areaX1) unityX = center.x + offX1;							// [x|-|-|-|-]

				if (center.y >= areaY5) unityY = center.y + offY5;								// [-|-|x|-|-]
				else if (center.y > areaY4 && center.y < areaY5) unityY = center.y + offY4;		// [-|-|x|-|-]
				else if (center.y > areaY2 && center.y < areaY4) unityY = center.y + offY3;		// [-|-|x|-|-]
				else if (center.y < areaY2 && center.y > areaY1) unityY = center.y + offY2;		// [-|-|x|-|-]
				else if (center.y <= areaY1) unityY = center.y + offY1;							// [-|-|x|-|-]

				drawContours(blobs, contours, i, Scalar::all(255), CV_FILLED);
				rectangle(output, boundRect[i].tl(), boundRect[i].br(), Scalar::all(200), 2, 8, 0);
				circle(output, center, 10, Scalar::all(0));

				double boundAreal = float(boundRect[i].height) * float(boundRect[i].width);

				cout << " - " << endl;
								
				// P1 OBJECTS
				if (convex && squadArea < contArea){
					if (!usingT1){
							putText(output, "P1 Ship", boundRect[i].tl(), 1, 1, Scalar::all(200));
							cout << "P1 Ship found at: " << unityX << ", " << unityY << " - " << circularity << endl;
							
							t1 = thread(clientSendInfo, unityX, unityY, "P1_Ship", 0);
							usingT1 = true;
						}
						else
						{
							cout << "Found duplicate of P1 Ship" << endl;
						}
					}
				else if (convex && circularity > p1_sq1CircMin && circularity < p1_sq1CircMax){
					if (!usingT2){
						putText(output, "P1 SQ1", boundRect[i].tl(), 1, 1, Scalar::all(200));
						t2 = thread(clientSendInfo, unityX, unityY, "P1_SQ1", 0);
						cout << "P1 SQ1 found at: " << unityX << ", " << unityY << " - " << circularity << endl;
						usingT2 = true;
					}
					else
					{
						cout << "Found duplicate of P1 SQ1, assuming it is P1 SQ2 ..." << endl;
						usingT3 = true;
						t3 = thread(clientSendInfo, unityX, unityY, "P1_SQ2", 0);
					}
				}
				else if (convex && circularity > p1_sq2CircMin && circularity < p1_sq2CircMax){
					if (!usingT3){
						putText(output, "P1 SQ2", boundRect[i].tl(), 1, 1, Scalar::all(200));
						t3 = thread(clientSendInfo, unityX, unityY, "P1_SQ2", 0);
						cout << "P1 SQ2 found at: " << unityX << ", " << unityY << " - " << circularity << endl;
						usingT3 = true;
					}
					else
					{
						if (!usingT2){
							cout << "Found duplicate of P1 SQ2, assuming it's P1 SQ1 ..." << endl;
							t2 = thread(clientSendInfo, unityX, unityY, "P1_SQ1", 0);
							usingT2 = true;
						}
						else
						{
							cout << "Tried assuming it was a duplicate of P1 SQ2, but P1 SQ2 has already been found." << endl;
						}
					}
				}
				// P2 OBJECTS
				else if (!convex && squadArea < contArea){
					if (!usingT4){
						putText(output, "P2 Ship", boundRect[i].tl(), 1, 1, Scalar::all(200));
						cout << "P2 Ship found at: " << unityX << ", " << unityY << " - " << circularity << endl;
						
						t4 = thread(clientSendInfo, unityX, unityY, "P2_Ship", 0);
						usingT4 = true;
					}
					else
					{
						cout << "Found duplicate of P2 Ship (this REALLY shouldn't happen)." << endl;
					}
				}
				else if (!convex && circularity > p2_sq1CircMin && circularity < p2_sq1CircMax){
					if (!usingT5){
						putText(output, "P2 SQ1", boundRect[i].tl(), 1, 1, Scalar::all(200));
						t5 = thread(clientSendInfo, unityX, unityY, "P2_SQ1", 0);
						cout << "P2 SQ1 found at: " << unityX << ", " << unityY << " - " << circularity << endl;
						usingT5 = true;
					}
					else
					{
						if (!usingT6){
							cout << "Found duplicate of P2 SQ1, assuming it's P2 SQ2 ..." << endl;
							t6 = thread(clientSendInfo, unityX, unityY, "P2_SQ2", 0);
							usingT6 = true;
						}
						else
						{
							cout << "Tried assuming it was a duplicate of P2 SQ2, but P2 SQ2 has already been found." << endl;
						}
					}
				}
				else if (!convex && circularity > p2_sq2CircMin && circularity < p2_sq2CircMax){
					if (!usingT6){
						putText(output, "P2 SQ2", boundRect[i].tl(), 1, 1, Scalar::all(200));
						t6 = thread(clientSendInfo, unityX, unityY, "P2_SQ2", 0);
						cout << "P2 SQ2 found at: " << unityX << ", " << unityY << " - " << circularity << endl;
						usingT6 = true;
					}
					else
					{
						if (!usingT5){
							cout << "Found duplicate of P2 SQ2, assuming it's P2 SQ1 ..." << endl;
							t5 = thread(clientSendInfo, unityX, unityY, "P2_SQ1", 0);
							usingT5 = true;
						}
						else
						{
							cout << "Tried assuming it was a duplicate of P2 SQ1, but P2 SQ1 has already been found." << endl;
						}
					}
				}
				else
				{
					cout << "Unidentified object at " << boundRect[i].x + (boundRect[i].width / 2) << ", " << boundRect[i].y + (boundRect[i].height / 2) << " - contArea: " << contArea << endl;
					putText(output, "Unidentified", boundRect[i].tl(), 1, 1, Scalar::all(200));
				}
				
				cout << "Circularity: " << circularity << " Blob Area : " << contArea << " - Hull Area : " 
					<< hullArea << " - Ratio : " << convexRatio << " - Convex: " << convex << endl;
			}

			cout << " - " << endl; // To clearly seperate the entries in the console = better overview
		}

//			cout << "For loop ended!" << endl;												// Debugging ...
//			cout << usingT1 << usingT2 << usingT3 << usingT4 << usingT5 << usingT6 << endl; // ... purposes

			if (usingT1) { t1.join(); }
			if (usingT2) { t2.join(); }
			if (usingT3) { t3.join(); }
			if (usingT4) { t4.join(); }
			if (usingT5) { t5.join(); }
			if (usingT6) { t6.join(); }
		}
	catch (Exception e){
		cout << "Error trying to send info!" << endl;
		clientSendInfo(0, 0, "Request failed", 0);
		return;
	}

	waitKey(1000);
	findObjects("Recursion"); // Call itself again to create a Loop

	//if (subject == "Server request"){
	//	cout << "Request done!" << endl;
	//	clientSendInfo(0, 0, "Request done", 0);
	//}
}

void startServer(){
	char buffer[10000];
	size_t received;

	while (listening){
		listener.listen(port, myIP);
		cout << "Waiting for client ... " << endl;
		listener.setBlocking(true);	// Går først videre når den connecter/receiver
		listener.accept(someSocket);
		cout << "Client connected!" << endl;
		connected = true;
		someSocket.setBlocking(false); // Prøver at receive hele tiden
		threadSearch = thread(findObjects, "First Call");

		while (connected){
		//	listener.setBlocking(true); // added det her
				someSocket.receive(buffer, sizeof(buffer), received);

				if (received > 0){
					string msg = buffer;

					// 0 - 7 er fixed fordi vi ved at "Update!" har syv bogstaver, og vi gør det for at filtrere noise;
					if (msg.substr(0, 7) == "Update!") findObjects("Server request"); //  data fra packets osv, det er lidt kompliceret fra C# til C++

					else cout << "Client: " << msg << endl;


					received = 0;
			}
				/*if (!clientConnected()){
					cout << "No client connected." << endl;
				}*/
		}
	}
}

bool clientConnected(){
	if (someSocket.getRemoteAddress() == "None" || someSocket.getRemoteAddress() == "0.0.0.0"){
		connected = false;
		someSocket.disconnect();

		cout << "#--- No client connected ---#" << endl;
		return false;
	}
	else
	{
		return true;
	}
}

void buttonScan(){
	int minBlobArea = 100;

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	Mat scanROI = Mat::zeros(background.rows, btnLength, CV_8UC1);

	// Make sure to run setButtonCoords before this is run
	for (int y = 0; y < background.rows; y++){
		for (int x = offsetX; x < offsetX + btnLength; x++){
			scanROI.at<uchar>(y, x - offsetX) = input.at<uchar>(y, x);
		}
	}

	findContours(scanROI, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	if (contours.size() > 0){
		for (int i = 0; i < contours.size(); i++){
			int area = contourArea(contours[i]);
			if (area > minBlobArea){ // Small BLOBs might be created due to "erode/dilate", so we make sure they are not counted
				drawContours(blobs, contours, i, Scalar::all(255), CV_FILLED);

				vector<Rect> boundRect(contours.size());
				boundRect[i] = boundingRect(Mat(contours[i]));

				Point center;
				center.x = boundRect[i].x;
				center.y = boundRect[i].y;

				if (center.y >= btnLeftStart.y && center.y <= btnLeftStop.y){
					cout << "Button clicked: Left" << endl;
					clientSendInfo(0, 0, "Button Left", 0);
				}
				else if (center.y >= btnRightStart.y && center.y <= btnRightStop.y){
					cout << "Button clicked: Right" << endl;
					clientSendInfo(0, 0, "Button Right", 0);
				}
				else if (center.y >= btnConfirmStart.y && center.y <= btnConfirmStop.y){
					cout << "Button clicked: Confirm" << endl;
					clientSendInfo(0, 0, "Button Confirm", 0);
				}
				else if (center.y >= btnCancelStart.y && center.y <= btnCancelStop.y){
					cout << "Button clicked: Cancel" << endl;
					clientSendInfo(0, 0, "Button Cancel", 0);
				}
				else if (center.y >= scanStart.y && center.y <= scanStop.y){
					cout << "Scan recognized ..." << endl;
					clientSendInfo(0, 0, "Scan " + scanCommand(), 0);
				}
			}
		}
	}
}

string scanCommand(){
	string command; 
	int foundBlobs = 0;
	int minBlobArea = 60;

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	Mat scanROI = Mat::zeros(scanStop.y - scanStart.y, scanStop.x - scanStart.x, CV_8UC1);
	
	for (int y = scanStart.y; y < scanStop.y; y++){
		for (int x = scanStart.x; x < scanStop.x; x++){
			scanROI.at<uchar>(y - scanStart.y, x - scanStart.x) = input.at<uchar>(y, x);
		}
	}

	findContours(scanROI, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	cout << "Contours Found: " << to_string(contours.size()) << endl;

	if (contours.size() > 0){
		for (int i = 0; i < contours.size(); i++){
		
			if (contourArea(contours[i]) > minBlobArea){ // Small BLOBs might be created due to "erode/dilate", so we make sure they are not counted
				foundBlobs++;
				drawContours(blobs, contours, i, Scalar::all(255), CV_FILLED);
			}
		}
		
		if (foundBlobs == 1) command = "Speed";
		else if (foundBlobs == 2) command = "Repair";
		else if (foundBlobs == 3) command = "Concentrate";
		else { cout << "Found " << foundBlobs << " BLOBs - Please adjust the card and try again." << endl; command = "Failed"; }
	}
	else
	{
		cout << "Could not recognize any BLOBs in the area - Please adjust the card and try again." << endl;
		command = "Failed";
	}

	cout << "Command: " << command << endl;

	return command;
}

void clientSendInfo(int PositionX, int PositionY, string object, int rotation) {
	string text;
	string posX = to_string(PositionX);
	string posY = to_string(PositionY);
	string direction = to_string(rotation);

	if (clientConnected()){
		text = (object + " " + posX + " " + posY + " " + direction);
		someSocket.send(text.c_str(), text.length() + 1);
	}
	else
	{
		cout << "Client is not connected." << endl;
	}
}

void setButtonCoords(){
	// Y-values don't change ...
	scanStart.y = scanY;
	btnLeftStart.y = offsetX * 1 + btnLength * 1;
	btnRightStart.y = offsetX * 2 + btnLength * 2;
	btnConfirmStart.y = offsetX * 3 + btnLength * 3;
	btnCancelStart.y = offsetX * 4 + btnLength * 4;

	scanStop.y = scanStart.y + btnLength;
	btnLeftStop.y = btnLeftStart.y + btnLength;
	btnRightStop.y = btnRightStart.y + btnLength;
	btnConfirmStop.y = btnConfirmStart.y + btnLength;
	btnCancelStop.y = btnCancelStart.y + btnLength;

	// ... only the X coordinates do
	if (player1){ // Left side of the table
		scanStart.x = offsetX;
		btnLeftStart.x = offsetX;		
		btnRightStart.x = offsetX;		
		btnConfirmStart.x = offsetX;	
		btnCancelStart.x = offsetX;		
	}
	else // Right side of the table
	{
		scanStart.x = background.cols - offsetX;

		btnLeftStart.x = background.cols - offsetX - btnLength;	
		btnRightStart.x = background.cols - offsetX - btnLength;
		btnConfirmStart.x = background.cols - offsetX - btnLength;
		btnCancelStart.x = background.cols - offsetX - btnLength;
	}

	// The _Start.x coordinates determine where the _Stop.x coordinates are, no extra calculation needed
	scanStop.x = scanStart.x + btnLength;
	btnLeftStop.x = btnLeftStart.x + btnLength;
	btnRightStop.x = btnRightStart.x + btnLength;
	btnConfirmStop.x = btnConfirmStart.x + btnLength;
	btnCancelStop.x = btnCancelStart.x + btnLength;
}

void showOutputLoop(){
	while (true){

		/*for (int i = 1; i < 6; i++){
			line(output, Point(i * 125, 0), Point(i * 125, output.rows), Scalar::all(20), 3);
		}
		// ---------- ^ GRID v --------------
		for (int i = 1; i < 6; i++){
			line(output, Point(0, (i * 120) - 80), Point(output.cols, (i * 120) - 80), Scalar::all(20), 3);
		}*/
		
		rectangle(output, Point(scanStart), Point(scanStop), Scalar::all(60), 2, 8, 0);
		rectangle(output, Point(btnLeftStart), Point(btnLeftStop), Scalar::all(60), 2, 8, 0);
		rectangle(output, Point(btnRightStart), Point(btnRightStop), Scalar::all(60), 2, 8, 0);
		rectangle(output, Point(btnCancelStart), Point(btnCancelStop), Scalar::all(60), 2, 8, 0);
		rectangle(output, Point(btnConfirmStart), Point(btnConfirmStop), Scalar::all(60), 2, 8, 0);

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
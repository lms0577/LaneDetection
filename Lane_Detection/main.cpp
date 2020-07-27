#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>
#include <time.h>

class LaneDetection
{
private:
	double width = 0;
	double height = 0;

public:
	// Constructor
	LaneDetection(double width, double height)
	{
		this->width = width;
		this->height = height;
	}

	// Member Function
	double getWidth()
	{
		return width;
	}

	double getHeight()
	{
		return height;
	}

	// return grayscale image of white and yellow
	cv::Mat yellowFilter(const cv::Mat &img, const int &lower_hue, const int &upper_hue)
	{
		// Declare Variables
		cv::Mat hsv_img, yellow_mask, hsv_value;
	    std::vector<cv::Mat> hsv_split;

		// Convert BGR to HSV
		cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);

		// Split hsv_image to H, S, V
		cv::split(hsv_img, hsv_split);

		// Set Yellow Range
		cv::Scalar lower_yellow(lower_hue, 100, 100);
		cv::Scalar upper_yellow(upper_hue, 255, 200);

		hsv_value = hsv_split[2];
		cv::inRange(hsv_img, lower_yellow, upper_yellow, yellow_mask);
	
		return hsv_value;
	}

};

void on_trackbar(int, void*) {

}

int main(int argc, char** argv)
{
	// Confirm OpenCV Version
	std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

	// Open Video File
	cv::VideoCapture cap("..\\Demo_Video\\Demo_Video.mp4");
	if (!cap.isOpened())
	{
		std::cerr << "Fail Read Video\n";
		return -1;
	}

	// Declare Variable
	cv::Mat img, img_gray, img_aBinary, img_gaussian, img_aBinary_gaussian, img_cali, img_pers,
		img_sobelx;

	// Get Video width x height
	double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

	// Declare LaneDetection Class
	//LaneDetection laneDetection(width, height);

	//std::cout << laneDetection.getWidth() << " " << laneDetection.getHeight() << std::endl;

	// Set NameWindow
	cv::namedWindow("Adaptive Binarization Image");

	// Create Trackbar
	cv::createTrackbar("Block Size", "Adaptive Binarization Image", 0, 200, on_trackbar);
	cv::setTrackbarPos("Block Size", "Adaptive Binarization Image", 21);

	// Camera Calibration
	cv::Mat mtx = (cv::Mat1d(3, 3) << 375.02024751, 0., 316.52572289, 0., 490.14999206, 288.56330145, 0., 0., 1.);
	cv::Mat dist = (cv::Mat1d(1, 5) << -0.30130634, 0.09320542, -0.00809047, 0.00165312, -0.00639115);
	cv::Mat newcameramtx = (cv::Mat1d(3, 3) << 273.75825806, 0., 318.4331204, 0., 391.74940796, 283.77532838, 0., 0., 1.);
	cv::Mat mapx, mapy;
	cv::initUndistortRectifyMap(mtx, dist, cv::Mat(), newcameramtx, cv::Size(width, height), CV_32FC1, mapx, mapy);

	// Perspective Transform
	std::vector<cv::Point2f> srcQuad = { cv::Point2f(0.4 * width, 0.45 * height), cv::Point2f(0.6 * width, 0.45 * height), cv::Point2f(0.0, height), cv::Point2f(width, height) };
	std::vector<cv::Point2f> dstQuad = { cv::Point2f(0.27 * width, 0), cv::Point2f(0.65 * width, 0), cv::Point2f(0.27 * width, height), cv::Point2f(0.73 * width, height) };
	// Get Perspective Transform Matrix
	cv::Mat pers = cv::getPerspectiveTransform(srcQuad, dstQuad);

	// Time Variables
	clock_t start, end, start_01, end_01, start_02, end_02, start_03, end_03;

	while (1)
	{
		// Start Time
		start = clock();

		// Read 1 frame from Video File
		cap.read(img);
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}

		// Show Video Image
		cv::imshow("Original Image", img);

		// Convert BGR to Gray
		cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
		cv::imshow("Gray Image", img_gray);

		// Gaussian Filter
		double sigma = 2;
		cv::GaussianBlur(img_gray, img_gaussian, cv::Size(), sigma);

		// Adaptive Binarization ------------------------------
		// Get Block Size
		int bsize = cv::getTrackbarPos("Block Size", "Adaptive Binarization Image");
		if (bsize % 2 == 0) bsize--;
		if (bsize < 3) bsize = 3;

		// adaptiveTreshold Function
		cv::adaptiveThreshold(img_gaussian, img_aBinary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, bsize, 3);
		cv::adaptiveThreshold(img_gaussian, img_aBinary_gaussian, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 3);
		cv::imshow("Adaptive Binarization Image", img_aBinary);
		cv::imshow("Adaptive Binarization Image of Gaussian", img_aBinary_gaussian);

		// Camera Calibration----------------------------------
		cv::remap(img_aBinary_gaussian, img_cali, mapx, mapy, cv::INTER_LINEAR);
		cv::imshow("Camera Calibration Image", img_cali);

		// Perspective Transform-------------------------------
		cv::warpPerspective(img_cali, img_pers, pers, cv::Size());
		cv::imshow("Perspective Transform Image", img_pers);

		// dx gradiant by sobel mask (세로 선 찾기 위한 마스크)
		// x방향의 변화율은 세로줄이다
		cv::Sobel(img_pers, img_sobelx, CV_64F, 1, 0);
		//cv::convertScaleAbs(img_sobelx, img_sobelx);
		img_sobelx.convertTo(img_sobelx, CV_8UC1);
		cv::imshow("Sobel Image", img_sobelx);

		cv::Mat img_sobel = img_sobelx > 200;
		cv::imshow("sobel Image test", img_sobel);

		// Lane Detection--------------------------------------
		cv::Mat img_copy = img_sobel.clone();
		start_01 = clock();
		// Find Right Lane
		cv::Rect right_01(360, 450, 200, 20);
		cv::Rect right_02(360, 430, 200, 20);
		cv::Rect right_03(360, 410, 200, 20);
		cv::Rect right_04(360, 390, 200, 20);
		//cv::Rect left_01(130, 460, 150, 10);
		//cv::Rect left_02(130, 440, 150, 10);
		cv::rectangle(img_pers, right_01, cv::Scalar(255), 1);
		cv::rectangle(img_pers, right_02, cv::Scalar(255), 1);
		cv::rectangle(img_pers, right_03, cv::Scalar(255), 1);
		cv::rectangle(img_pers, right_04, cv::Scalar(255), 1);
		//cv::rectangle(img_pers, left_01, cv::Scalar(255), 1);
		//cv::rectangle(img_pers, left_02, cv::Scalar(255), 1);
		cv::imshow("Perspective Transform Image", img_pers);
		// 4개의 영역에 대해서 열만 합해서 따로 행렬을 만든다.
		
		// Variables
		cv::Mat right_01_colSum, right_02_colSum, left_01_colSum, left_02_colSum;
		std::vector<cv::Point> right_point;

		// Find Right Lane Point
		for (int i = 0; i < 4; i++)
		{
			int max = 0;
			int x = 360, y = 450 - (i * 20);
			cv::Rect right_roi(x, y, 200, 20);
			std::cout << "x: " << x << " y: " << y << std::endl;

			for (int j = 0; j < 200; j++)
			{
				int sum = (int)cv::sum(img_copy(right_roi).col(j))[0];
				if (sum > 600)
				{
					if (sum > max)
					{
						max = sum;
					}
					else
					{
						right_point.push_back(cv::Point((x + (j + 1)), (y + 10)));
						break;
					}
				}
			}
		}
		// Draw Right Lane
		std::cout << right_point << std::endl;
		cv::polylines(img_copy, right_point, false, cv::Scalar(255), 5);
		cv::imshow("Lane Image", img_copy);
		//std::cout << right_01_colSum << std::endl;
		//std::cout << right_02_colSum << std::endl;
		//std::cout << left_01_colSum << std::endl;
		//std::cout << left_02_colSum << std::endl;
		end_01 = clock();

		// End Time
		end = clock();
		std::cout << "Total: " << 1000 / (double)(end - start) << "FPS, Lane: " << (double)(end_01 - start_01) << "ms" << std::endl;

		// WaitKey
		int key = cv::waitKey(1);
		if (key >= 0)
		{
			// Print Key
			std::cout << "Key Value: " << key << std::endl;
			// Release Resources
			cap.release();
			cv::destroyAllWindows();
			break;
		}
	}

	return 0;
}
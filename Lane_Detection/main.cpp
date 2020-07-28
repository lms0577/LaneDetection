#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>

/*
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
*/

int main(int argc, char** argv)
{
	// Confirm OpenCV Version --------------------------------------------
	std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

	// Open Video File ----------------------------------------------------
	cv::VideoCapture cap("..\\Demo_Video\\Demo_Video.mp4");
	if (!cap.isOpened())
	{
		std::cerr << "Fail Read Video\n";
		return -1;
	}

	// Declare Variable -------------------------------------------------------
	cv::Mat img, img_gray, img_aBinary, img_gaussian, img_aBinary_gaussian, img_hsv, 
		img_lab, img_binary_gaussian, img_no_light;

	// Get Video width x height ----------------------------------------------
	double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

	// Declare LaneDetection Class
	//LaneDetection laneDetection(width, height);
	//std::cout << laneDetection.getWidth() << " " << laneDetection.getHeight() << std::endl;

	// Camera Calibration --------------------------------------------
	cv::Mat mtx = (cv::Mat1d(3, 3) << 375.02024751, 0., 316.52572289, 0., 490.14999206, 288.56330145, 0., 0., 1.);
	cv::Mat dist = (cv::Mat1d(1, 5) << -0.30130634, 0.09320542, -0.00809047, 0.00165312, -0.00639115);
	cv::Mat newcameramtx = (cv::Mat1d(3, 3) << 273.75825806, 0., 318.4331204, 0., 391.74940796, 283.77532838, 0., 0., 1.);
	cv::Mat mapx, mapy;
	cv::initUndistortRectifyMap(mtx, dist, cv::Mat(), newcameramtx, cv::Size(width, height), CV_32FC1, mapx, mapy);

	// Perspective Transform ---------------------------------------------------------
	std::vector<cv::Point2f> srcQuad = { cv::Point2f(0.4 * width, 0.45 * height), cv::Point2f(0.6 * width, 0.45 * height), cv::Point2f(0.0, height), cv::Point2f(width, height) };
	std::vector<cv::Point2f> dstQuad = { cv::Point2f(0.27 * width, 0), cv::Point2f(0.65 * width, 0), cv::Point2f(0.27 * width, height), cv::Point2f(0.73 * width, height) };
	// Get Perspective Transform Matrix
	cv::Mat pers = cv::getPerspectiveTransform(srcQuad, dstQuad);

	// Set Trackbar -----------------------------------------------------------------
	// Set NameWindow
	//cv::namedWindow("Adaptive Binarization Image");
	cv::namedWindow("No Lightness Image");
	cv::namedWindow("Binary Image");

	// Create Trackbar
	//cv::createTrackbar("Block Size", "Adaptive Binarization Image", 0, 200, on_trackbar);
	//cv::setTrackbarPos("Block Size", "Adaptive Binarization Image", 11);
	cv::createTrackbar("K Size", "No Lightness Image", 0, 101);
	cv::setTrackbarPos("K Size", "No Lightness Image", 101);
	cv::createTrackbar("Threshold", "Binary Image", 0, 255);
	cv::setTrackbarPos("Threshold", "Binary Image", 50);

	while (1)
	{
		// Read 1 frame from Video File
		cap.read(img);
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}

		// Show Video Image--------------------------------------
		cv::imshow("Original Image", img);

		// Convert BGR to Gray------------------------------------
		//cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
		//cv::imshow("Gray Image", img_gray);

		// Convert BGR to Lab-----------------------------------
		// 조명 효과 제거하기 위한 변환
		cv::cvtColor(img, img_lab, cv::COLOR_BGR2Lab);
		std::vector<cv::Mat> lab;
		cv::split(img_lab, lab);
		cv::Mat lightness = lab[0];
		cv::Mat lightness_blur, lightness_not, temp;
		cv::bitwise_not(lightness, lightness_not);
		int ksize = cv::getTrackbarPos("K Size", "No Lightness Image");
		if (ksize % 2 == 0) ksize--;
		if (ksize < 3) ksize = 1;
		cv::medianBlur(lightness, lightness_blur, ksize);
		cv::add(lightness_not, lightness_blur, temp);
		cv::bitwise_not(temp, lab[0]);
		cv::merge(lab, img_lab);
		cv::cvtColor(img_lab, img_no_light, cv::COLOR_Lab2BGR);
		cv::imshow("No Lightness Image", img_no_light);
		
		// Convert BGR to Gray-------------------------------------
		cv::cvtColor(img_no_light, img_gray, cv::COLOR_BGR2GRAY);
		cv::imshow("No Lightness Gray Image", img_gray);

		// Convert BGR to HSV----------------------------------------
		/*
		cv::cvtColor(lightness_remove, img_hsv, cv::COLOR_BGR2HSV);
		std::vector<cv::Mat> hsv;
		cv::split(img_hsv, hsv);
		cv::Mat hue = hsv[0];
		cv::Mat saturation = hsv[1];
		cv::Mat value = hsv[2];
		value = value > 50;
		cv::inRange(saturation, cv::Scalar(50), cv::Scalar(100), saturation);
		cv::imshow("H", hue);
		cv::imshow("S", saturation);
		cv::imshow("V", value);
		*/
		
		// Gaussian Filter-------------------------------------
		double sigma = 2;
		cv::GaussianBlur(img_gray, img_gaussian, cv::Size(), sigma);

		// Binarization----------------------------------------
		int threshold = cv::getTrackbarPos("Threshold", "Binary Image");
		img_binary_gaussian = img_gray > threshold;
		cv::imshow("Binary Image", img_binary_gaussian);

		// Adaptive Binarization ------------------------------
		/*
		// Get Block Size
		int bsize = cv::getTrackbarPos("Block Size", "Adaptive Binarization Image");
		if (bsize % 2 == 0) bsize--;
		if (bsize < 3) bsize = 3;

		// adaptiveTreshold Function
		cv::adaptiveThreshold(img_gray, img_aBinary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, bsize, 3);
		cv::adaptiveThreshold(img_gaussian, img_aBinary_gaussian, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, bsize, 3);
		cv::imshow("Adaptive Binarization Image", img_aBinary);
		cv::imshow("Adaptive Binarization Image of Gaussian", img_aBinary_gaussian);
		*/

		// Camera Calibration ---------------------------------
		cv::Mat img_cali;
		cv::remap(img_binary_gaussian, img_cali, mapx, mapy, cv::INTER_LINEAR);
		cv::imshow("Camera Calibration Image", img_cali);

		// Perspective Transform--------------------------------
		cv::Mat img_pers;
		cv::warpPerspective(img_cali, img_pers, pers, cv::Size());
		cv::imshow("Perspective Transform Image", img_pers);

		// Lane Detection ---------------------------------------
		cv::Mat img_zero = cv::Mat::zeros(480, 640, CV_8U);
		int index = 0, column = 100, row = 50;
		std::vector<cv::Point> right_lane;
		for (int i = 0; i < 4; i++)
		{
			int x, y = 420 - (i * row);
			if (i == 0) x = 400;
			else x = 400 + index - (column / 2);
			cv::Rect right_rect(x, y, column, row);
			cv::Mat sum_col;
			int max_value = 0;
			for (int j = 0; j < column; j++)
			{
				sum_col.push_back(cv::sum(img_pers(right_rect).col(j))[0]);
				int value = (int)(cv::sum(img_pers(right_rect).col(j))[0]);
				if (value > 5000)
				{
					max_value = value;
					index = j + 8;
					right_lane.push_back(cv::Point((x + index), (y + (row / 2))));
					std::cout << sum_col << std::endl;
					cv::rectangle(img_pers, right_rect, cv::Scalar(255), 1);
					break;
				}
			}
			//right_lane.push_back(cv::Point((x + index), (y + (row / 2))));
			//std::cout << sum_col << std::endl;
			//cv::rectangle(img_pers, right_rect, cv::Scalar(255), 1);
		}
		cv::imshow("Check ROI Image", img_pers);

		// Draw Right Lane
		cv::polylines(img_zero, right_lane, false, cv::Scalar(255), 2);
		cv::imshow("Check Right Lane", img_zero);

		// WaitKey
		int key = cv::waitKey(1);
		if (key >= 0)
		{
			// Print Key
			std::cout << "Key Value: " << key << std::endl;

			// Release Resource
			cap.release();
			cv::destroyAllWindows();
			break;
		}
	}

	return 0;
}
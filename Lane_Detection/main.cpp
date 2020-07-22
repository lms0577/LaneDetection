#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>

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
	cv::VideoCapture cap("D:\\Test_Video_File\\Demo_Video.mp4");
	if (!cap.isOpened())
	{
		std::cerr << "Fail Read Video\n";
		return -1;
	}

	// Declare Variable
	cv::Mat img, img_gray, img_aBinary, img_gaussian, img_aBinary_gaussian;

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
	cv::setTrackbarPos("Block Size", "Adaptive Binarization Image", 11);

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

	while (1)
	{
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
		cv::adaptiveThreshold(img_gray, img_aBinary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, bsize, 3);
		cv::adaptiveThreshold(img_gaussian, img_aBinary_gaussian, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, bsize, 3);
		cv::imshow("Adaptive Binarization Image", img_aBinary);
		cv::imshow("Adaptive Binarization Image of Gaussian", img_aBinary_gaussian);

		// Camera Calibration
		cv::Mat img_cali;
		cv::remap(img_aBinary_gaussian, img_cali, mapx, mapy, cv::INTER_LINEAR);
		cv::imshow("Camera Calibration Image", img_cali);

		// Perspective Transform
		cv::Mat img_pers;
		cv::warpPerspective(img_cali, img_pers, pers, cv::Size());
		cv::imshow("Perspective Transform Image", img_pers);

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
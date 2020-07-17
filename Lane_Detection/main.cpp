#include "opencv2/opencv.hpp"
#include <iostream>	

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

};

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

	// Declare Mat Variable
	cv::Mat img;

	// Get Video width x height
	double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

	// Declare LaneDetection Class
	LaneDetection laneDetection(width, height);

	std::cout << laneDetection.getWidth() << " " << laneDetection.getHeight() << std::endl;

	while (1)
	{
		// Read 1 frame from Video File
		cap.read(img);
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}

		// Show Frame
		cv::imshow("Video Frame", img);

		// WaitKey
		int key = cv::waitKey(25);
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
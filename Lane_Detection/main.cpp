#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>
//#include <string>
//#include <cstdlib>

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

//#define Main
#define Debug

#ifdef Main
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
	cv::namedWindow("Adaptive Binarization Image");
	cv::namedWindow("No Lightness Image");
	cv::namedWindow("Binary Image");
	cv::namedWindow("HSV Image", cv::WINDOW_NORMAL);
	cv::resizeWindow("HSV Image", cv::Size(640, 480));

	// Create Trackbar
	cv::createTrackbar("Block Size", "Adaptive Binarization Image", 0, 200);
	cv::setTrackbarPos("Block Size", "Adaptive Binarization Image", 11);
	cv::createTrackbar("K Size", "No Lightness Image", 0, 200);
	cv::setTrackbarPos("K Size", "No Lightness Image", 101);
	cv::createTrackbar("Threshold", "Binary Image", 0, 255);
	cv::setTrackbarPos("Threshold", "Binary Image", 50);
	cv::createTrackbar("Lower Hue", "HSV Image", 0, 180);
	cv::setTrackbarPos("Lower Hue", "HSV Image", 0);
	cv::createTrackbar("Upper Hue", "HSV Image", 0, 180);
	cv::setTrackbarPos("Upper Hue", "HSV Image", 50);
	cv::createTrackbar("Lower Sat", "HSV Image", 0, 255);
	cv::setTrackbarPos("Lower Sat", "HSV Image", 50);
	cv::createTrackbar("Upper Sat", "HSV Image", 0, 255);
	cv::setTrackbarPos("Upper Sat", "HSV Image", 255);
	cv::createTrackbar("Lower Val", "HSV Image", 0, 255);
	cv::setTrackbarPos("Lower Val", "HSV Image", 30);
	cv::createTrackbar("Upper Val", "HSV Image", 0, 255);
	cv::setTrackbarPos("Upper Val", "HSV Image", 255);

	int count = 0, right_lane_start_index = 0;

	while (1)
	{
		// Read 1 frame from Video File
		cap.read(img);
		//img = cv::imread("D:\\Computer Vision\\LaneDetection\\Video_Frame\\image_10.jpg");
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}

		// Gaussian Filter-------------------------------------
		//double sigma = 2;
		cv::GaussianBlur(img, img_gaussian, cv::Size(), 1);

		// Show Video Image--------------------------------------
		cv::imshow("Original Image", img);

		// Convert BGR to Gray------------------------------------
		//cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
		//cv::imshow("Gray Image", img_gray);

		// Convert BGR to HSV----------------------------------------
		/*cv::Mat hsv, v, white, yellow, or_img;
		cv::Scalar yellow_lower(5, 90, 100);
		cv::Scalar yellow_upper(200, 255, 255);
		std::vector<cv::Mat> hsv_split;
		cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
		cv::split(hsv, hsv_split);
		v = hsv_split[2];
		cv::inRange(v, 245, 255, white);
		cv::inRange(hsv, yellow_lower, yellow_upper, yellow);
		cv::bitwise_or(white, yellow, or_img);
		cv::imshow("White", white);
		cv::imshow("Yellow", yellow);
		cv::imshow("Or Image", or_img);*/

		// Convert BGR to Lab-----------------------------------
		// 조명 효과 제거하기 위한 변환
		cv::cvtColor(img_gaussian, img_lab, cv::COLOR_BGR2Lab);
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

		// Convert BGR to HSV----------------------------------------
		cv::Mat hue, saturation, value, dst, hsv_range, h_range, s_range, v_range, dst_and, test;
		int hue_low = cv::getTrackbarPos("Lower Hue", "HSV Image");
		int hue_up = cv::getTrackbarPos("Upper Hue", "HSV Image");
		int sat_low = cv::getTrackbarPos("Lower Sat", "HSV Image");
		int sat_up = cv::getTrackbarPos("Upper Sat", "HSV Image");
		int val_low = cv::getTrackbarPos("Lower Val", "HSV Image");
		int val_up = cv::getTrackbarPos("Upper Val", "HSV Image");
		cv::Scalar lower(hue_low, sat_low, val_low);
		cv::Scalar upper(hue_up, sat_up, val_up);
		std::vector<cv::Mat> hsv_split;
		cv::cvtColor(img_no_light, img_hsv, cv::COLOR_BGR2HSV);
		cv::inRange(img_hsv, lower, upper, hsv_range);
		cv::imshow("HSV Range Binary Image", hsv_range);

		cv::Mat img_or, yello_01, white_01;
		cv::inRange(img_hsv, cv::Scalar(0, 50, 50), cv::Scalar(50, 255, 255), yello_01);
		cv::inRange(img_hsv, cv::Scalar(0, 0, 30), cv::Scalar(180, 50, 255), white_01);
		cv::bitwise_or(yello_01, white_01, img_or);
		cv::imshow("Or Image", img_or);

		//cv::bitwise_and(img_hsv, img_hsv, dst_and, hsv_range);
		cv::split(img_hsv, hsv_split);
		hue = hsv_split[0];
		saturation = hsv_split[1];
		value = hsv_split[2];
		//cv::inRange(hue, 0, 50, hue);
		//cv::inRange(saturation, 0, 50, saturation);
		//cv::inRange(hue, hue_low, hue_up, hue);
		//cv::inRange(saturation, sat_low, sat_up, saturation);
		//cv::inRange(value, val_low, val_up, value);
		//cv::bitwise_and(img_hsv, img_hsv, h_range, hue);
		//cv::bitwise_and(img_hsv, img_hsv, s_range, saturation);
		//cv::bitwise_and(img_hsv, img_hsv, v_range, value);
		//cv::bitwise_or(h_range, s_range, test);
		//cv::bitwise_or(hsv_range, v_range, hsv_range);
		//cv::cvtColor(dst_and, dst, cv::COLOR_HSV2BGR);
		//cv::inRange(hsv, lower_hue, upper_hue, yellow);
		//cv::bitwise_or(white, yellow, or_img);
		//cv::imshow("White", white);
		//cv::imshow("HSV Range Image", dst);
		//img_roi = img_no_light(roi).row(0);
		/*std::cout << "Hue: " << std::endl;
		std::cout << h.row(460) << std::endl;
		std::cout << "Saturation: " << std::endl;
		std::cout << s.row(460) << std::endl;
		std::cout << "Value: " << std::endl;
		std::cout << v.row(460) << std::endl;

		cv::line(img_no_light, cv::Point(0, 460), cv::Point(640, 460), cv::Scalar(0, 0, 255), 2);
		cv::imshow("Show ROI", img_no_light);*/

		// Canny Edge ----------------------------------------------
		cv::Mat img_canny;
		cv::Canny(img_or, img_canny, 150, 300);
		cv::imshow("Canny Edge Image", img_canny);

		// Convert BGR to Gray-------------------------------------
		/*cv::cvtColor(img_no_light, img_gray, cv::COLOR_BGR2GRAY);
		cv::imshow("No Lightness Gray Image", img_gray);*/
		

		// Convert BGR to HSV----------------------------------------
		//cv::cvtColor(img_no_light, img_hsv, cv::COLOR_BGR2HSV);
		//cv::Mat test;
		//std::vector<cv::Mat> hsv;
		//cv::split(img_hsv, hsv);
		//cv::Mat hue = hsv[0];
		//cv::Mat saturation = hsv[1];
		//cv::Mat value = hsv[2];
		//cv::Scalar yellow_low(5, 90, 100);
		//cv::Scalar yellow_up(200, 255, 255);
		////value = value > 50;
		//cv::inRange(img_hsv, yellow_low, yellow_up, test);
		//cv::imshow("Test", test);
		////cv::imshow("H", hue);
		////cv::imshow("S", saturation);
		////cv::imshow("V", value);

		
		// Gaussian Filter-------------------------------------
		//double sigma = 2;
		//cv::GaussianBlur(value, img_gaussian, cv::Size(), sigma);

		// Binarization----------------------------------------
		/*
		int threshold = cv::getTrackbarPos("Threshold", "Binary Image");
		img_binary_gaussian = img_gray > threshold;
		cv::imshow("Binary Image", img_binary_gaussian);
		*/

		// Adaptive Binarization ------------------------------
		// Get Block Size
		//int bsize = cv::getTrackbarPos("Block Size", "Adaptive Binarization Image");
		//if (bsize % 2 == 0) bsize--;
		//if (bsize < 3) bsize = 3;

		// adaptiveTreshold Function
		//cv::adaptiveThreshold(img_gaussian, img_aBinary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, bsize, 3);
		//cv::adaptiveThreshold(img_gaussian, img_aBinary_gaussian, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, bsize, 3);
		//cv::imshow("Adaptive Binarization Image", img_aBinary);
		//cv::imshow("Adaptive Binarization Image of Gaussian", img_aBinary_gaussian);

		// Camera Calibration ---------------------------------
		cv::Mat img_cali;
		cv::remap(img_canny, img_cali, mapx, mapy, cv::INTER_LINEAR);
		cv::imshow("Camera Calibration Image", img_cali);

		// Perspective Transform--------------------------------
		cv::Mat img_pers;
		cv::warpPerspective(img_cali, img_pers, pers, cv::Size());
		img_pers = img_pers >= 50;
		cv::imshow("Perspective Transform Image", img_pers);

		// Sobel Operator ----------------------------------------------
		/*cv::Mat img_sobel_x, img_sobel_y;
		cv::Sobel(img_pers, img_sobel_x, CV_32FC1, 1, 0);
		cv::Sobel(img_pers, img_sobel_y, CV_32FC1, 0, 1);
		img_sobel_x.convertTo(img_sobel_x, CV_8UC1);
		img_sobel_y.convertTo(img_sobel_y, CV_8UC1);
		img_sobel_x = img_sobel_x > 250;
		img_sobel_y = img_sobel_y > 250;
		cv::imshow("Sobel dx Image", img_sobel_x);
		cv::imshow("Sobel dy Image", img_sobel_y);*/

		// Check Lane Pixel--------------------------------------
		cv::Rect right_01(415, 405, 20, 40);
		cv::Rect right_02(415, 365, 20, 40);
		cv::Rect right_03(415, 325, 20, 40);
		cv::Rect right_04(415, 285, 20, 40);
		cv::Mat img_check = img_pers.clone();
		cv::Mat lane_pixel_01 = img_pers(right_01);
		cv::Mat lane_pixel_02 = img_pers(right_02);
		cv::Mat lane_pixel_03 = img_pers(right_03);
		cv::Mat lane_pixel_04 = img_pers(right_04);
		cv::rectangle(img_check, right_01, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_02, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_03, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_04, cv::Scalar(255), 1);
		cv::imshow("Check Lane Pixel Image", img_check);
		std::cout << lane_pixel_04 << std::endl;
		std::cout << lane_pixel_03 << std::endl;
		std::cout << lane_pixel_02 << std::endl;
		std::cout << lane_pixel_01 << std::endl;
		std::cout << "--------------------------------------" << std::endl;

		// Lane Detection 02---------------------------------------
		//std::vector<int> lane_index;
		int window_num = 4, lane_count, lane_index = 0;
		int row = 40, column = 20;
		
		// 오른쪽 차선
		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < window_num; i++)
		{
			if (right_lane_start_index == 0)
			{
				// x = 415(열)부터 선 찾기 시작해서 x = 415 + 20 까지만 찾기
				// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩) 
				right_lane_start_index = 415;
				int x = 415, y = 445;
				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < row; r++)
				{
					// 영상의 y = 445(행), x = 415(열)의 원소값
					uchar data = img_pers.at<uchar>(y, x);

					// lane_index의 값이 50이상인 index가 있다면 20(열)의 반복 생략
					if (lane_index != 0)
					{

					}
					// lane_index의 값이 없다면 20(열)의 반복으로 찾기
					else
					{
						for (int c = 0; c < column; c++)
						{

						}
					}
				}
			}
			else
			{
				//std::cout << "index != 0" << std::endl;
			}
		}

		// Lane Detection 01---------------------------------------
		/*
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
		*/

		// Save Frame ------------------------------------------------
		count++;
		std::string num = std::to_string(count);
		std::string filename = "D:\\Computer Vision\\LaneDetection\\Video_Frame\\image_" + num + ".bmp";
		cv::imwrite(filename, img_pers);

		// WaitKey
		int key = cv::waitKey(100);
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
#endif // Main

#ifdef Debug
int main(int argc, char** argv)
{
	cv::Mat img;
	int count = 92;

	/*std::string num = std::to_string(count);
	std::string filename = "d:\\computer vision\\lanedetection\\video_frame\\image_" + num + ".jpg";
	cv::imwrite(filename, img_pers);*/

	int right_lane_start_index = 423;

	while (1)
	{
		std::string num = std::to_string(count);
		std::string filename = "d:\\computer vision\\lanedetection\\video_frame\\Bird Eye View\\image_" + num + ".jpg";
		img = cv::imread(filename);
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}
		cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
		img = img > 50;

		// Lane Detection ---------------------------------------
		//std::vector<int> lane_index;
		int window_num = 1, lane_count, lane_index = 0;
		int row = 40, margin = 2;
		//std::vector<cv::Point> lane_point;
		std::vector<cv::Point> right_lane;
		// y = 445(행)부터 시작
		int y = 445;

		// 오른쪽 차선
		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < window_num; i++)
		{
			lane_count = 0;

			// 첫번째 슬라이딩 윈도우
			if (i == 0)
			{
				cv::Point start_point, end_point, tmp_point;

				// x = right_lane_start_index(열)부터 선 찾기 시작해서 x = x + 5 까지만 찾기
				// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
				//int x = right_lane_start_index;

				// start_point 찾으면 flag 올리기
				bool start_flag = false;

				// 중간에 선이 끊기면 flag 올리기
				bool lose_flag = false;

				// 임시 x(열) 값 저장 변수
				int x_tmp;

				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < row; r++)
				{
					uchar *p = img.ptr<uchar>(y);

					// start_point 못 찾았으면 계속 찾는다
					if (start_flag == false)
					{
						// x = right_lane_start_index(열)부터 선 찾기 시작해서 x = x + 5 까지만 찾기
						// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
						int column = 5;
						int x = right_lane_start_index;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// right_lane_start_index 저장(margin만큼 뺀 값을 저장)
								// 다음 프레임에서 반복문 횟수를 줄이기 위한 과정
								// margin 많이 할 수록 계산 횟수 증가
								right_lane_start_index = x - margin;

								// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
								start_point = cv::Point(x, y);
								end_point = cv::Point(x, y);
								start_flag = true;
								x_tmp = x;
								break;
							}
							else x++;
						}
					}
					// start_point 찾았을 때
					else if (start_flag == true && lose_flag == false)
					{
						// 영상의 y(행), x(열)의 원소값
						// start_point 바로 위의 원소값부터 차례대로 확인
						int data = p[x_tmp];

						// 바로 위의 값이 있다면 그 다음 행으로 이동
						if (data == 255)
						{
							end_point = cv::Point(x_tmp, y);
						}
						// 바로 위의 값이 없다면 왼쪽 오른쪽 확인
						else if(data == 0)
						{
							// 바로 위의 값이 없다면 왼쪽부터 확인
							int data_left = p[(x_tmp - 1)];
							int data_right = p[(x_tmp + 1)];
							// 왼쪽에 값이 있으면 왼쪽으로 이동
							if (data_left == 255)
							{
								lane_count--;
								x_tmp = x_tmp - 1;
								if (abs(lane_count) >= 3)
								{
									tmp_point = cv::Point(x_tmp, y);
								}
								end_point = cv::Point(x_tmp, y);
							}
							// 왼쪽에 값이 없고 오른쪽에 있으면 오른쪽으로 이동
							else if (data_right == 255)
							{
								lane_count++;
								x_tmp = x_tmp + 1;
								if (abs(lane_count) >= 3)
								{
									tmp_point = cv::Point(x_tmp, y);
								}
								end_point = cv::Point(x_tmp, y);
							}
							// 왼쪽과 오른쪽에 둘다 값이 없으면 그 다음 행으로 이동
							else
							{
								lose_flag = true;
							}
						}
					}
					// start_point 찾았고 중간에 선이 끊겼을 때
					else if (start_flag == true && lose_flag == true)
					{
						// start_point의 x - 2 부터 end_point의 x + 2 까지 검색
						int column = (end_point.x + 2) - (start_point.x - 2) + 1;
						int x = start_point.x - 2;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
								end_point = cv::Point(x, y);
								lose_flag = false;
								x_tmp = x;
								// end_point 변경으로 lane_count 다시 설정
								lane_count = (end_point.x - start_point.x);
								break;
							}
							else x++;
						}
					}
					// 그 다음 행으로 이동(위로 이동)
					y--;
				}

				// 첫번째 슬라이딩에서 선 못 찾았으면 저장 X
				if (start_point.x != 0)
				{
					// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
					// abs(lane_count) <= 3: 직선
					if (abs(lane_count) <= 3)
					{
						// lane_count 값을 기준으로 직선의 열을 정한다.
						// lane_count 값이 양수면 start_point를 기준
						if (lane_count > 0)
						{
							start_point.y = 445;
							end_point.x = start_point.x;
							end_point.y = y + 1;
							right_lane.push_back(start_point);
							right_lane.push_back(end_point);
						}
						// lane_count 값이 음수면 end_point를 기준
						else if (lane_count < 0)
						{
							start_point.y = 445;
							start_point.x = end_point.x;
							end_point.y = y + 1;
							right_lane.push_back(start_point);
							right_lane.push_back(end_point);
						}
						// lane_count 값이 0이면 그대로
						else
						{
							start_point.y = 445;
							end_point.y = y + 1;
							right_lane.push_back(start_point);
							right_lane.push_back(end_point);
						}
					}
					// lane_count >= 4: 우회전
					else if (lane_count >= 4)
					{
						right_lane.push_back(start_point);
						right_lane.push_back(end_point);
					}
					// lane_count <= -4: 좌회전
					else if (lane_count <= -4)
					{
						right_lane.push_back(start_point);
						right_lane.push_back(end_point);
					}
				}
			}

			// 두번째부터의 슬라이딩 윈도우
			else
			{

			}
			
		}

		// Draw Right Lane
		cv::Mat img_zero = cv::Mat::zeros(480, 640, CV_8U);
		cv::polylines(img_zero, right_lane, false, cv::Scalar(255), 2);
		cv::imshow("Check Right Lane", img_zero);
		//std::cout << "Right_Lane 값: " << right_lane << std::endl;

		// Check Lane Pixel--------------------------------------
		cv::Rect right_01(415, 405, 20, 40);
		cv::Rect right_02(415, 365, 20, 40);
		cv::Rect right_03(415, 325, 20, 40);
		cv::Rect right_04(415, 285, 20, 40);
		cv::Mat img_check = img.clone();
		cv::Mat lane_pixel_01 = img(right_01);
		cv::Mat lane_pixel_02 = img(right_02);
		cv::Mat lane_pixel_03 = img(right_03);
		cv::Mat lane_pixel_04 = img(right_04);
		cv::rectangle(img_check, right_01, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_02, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_03, cv::Scalar(255), 1);
		cv::rectangle(img_check, right_04, cv::Scalar(255), 1);
		cv::imshow("Check Lane Pixel Image", img_check);
		std::cout << lane_pixel_04 << std::endl;
		std::cout << lane_pixel_03 << std::endl;
		std::cout << lane_pixel_02 << std::endl;
		std::cout << lane_pixel_01 << std::endl;
		std::cout << "--------------------------------------" << std::endl;

		 // Test Code
		/*int number = 1;
		for (int i = 0; i < 2; i++)
		{
			if (number == 1)
			{
				std::cout << "Number: " << number << std::endl;
				number++;
			}
			else
			{
				if (number == 2)
				{
					std::cout << "Number: " << number << std::endl;
					break;
				}
				std::cout << "Number: " << number << std::endl;
			}
		}*/
		/*for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				if (i == 1)
				{
					break;
				}
				else
				{
					std::cout << "j: " << j << std::endl;
				}
				
			}
			std::cout << "i: " << i << std::endl;
		}*/
		//cv::Point test;
		//std::cout << "Point test 값: " << test << std::endl;

		// WaitKey
		int key = cv::waitKey(0);
		if (key == 27) // ESC
		{
			// Print Key
			//std::cout << "Key Value: " << key << std::endl;

			// Release Resource
			cv::destroyAllWindows();
			break;
		}
		else if (key == 97) // a
		{
			if (count == 13) count = 13;
			else count--;
		}
		else if (key == 100) // d
		{
			if (count == 524) count = 524;
			else count++;
		}
		else
		{
			// Print Key
			//std::cout << "Key Value: " << key << std::endl;

			// Release Resource
			cv::destroyAllWindows();
			break;
		}
	}
}
#endif // Debug

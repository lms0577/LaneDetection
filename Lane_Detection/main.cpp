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

#define Main
//#define Debug

#ifdef Main
int main(int argc, char** argv)
{
	// Confirm OpenCV Version --------------------------------------------
	std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

	// Open Video File ----------------------------------------------------
	cv::VideoCapture cap("..\\Demo_Video\\Test_Video.mp4");
	if (!cap.isOpened())
	{
		std::cerr << "Fail Read Video\n";
		return -1;
	}

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
	// Get Inverse Perspective Transform Matrix
	cv::Mat invPers = cv::getPerspectiveTransform(dstQuad, srcQuad);

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

	// Lane Detection Variables
	int right_lane_start_index = 423;
	int left_lane_start_index = 248;

	while (1)
	{
		// Read 1 frame from Video File
		cv::Mat img;
		cap.read(img);
		//img = cv::imread("..\\Video_Frame\\Original Image\\image_70.jpg");
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}
		cv::imshow("Original Image", img);

		// Gaussian Filter-------------------------------------
		cv::Mat img_gaussian;
		double sigma = 1;
		cv::GaussianBlur(img, img_gaussian, cv::Size(), sigma);
		cv::imshow("Gaussian Filter Image", img_gaussian);

		// Convert BGR to Lab-----------------------------------
		// 조명 효과 제거하기 위한 변환
		cv::Mat img_lab, img_no_light;
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
		cv::Mat img_hsv, hue, saturation, value, hsv_range;
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
		
		//cv::split(img_hsv, hsv_split);
		//hue = hsv_split[0];
		//saturation = hsv_split[1];
		//value = hsv_split[2];img_or

		// Canny Edge ----------------------------------------------
		cv::Mat img_canny;
		cv::Canny(img_or, img_canny, 150, 300);
		cv::imshow("Canny Edge Image", img_canny);

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
		/*cv::Rect right_01(415, 405, 20, 40);
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
		cv::imshow("Check Lane Pixel Image", img_check);*/
		//std::cout << lane_pixel_04 << std::endl;
		//std::cout << lane_pixel_03 << std::endl;
		//std::cout << lane_pixel_02 << std::endl;
		//std::cout << lane_pixel_01 << std::endl;
		//std::cout << "--------------------------------------" << std::endl;

		// Lane Detection ------------------------------------------------------------------------
		//cv::Mat img_check = img_pers.clone();
		cv::Mat img_lane = img_pers.clone();

		// 오른쪽 차선------------시작-----------------------------------------------
		// 윈도우 개수
		int right_window_num = 3;
		// 윈도우당 40행씩
		int right_row = 40;
		// 오른쪽 차선 포인트 저장 변수
		std::vector<cv::Point> right_lane;
		// y = 445(행)부터 시작
		int right_y = 445;

		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < right_window_num; i++)
		{
			// 찾은 선이 직선인지 곡선인지 판별하기 위한 변수
			int lane_count = 0;

			// 첫번째 슬라이딩 윈도우
			if (i == 0)
			{
				// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
				// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
				cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

				// start_point 찾으면 flag 올리기
				bool start_flag = false;

				// 중간에 선이 끊기면 flag 올리기
				bool lose_flag = false;

				// 임시 x(열) 값 저장 변수
				int x_tmp;

				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < right_row; r++)
				{
					// 현재 y(행)의 시작 포인터 변수
					uchar *p = img_lane.ptr<uchar>(right_y);

					// start_point 못 찾았으면 계속 찾는다
					if (start_flag == false)
					{
						// x = right_lane_start_index(열)부터 선 찾기 시작해서 x = x + 20 까지만 찾기
						// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
						int column = 20;
						int x = right_lane_start_index;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
								// end_point도 저장
								start_point = cv::Point(x, right_y);
								end_point = cv::Point(x, right_y);
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
						// 영상의 y(행), x_tmp(열)의 원소값
						// start_point 바로 위의 원소값부터 차례대로 확인
						int data = p[x_tmp];

						// 바로 위의 값이 있다면 그 다음 행으로 이동
						if (data == 255)
						{
							end_point = cv::Point(x_tmp, right_y);
						}

						// 바로 위의 값이 없다면 왼쪽 오른쪽 확인
						else if (data == 0)
						{
							// 바로 위의 값이 없다면 왼쪽부터 확인
							int data_left_01 = p[(x_tmp - 2)];
							int data_left_02 = p[(x_tmp - 1)];
							int data_right_01 = p[(x_tmp + 1)];
							int data_right_02 = p[(x_tmp + 2)];

							// 왼쪽에 값이 있으면 왼쪽으로 이동
							if (data_left_01 == 255)
							{
								lane_count = lane_count - 2;
								x_tmp = x_tmp - 2;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}
							else if (data_left_02 == 255)
							{
								lane_count = lane_count - 1;
								x_tmp = x_tmp - 1;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}

							// 왼쪽에 값이 없고 오른쪽에 있으면 오른쪽으로 이동
							else if (data_right_01 == 255)
							{
								lane_count = lane_count + 1;
								x_tmp = x_tmp + 1;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}
							else if (data_right_02 == 255)
							{
								lane_count = lane_count + 2;
								x_tmp = x_tmp + 2;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
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
						// column, x 변수 선언
						int column, x;

						// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
						if (lane_count > 0)
						{
							// start_point의 x - 10 부터 end_point의 x + 5 까지 검색
							column = (end_point.x + 5) - (start_point.x - 10) + 1;
							x = start_point.x - 10;
						}
						else if (lane_count < 0)
						{
							// end_point의 x - 20 부터 start_point의 x + 5 까지 검색
							column = (start_point.x + 5) - (end_point.x - 20) + 1;
							x = end_point.x - 20;
						}
						else
						{
							// end_point의 x - 5 부터 start_point의 x + 5 까지 검색
							column = 15;
							x = end_point.x - 5;
						}

						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
								end_point = cv::Point(x, right_y);
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
					right_y--;
				}

				// 첫번째 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
				if (start_point.x != 0)
				{
					// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
					// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

					// 출력
					std::cout << "start_point: " << start_point << std::endl;
					std::cout << "end_point: " << end_point << std::endl;
					std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
					std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

					// 가짜 곡선 flag 변수 선언
					bool fake_flag = false;
					// 진짜 곡선 flag 변수 선언
					bool curve_flag = false;

					// abs(lane_count) >= 4: 곡선
					// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
					if (abs(lane_count) >= 4)
					{
						// 가짜 곡선인지 확인
						// tmp_point_3 값이 있으면 이것과 end_point 비교
						if (tmp_point_3.x != 0)
						{
							// (tmp_point_3.y - end_point.y)의 값이
							// 3보다 크면 곡선
							if ((tmp_point_3.y - end_point.y) > 3)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
						else if (tmp_point_4.x != 0)
						{
							// (tmp_point_4.y - end_point.y)의 값이
							// 2보다 크면 곡선
							if ((tmp_point_4.y - end_point.y) > 2)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3, tmp_point_4 값이 없을 때
						// 기울기의 절대값이 1보다 크면 직선
						// 기울기의 절대값이 1보다 작거나 같으면 곡선
						else
						{
							int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
							if (gradient > 1) fake_flag = true;
							else curve_flag = true;
						}
					}

					// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
					// curve_flag == true: 곡선
					if (curve_flag == true)
					{
						// lane_count >= 4: 우회전
						if (lane_count >= 4)
						{
							// right_lane_start_index 저장
							right_lane_start_index = start_point.x - 10;
						}
						// lane_count <= -4: 좌회전
						else if (lane_count <= -4)
						{
							// right_lane_start_index 저장
							right_lane_start_index = start_point.x - 15;
						}

						// end_point.y 값이 right_y + 1 값과 같지 않다면
						// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
						if (end_point.y != (right_y + 1))
						{
							end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (right_y + 1)));
						}
					}

					// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
					else if (abs(lane_count) <= 3 || fake_flag == true)
					{
						// right_lane_start_index 저장
						right_lane_start_index = start_point.x - 7;

						// lane_count 값을 기준으로 직선의 열을 정한다.
						// lane_count 값이 양수면 start_point를 기준
						if (lane_count > 0)
						{
							end_point.x = start_point.x;
						}
						// lane_count 값이 음수이거나 0이면 end_point를 기준
						else if (lane_count <= 0)
						{
							start_point.x = end_point.x;
						}
					}

					// 포인트 저장
					// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
					start_point.y = 445;
					end_point.y = right_y + 1;
					right_lane.push_back(start_point);
					right_lane.push_back(end_point);
				}
			}

			// 두번째부터의 슬라이딩 윈도우
			else
			{
				// 첫번째 슬라이딩 윈도우에서 못 찾으면 그 다음부터 찾지 않는다. 또는
				// 세번째 슬라이딩 윈도우부터 그 전의 슬라이딩 윈도우 못 찾으면
				// 그 다음부터 찾지 않는다.
				if (right_lane.empty() || (right_lane.size() != (i * 2)))
				{
					break;
				}
				// 슬라이딩 윈도우에서 찾았으면 그 다음 윈도우를 검색
				else
				{
					// 이전의 윈도우가 직선, 곡선이냐에 따라서 start_index를 설정한다.
					int start_index, index = (2 * i - 2) + 1;
					int judge_num = right_lane[index].x - right_lane[index - 1].x;
					// 직선일 때
					if (judge_num == 0) start_index = right_lane[index].x - 7;
					// 우회전일 때
					else if (judge_num > 0) start_index = right_lane[index].x - 10;
					// 좌회전일 때
					else start_index = right_lane[index].x - 15;

					// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
					// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
					cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

					// start_point 찾으면 flag 올리기
					bool start_flag = false;

					// 중간에 선이 끊기면 flag 올리기
					bool lose_flag = false;

					// 임시 x(열) 값 저장 변수
					int x_tmp;

					// 윈도우당 총 40행이므로 40번 반복
					for (int r = 0; r < right_row; r++)
					{
						// 현재 y(행)의 시작 포인터 변수
						uchar *p = img_lane.ptr<uchar>(right_y);

						// start_point 못 찾았으면 계속 찾는다
						if (start_flag == false)
						{
							// x = start_index(열)부터 선 찾기 시작해서 x = x + 20 까지만 찾기
							// 행은 윈도우당 40개씩
							int column = 20;
							int x = start_index;
							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
									// end_point도 저장
									start_point = cv::Point(x, right_y);
									end_point = cv::Point(x, right_y);
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
							// 영상의 y(행), x_tmp(열)의 원소값
							// start_point 바로 위의 원소값부터 차례대로 확인
							int data = p[x_tmp];

							// 바로 위의 값이 있다면 그 다음 행으로 이동
							if (data == 255)
							{
								end_point = cv::Point(x_tmp, right_y);
							}

							// 바로 위의 값이 없다면 왼쪽 오른쪽 확인
							else if (data == 0)
							{
								// 바로 위의 값이 없다면 왼쪽부터 확인
								int data_left_01 = p[(x_tmp - 2)];
								int data_left_02 = p[(x_tmp - 1)];
								int data_right_01 = p[(x_tmp + 1)];
								int data_right_02 = p[(x_tmp + 2)];

								// 왼쪽에 값이 있으면 왼쪽으로 이동
								if (data_left_01 == 255)
								{
									lane_count = lane_count - 2;
									x_tmp = x_tmp - 2;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}
								else if (data_left_02 == 255)
								{
									lane_count = lane_count - 1;
									x_tmp = x_tmp - 1;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}

								// 왼쪽에 값이 없고 오른쪽에 있으면 오른쪽으로 이동
								else if (data_right_01 == 255)
								{
									lane_count = lane_count + 1;
									x_tmp = x_tmp + 1;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}
								else if (data_right_02 == 255)
								{
									lane_count = lane_count + 2;
									x_tmp = x_tmp + 2;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
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
							// column, x 변수 선언
							int column, x;

							// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
							if (lane_count > 0)
							{
								// start_point의 x - 10 부터 end_point의 x + 5 까지 검색
								column = (end_point.x + 5) - (start_point.x - 10) + 1;
								x = start_point.x - 10;
							}
							else if (lane_count < 0)
							{
								// end_point의 x - 20 부터 start_point의 x + 5 까지 검색
								column = (start_point.x + 5) - (end_point.x - 20) + 1;
								x = end_point.x - 20;
							}
							else
							{
								// end_point의 x - 5 부터 start_point의 x + 5 까지 검색
								column = 15;
								x = end_point.x - 5;
							}

							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
									end_point = cv::Point(x, right_y);
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
						right_y--;
					}

					// 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
					if (start_point.x != 0)
					{
						// 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
						// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

						// 출력
						std::cout << "start_point: " << start_point << std::endl;
						std::cout << "end_point: " << end_point << std::endl;
						std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
						std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

						// 가짜 곡선 flag 변수 선언
						bool fake_flag = false;
						// 진짜 곡선 flag 변수 선언
						bool curve_flag = false;

						// abs(lane_count) >= 4: 곡선
						// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
						if (abs(lane_count) >= 4)
						{
							// 가짜 곡선인지 확인
							// tmp_point_3 값이 있으면 이것과 end_point 비교
							if (tmp_point_3.x != 0)
							{
								// (tmp_point_3.y - end_point.y)의 값이
								// 3보다 크면 곡선
								if ((tmp_point_3.y - end_point.y) > 3)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
							else if (tmp_point_4.x != 0)
							{
								// (tmp_point_4.y - end_point.y)의 값이
								// 2보다 크면 곡선
								if ((tmp_point_4.y - end_point.y) > 2)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3, tmp_point_4 값이 없을 때
							// 기울기의 절대값이 1보다 크면 직선
							// 기울기의 절대값이 1보다 작거나 같으면 곡선
							else
							{
								int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
								if (gradient > 1) fake_flag = true;
								else curve_flag = true;
							}
						}

						// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
						// curve_flag == true: 곡선
						if (curve_flag == true)
						{
							// end_point.y 값이 right_y + 1 값과 같지 않다면
							// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
							if (end_point.y != (right_y + 1))
							{
								end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (right_y + 1)));
							}
						}

						// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
						else if (abs(lane_count) <= 3 || fake_flag == true)
						{
							// lane_count 값을 기준으로 직선의 열을 정한다.
							// lane_count 값이 양수면 start_point를 기준
							if (lane_count > 0)
							{
								end_point.x = start_point.x;
							}
							// lane_count 값이 음수이거나 0이면 end_point를 기준
							else if (lane_count <= 0)
							{
								start_point.x = end_point.x;
							}
						}

						// 포인트 저장
						// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
						start_point.y = 445 - (i * right_row);
						end_point.y = right_y + 1;
						right_lane.push_back(start_point);
						right_lane.push_back(end_point);
					}
				}
			}
		}

		// Check Right Lane Pixel and Draw Right Lane
		if (right_lane.empty())
		{
			std::cout << "오른쪽 차선 못 찾음" << std::endl;
		}
		else
		{
			std::cout << "오른쪽 차선 찾음" << std::endl;
			
			// Draw Right Lane
			cv::polylines(img_lane, right_lane, false, cv::Scalar(255), 6);
		}
		// 오른쪽 차선------------끝-----------------------------------------------

		// 왼쪽 차선------------시작-----------------------------------------------
		// 윈도우 개수
		int left_window_num = 3;
		// 윈도우당 40행씩
		int left_row = 40;
		// 오른쪽 차선 포인트 저장 변수
		std::vector<cv::Point> left_lane;
		// y = 445(행)부터 시작
		int left_y = 445;

		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < left_window_num; i++)
		{
			// 찾은 선이 직선인지 곡선인지 판별하기 위한 변수
			int lane_count = 0;

			// 첫번째 슬라이딩 윈도우
			if (i == 0)
			{
				// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
				// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
				cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

				// start_point 찾으면 flag 올리기
				bool start_flag = false;

				// 중간에 선이 끊기면 flag 올리기
				bool lose_flag = false;

				// 임시 x(열) 값 저장 변수
				int x_tmp;

				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < left_row; r++)
				{
					// 현재 y(행)의 시작 포인터 변수
					uchar *p = img_lane.ptr<uchar>(left_y);

					// start_point 못 찾았으면 계속 찾는다
					if (start_flag == false)
					{
						// x = left_lane_start_index(열)부터 선 찾기 시작해서 x = x - 20 까지만 찾기
						// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
						int column = 20;
						int x = left_lane_start_index;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
								// end_point도 저장
								start_point = cv::Point(x, left_y);
								end_point = cv::Point(x, left_y);
								start_flag = true;
								x_tmp = x;
								break;
							}
							else x--;
						}
					}
					// start_point 찾았을 때
					else if (start_flag == true && lose_flag == false)
					{
						// 영상의 y(행), x_tmp(열)의 원소값
						// start_point 바로 위의 원소값부터 차례대로 확인
						int data = p[x_tmp];

						// 바로 위의 값이 있다면 그 다음 행으로 이동
						if (data == 255)
						{
							end_point = cv::Point(x_tmp, left_y);
						}

						// 바로 위의 값이 없다면 오른쪽 왼쪽 확인
						else if (data == 0)
						{
							// 바로 위의 값이 없다면 오른쪽부터 확인
							int data_left_01 = p[(x_tmp - 2)];
							int data_left_02 = p[(x_tmp - 1)];
							int data_right_01 = p[(x_tmp + 1)];
							int data_right_02 = p[(x_tmp + 2)];

							// 오른쪽에 있으면 오른쪽으로 이동
							if (data_right_02 == 255)
							{
								lane_count = lane_count + 2;
								x_tmp = x_tmp + 2;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							else if (data_right_01 == 255)
							{
								lane_count = lane_count + 1;
								x_tmp = x_tmp + 1;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							// 오른쪽에 값이 없고 왼쪽에 값이 있으면 왼쪽으로 이동
							else if (data_left_02 == 255)
							{
								lane_count = lane_count - 1;
								x_tmp = x_tmp - 1;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							else if (data_left_01 == 255)
							{
								lane_count = lane_count - 2;
								x_tmp = x_tmp - 2;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
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
						// column, x 변수 선언
						int column, x;

						// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
						if (lane_count > 0)
						{
							// end_point의 x + 20 부터 start_point의 x - 5 까지 검색
							column = (end_point.x + 20) - (start_point.x - 5) + 1;
							x = end_point.x + 20;
						}
						else if (lane_count < 0)
						{
							// start_point의 x + 10 부터 end_point의 x - 5 까지 검색
							column = (start_point.x + 10) - (end_point.x - 5) + 1;
							x = start_point.x + 10;
						}
						else
						{
							// end_point의 x + 5 부터 start_point의 x - 15 까지 검색
							column = 15;
							x = end_point.x + 5;
						}

						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
								end_point = cv::Point(x, left_y);
								lose_flag = false;
								x_tmp = x;
								// end_point 변경으로 lane_count 다시 설정
								lane_count = (end_point.x - start_point.x);
								break;
							}
							else x--;
						}
					}

					// 그 다음 행으로 이동(위로 이동)
					left_y--;
				}

				// 첫번째 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
				if (start_point.x != 0)
				{
					// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
					// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

					// 출력
					std::cout << "start_point: " << start_point << std::endl;
					std::cout << "end_point: " << end_point << std::endl;
					std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
					std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

					// 가짜 곡선 flag 변수 선언
					bool fake_flag = false;
					// 진짜 곡선 flag 변수 선언
					bool curve_flag = false;

					// abs(lane_count) >= 4: 곡선
					// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
					if (abs(lane_count) >= 4)
					{
						// 가짜 곡선인지 확인
						// tmp_point_3 값이 있으면 이것과 end_point 비교
						if (tmp_point_3.x != 0)
						{
							// (tmp_point_3.y - end_point.y)의 값이
							// 3보다 크면 곡선
							if ((tmp_point_3.y - end_point.y) > 3)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
						else if (tmp_point_4.x != 0)
						{
							// (tmp_point_4.y - end_point.y)의 값이
							// 2보다 크면 곡선
							if ((tmp_point_4.y - end_point.y) > 2)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3, tmp_point_4 값이 없을 때
						// 기울기의 절대값이 1보다 크면 직선
						// 기울기의 절대값이 1보다 작거나 같으면 곡선
						else
						{
							int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
							if (gradient > 1) fake_flag = true;
							else curve_flag = true;
						}
					}

					// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
					// curve_flag == true: 곡선
					if (curve_flag == true)
					{
						// lane_count >= 4: 우회전
						if (lane_count >= 4)
						{
							// left_lane_start_index 저장
							left_lane_start_index = start_point.x + 15;
						}
						// lane_count <= -4: 좌회전
						else if (lane_count <= -4)
						{
							// left_lane_start_index 저장
							left_lane_start_index = start_point.x + 10;
						}

						// end_point.y 값이 left_y + 1 값과 같지 않다면
						// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
						if (end_point.y != (left_y + 1))
						{
							end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (left_y + 1)));
						}
					}

					// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
					else if (abs(lane_count) <= 3 || fake_flag == true)
					{
						// left_lane_start_index 저장
						left_lane_start_index = start_point.x + 7;

						// lane_count 값을 기준으로 직선의 열을 정한다.
						// lane_count 값이 양수면 start_point를 기준
						if (lane_count > 0)
						{
							start_point.x = end_point.x;
						}
						// lane_count 값이 음수이거나 0이면 end_point를 기준
						else if (lane_count <= 0)
						{
							end_point.x = start_point.x;
						}
					}

					// 포인트 저장
					// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
					start_point.y = 445;
					end_point.y = left_y + 1;
					left_lane.push_back(start_point);
					left_lane.push_back(end_point);
				}
			}

			// 두번째부터의 슬라이딩 윈도우
			else
			{
				// 첫번째 슬라이딩 윈도우에서 못 찾으면 그 다음부터 찾지 않는다. 또는
				// 세번째 슬라이딩 윈도우부터 그 전의 슬라이딩 윈도우 못 찾으면
				// 그 다음부터 찾지 않는다.
				if (left_lane.empty() || (left_lane.size() != (i * 2)))
				{
					break;
				}
				// 슬라이딩 윈도우에서 찾았으면 그 다음 윈도우를 검색
				else
				{
					// 이전의 윈도우가 직선, 곡선이냐에 따라서 start_index를 설정한다.
					int start_index, index = (2 * i - 2) + 1;
					int judge_num = left_lane[index].x - left_lane[index - 1].x;
					// 직선일 때
					if (judge_num == 0) start_index = left_lane[index].x + 7;
					// 우회전일 때
					else if (judge_num > 0) start_index = left_lane[index].x + 15;
					// 좌회전일 때
					else start_index = left_lane[index].x + 10;

					// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
					// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
					cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

					// start_point 찾으면 flag 올리기
					bool start_flag = false;

					// 중간에 선이 끊기면 flag 올리기
					bool lose_flag = false;

					// 임시 x(열) 값 저장 변수
					int x_tmp;

					// 윈도우당 총 40행이므로 40번 반복
					for (int r = 0; r < left_row; r++)
					{
						// 현재 y(행)의 시작 포인터 변수
						uchar *p = img_lane.ptr<uchar>(left_y);

						// start_point 못 찾았으면 계속 찾는다
						if (start_flag == false)
						{
							// x = left_lane_start_index(열)부터 선 찾기 시작해서 x = x - 20 까지만 찾기
							// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
							int column = 20;
							int x = start_index;
							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
									// end_point도 저장
									start_point = cv::Point(x, left_y);
									end_point = cv::Point(x, left_y);
									start_flag = true;
									x_tmp = x;
									break;
								}
								else x--;
							}
						}
						// start_point 찾았을 때
						else if (start_flag == true && lose_flag == false)
						{
							// 영상의 y(행), x_tmp(열)의 원소값
							// start_point 바로 위의 원소값부터 차례대로 확인
							int data = p[x_tmp];

							// 바로 위의 값이 있다면 그 다음 행으로 이동
							if (data == 255)
							{
								end_point = cv::Point(x_tmp, left_y);
							}

							// 바로 위의 값이 없다면 오른쪽 왼쪽 확인
							else if (data == 0)
							{
								// 바로 위의 값이 없다면 오른쪽부터 확인
								int data_left_01 = p[(x_tmp - 2)];
								int data_left_02 = p[(x_tmp - 1)];
								int data_right_01 = p[(x_tmp + 1)];
								int data_right_02 = p[(x_tmp + 2)];

								// 오른쪽에 있으면 오른쪽으로 이동
								if (data_right_02 == 255)
								{
									lane_count = lane_count + 2;
									x_tmp = x_tmp + 2;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								else if (data_right_01 == 255)
								{
									lane_count = lane_count + 1;
									x_tmp = x_tmp + 1;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								// 오른쪽에 값이 없고 왼쪽에 값이 있으면 왼쪽으로 이동
								else if (data_left_02 == 255)
								{
									lane_count = lane_count - 1;
									x_tmp = x_tmp - 1;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								else if (data_left_01 == 255)
								{
									lane_count = lane_count - 2;
									x_tmp = x_tmp - 2;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
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
							// column, x 변수 선언
							int column, x;

							// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
							if (lane_count > 0)
							{
								// end_point의 x + 20 부터 start_point의 x - 5 까지 검색
								column = (end_point.x + 20) - (start_point.x - 5) + 1;
								x = end_point.x + 20;
							}
							else if (lane_count < 0)
							{
								// start_point의 x + 10 부터 end_point의 x - 5 까지 검색
								column = (start_point.x + 10) - (end_point.x - 5) + 1;
								x = start_point.x + 10;
							}
							else
							{
								// end_point의 x + 5 부터 start_point의 x - 15 까지 검색
								column = 15;
								x = end_point.x + 5;
							}

							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
									end_point = cv::Point(x, left_y);
									lose_flag = false;
									x_tmp = x;
									// end_point 변경으로 lane_count 다시 설정
									lane_count = (end_point.x - start_point.x);
									break;
								}
								else x--;
							}
						}

						// 그 다음 행으로 이동(위로 이동)
						left_y--;
					}

					// 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
					if (start_point.x != 0)
					{
						// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
						// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

						// 출력
						std::cout << "start_point: " << start_point << std::endl;
						std::cout << "end_point: " << end_point << std::endl;
						std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
						std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

						// 가짜 곡선 flag 변수 선언
						bool fake_flag = false;
						// 진짜 곡선 flag 변수 선언
						bool curve_flag = false;

						// abs(lane_count) >= 4: 곡선
						// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
						if (abs(lane_count) >= 4)
						{
							// 가짜 곡선인지 확인
							// tmp_point_3 값이 있으면 이것과 end_point 비교
							if (tmp_point_3.x != 0)
							{
								// (tmp_point_3.y - end_point.y)의 값이
								// 3보다 크면 곡선
								if ((tmp_point_3.y - end_point.y) > 3)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
							else if (tmp_point_4.x != 0)
							{
								// (tmp_point_4.y - end_point.y)의 값이
								// 2보다 크면 곡선
								if ((tmp_point_4.y - end_point.y) > 2)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3, tmp_point_4 값이 없을 때
							// 기울기의 절대값이 1보다 크면 직선
							// 기울기의 절대값이 1보다 작거나 같으면 곡선
							else
							{
								int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
								if (gradient > 1) fake_flag = true;
								else curve_flag = true;
							}
						}

						// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
						// curve_flag == true: 곡선
						if (curve_flag == true)
						{
							// end_point.y 값이 left_y + 1 값과 같지 않다면
							// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
							if (end_point.y != (left_y + 1))
							{
								end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (left_y + 1)));
							}
						}

						// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
						else if (abs(lane_count) <= 3 || fake_flag == true)
						{
							// lane_count 값을 기준으로 직선의 열을 정한다.
							// lane_count 값이 양수면 start_point를 기준
							if (lane_count > 0)
							{
								start_point.x = end_point.x;
							}
							// lane_count 값이 음수이거나 0이면 end_point를 기준
							else if (lane_count <= 0)
							{
								end_point.x = start_point.x;
							}
						}

						// 포인트 저장
						// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
						start_point.y = 445 - (i * 40);
						end_point.y = left_y + 1;
						left_lane.push_back(start_point);
						left_lane.push_back(end_point);
					}
				}
			}

		}

		// Check Left Lane Pixel and Draw Left Lane
		if (left_lane.empty())
		{
			std::cout << "왼쪽 차선 못 찾음" << std::endl;
		}
		else
		{
			std::cout << "왼쪽 차선 찾음" << std::endl;

			// Draw Right Lane
			cv::polylines(img_lane, left_lane, false, cv::Scalar(255), 6);
		}
		// 왼쪽 차선------------끝-----------------------------------------------

		// Show Image
		//cv::imshow("Check Lane Pixel Image", img_check);
		cv::imshow("Lane Detection Image", img_lane);

		// Inverse Perspective 
		/*cv::Mat img_invPers;
		cv::warpPerspective(img_lane, img_invPers, invPers, cv::Size());
		cv::imshow("Inverse Perspective Transform Image", img_invPers);*/

		// Save Frame ------------------------------------------------
		/*count++;
		std::string num = std::to_string(count);
		std::string filename = "D:\\Computer Vision\\LaneDetection\\Video_Frame\\image_" + num + ".bmp";
		cv::imwrite(filename, img_pers);*/

		// WaitKey-------------------------------------------------------------
		// For Video
		int key = cv::waitKey(1);
		// For Image
		//int key = cv::waitKey(0);

		if (key == 27) // ESC
		{
			// Print Key
			//std::cout << "Key Value: " << key << std::endl;

			// Release Resource
			cap.release();
			cv::destroyAllWindows();
			break;
		}
		else if (key == 99) // c (Capture Image)
		{
			std::string path = "D:\\Computer Vision\\LaneDetection\\Video_Frame\\";
			std::string original_file = path + "Original_Image.jpg";
			cv::imwrite(original_file, img);
			std::string gaussian_file = path + "Gaussian_Image.jpg";
			cv::imwrite(gaussian_file, img_gaussian);
			std::string no_light_file = path + "No_Light_Image.jpg";
			cv::imwrite(no_light_file, img_no_light);
			std::string hsv_file = path + "HSV_Image.jpg";
			cv::imwrite(hsv_file, img_or);
			std::string canny_file = path + "Canny_Edge_Image.jpg";
			cv::imwrite(canny_file, img_canny);
			std::string cali_file = path + "Calibration_Image.jpg";
			cv::imwrite(cali_file, img_cali);
			std::string pers_file = path + "Perspective_Image.jpg";
			cv::imwrite(pers_file, img_pers);
			std::string lane_file = path + "Lane_Image.jpg";
			cv::imwrite(lane_file, img_lane);
		}
	}

	return 0;
}
#endif // Main

#ifdef Debug
int main(int argc, char** argv)
{
	cv::Mat img;
	int count = 13;

	/*std::string num = std::to_string(count);
	std::string filename = "d:\\computer vision\\lanedetection\\video_frame\\image_" + num + ".jpg";
	cv::imwrite(filename, img_pers);*/

	int right_lane_start_index = 423;
	int left_lane_start_index = 248;

	while (1)
	{
		std::string num = std::to_string(count);
		std::string filename = "..\\video_frame\\Bird Eye View\\image_" + num + ".jpg";
		img = cv::imread(filename);
		if (img.empty())
		{
			std::cerr << "No Frame\n";
			break;
		}
		cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
		img = img > 50;
		cv::Mat img_check = img.clone();

		// Lane Detection --------------------------------------------------
		// 오른쪽 차선------------시작-----------------------------------------------
		// 윈도우 개수
		int right_window_num = 3;
		// 윈도우당 40행씩
		int right_row = 40;
		// 오른쪽 차선 포인트 저장 변수
		std::vector<cv::Point> right_lane;
		// y = 445(행)부터 시작
		int right_y = 445;

		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < right_window_num; i++)
		{
			// 찾은 선이 직선인지 곡선인지 판별하기 위한 변수
			int lane_count = 0;

			// 첫번째 슬라이딩 윈도우
			if (i == 0)
			{
				// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
				// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
				cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

				// start_point 찾으면 flag 올리기
				bool start_flag = false;

				// 중간에 선이 끊기면 flag 올리기
				bool lose_flag = false;

				// 임시 x(열) 값 저장 변수
				int x_tmp;

				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < right_row; r++)
				{
					// 현재 y(행)의 시작 포인터 변수
					uchar *p = img.ptr<uchar>(right_y);

					// start_point 못 찾았으면 계속 찾는다
					if (start_flag == false)
					{
						// x = right_lane_start_index(열)부터 선 찾기 시작해서 x = x + 20 까지만 찾기
						// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
						int column = 20;
						int x = right_lane_start_index;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
								// end_point도 저장
								start_point = cv::Point(x, right_y);
								end_point = cv::Point(x, right_y);
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
						// 영상의 y(행), x_tmp(열)의 원소값
						// start_point 바로 위의 원소값부터 차례대로 확인
						int data = p[x_tmp];

						// 바로 위의 값이 있다면 그 다음 행으로 이동
						if (data == 255)
						{
							end_point = cv::Point(x_tmp, right_y);
						}

						// 바로 위의 값이 없다면 왼쪽 오른쪽 확인
						else if(data == 0)
						{
							// 바로 위의 값이 없다면 왼쪽부터 확인
							int data_left_01 = p[(x_tmp - 2)];
							int data_left_02 = p[(x_tmp - 1)];
							int data_right_01 = p[(x_tmp + 1)];
							int data_right_02 = p[(x_tmp + 2)];

							// 왼쪽에 값이 있으면 왼쪽으로 이동
							if (data_left_01 == 255)
							{
								lane_count = lane_count - 2;
								x_tmp = x_tmp - 2;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}
							else if (data_left_02 == 255)
							{
								lane_count = lane_count - 1;
								x_tmp = x_tmp - 1;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}

							// 왼쪽에 값이 없고 오른쪽에 있으면 오른쪽으로 이동
							else if (data_right_01 == 255)
							{
								lane_count = lane_count + 1;
								x_tmp = x_tmp + 1;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
							}
							else if (data_right_02 == 255)
							{
								lane_count = lane_count + 2;
								x_tmp = x_tmp + 2;
								end_point = cv::Point(x_tmp, right_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, right_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, right_y);
								}
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
						// column, x 변수 선언
						int column, x;

						// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
						if (lane_count > 0)
						{
							// start_point의 x - 10 부터 end_point의 x + 5 까지 검색
							column = (end_point.x + 5) - (start_point.x - 10) + 1;
							x = start_point.x - 10;
						}
						else if (lane_count < 0)
						{
							// end_point의 x - 20 부터 start_point의 x + 5 까지 검색
							column = (start_point.x + 5) - (end_point.x - 20) + 1;
							x = end_point.x - 20;
						}
						else
						{
							// end_point의 x - 5 부터 start_point의 x + 5 까지 검색
							column = 15;
							x = end_point.x - 5;
						}
						
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
								end_point = cv::Point(x, right_y);
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
					right_y--;
				}

				// 첫번째 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
				if (start_point.x != 0)
				{
					// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
					// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

					// 출력
					std::cout << "start_point: " << start_point << std::endl;
					std::cout << "end_point: " << end_point << std::endl;
					std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
					std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

					// 가짜 곡선 flag 변수 선언
					bool fake_flag = false;
					// 진짜 곡선 flag 변수 선언
					bool curve_flag = false;

					// abs(lane_count) >= 4: 곡선
					// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
					if (abs(lane_count) >= 4)
					{
						// 가짜 곡선인지 확인
						// tmp_point_3 값이 있으면 이것과 end_point 비교
						if (tmp_point_3.x != 0)
						{
							// (tmp_point_3.y - end_point.y)의 값이
							// 3보다 크면 곡선
							if ((tmp_point_3.y - end_point.y) > 3)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
						else if (tmp_point_4.x != 0)
						{
							// (tmp_point_4.y - end_point.y)의 값이
							// 2보다 크면 곡선
							if ((tmp_point_4.y - end_point.y) > 2)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3, tmp_point_4 값이 없을 때
						// 기울기의 절대값이 1보다 크면 직선
						// 기울기의 절대값이 1보다 작거나 같으면 곡선
						else
						{
							int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
							if (gradient > 1) fake_flag = true;
							else curve_flag = true;
						}
					}

					// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
					// curve_flag == true: 곡선
					if (curve_flag == true)
					{
						// lane_count >= 4: 우회전
						if (lane_count >= 4)
						{
							// right_lane_start_index 저장
							right_lane_start_index = start_point.x - 10;
						}
						// lane_count <= -4: 좌회전
						else if (lane_count <= -4)
						{
							// right_lane_start_index 저장
							right_lane_start_index = start_point.x - 15;
						}

						// end_point.y 값이 right_y + 1 값과 같지 않다면
						// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
						if (end_point.y != (right_y + 1))
						{
							end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (right_y + 1)));
						}
					}

				    // abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
					else if (abs(lane_count) <= 3 || fake_flag == true)
					{
						// right_lane_start_index 저장
						right_lane_start_index = start_point.x - 7;

						// lane_count 값을 기준으로 직선의 열을 정한다.
						// lane_count 값이 양수면 start_point를 기준
						if (lane_count > 0)
						{
							end_point.x = start_point.x;
						}
						// lane_count 값이 음수이거나 0이면 end_point를 기준
						else if (lane_count <= 0)
						{
							start_point.x = end_point.x;
						}
					}

					// 포인트 저장
					// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
					start_point.y = 445;
					end_point.y = right_y + 1;
					right_lane.push_back(start_point);
					right_lane.push_back(end_point);
				}
			}

			// 두번째부터의 슬라이딩 윈도우
			else
			{
				// 첫번째 슬라이딩 윈도우에서 못 찾으면 그 다음부터 찾지 않는다. 또는
				// 세번째 슬라이딩 윈도우부터 그 전의 슬라이딩 윈도우 못 찾으면
				// 그 다음부터 찾지 않는다.
				if (right_lane.empty() || (right_lane.size() != (i * 2)))
				{
					break;
				}
				// 슬라이딩 윈도우에서 찾았으면 그 다음 윈도우를 검색
				else
				{
					// 이전의 윈도우가 직선, 곡선이냐에 따라서 start_index를 설정한다.
					int start_index, index = (2*i - 2) + 1;
					int judge_num = right_lane[index].x - right_lane[index - 1].x;
					// 직선일 때
					if (judge_num == 0) start_index = right_lane[index].x - 7;
					// 우회전일 때
					else if (judge_num > 0) start_index = right_lane[index].x - 10;
					// 좌회전일 때
					else start_index = right_lane[index].x - 15;

					// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
					// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
					cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

					// start_point 찾으면 flag 올리기
					bool start_flag = false;

					// 중간에 선이 끊기면 flag 올리기
					bool lose_flag = false;

					// 임시 x(열) 값 저장 변수
					int x_tmp;

					// 윈도우당 총 40행이므로 40번 반복
					for (int r = 0; r < right_row; r++)
					{
						// 현재 y(행)의 시작 포인터 변수
						uchar *p = img.ptr<uchar>(right_y);

						// start_point 못 찾았으면 계속 찾는다
						if (start_flag == false)
						{
							// x = start_index(열)부터 선 찾기 시작해서 x = x + 20 까지만 찾기
							// 행은 윈도우당 40개씩
							int column = 20;
							int x = start_index;
							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
									// end_point도 저장
									start_point = cv::Point(x, right_y);
									end_point = cv::Point(x, right_y);
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
							// 영상의 y(행), x_tmp(열)의 원소값
							// start_point 바로 위의 원소값부터 차례대로 확인
							int data = p[x_tmp];

							// 바로 위의 값이 있다면 그 다음 행으로 이동
							if (data == 255)
							{
								end_point = cv::Point(x_tmp, right_y);
							}

							// 바로 위의 값이 없다면 왼쪽 오른쪽 확인
							else if (data == 0)
							{
								// 바로 위의 값이 없다면 왼쪽부터 확인
								int data_left_01 = p[(x_tmp - 2)];
								int data_left_02 = p[(x_tmp - 1)];
								int data_right_01 = p[(x_tmp + 1)];
								int data_right_02 = p[(x_tmp + 2)];

								// 왼쪽에 값이 있으면 왼쪽으로 이동
								if (data_left_01 == 255)
								{
									lane_count = lane_count - 2;
									x_tmp = x_tmp - 2;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}
								else if (data_left_02 == 255)
								{
									lane_count = lane_count - 1;
									x_tmp = x_tmp - 1;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}

								// 왼쪽에 값이 없고 오른쪽에 있으면 오른쪽으로 이동
								else if (data_right_01 == 255)
								{
									lane_count = lane_count + 1;
									x_tmp = x_tmp + 1;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
								}
								else if (data_right_02 == 255)
								{
									lane_count = lane_count + 2;
									x_tmp = x_tmp + 2;
									end_point = cv::Point(x_tmp, right_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, right_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, right_y);
									}
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
							// column, x 변수 선언
							int column, x;

							// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
							if (lane_count > 0)
							{
								// start_point의 x - 10 부터 end_point의 x + 5 까지 검색
								column = (end_point.x + 5) - (start_point.x - 10) + 1;
								x = start_point.x - 10;
							}
							else if (lane_count < 0)
							{
								// end_point의 x - 20 부터 start_point의 x + 5 까지 검색
								column = (start_point.x + 5) - (end_point.x - 20) + 1;
								x = end_point.x - 20;
							}
							else
							{
								// end_point의 x - 5 부터 start_point의 x + 5 까지 검색
								column = 15;
								x = end_point.x - 5;
							}

							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
									end_point = cv::Point(x, right_y);
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
						right_y--;
					}

					// 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
					if (start_point.x != 0)
					{
						// 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
						// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

						// 출력
						std::cout << "start_point: " << start_point << std::endl;
						std::cout << "end_point: " << end_point << std::endl;
						std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
						std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

						// 가짜 곡선 flag 변수 선언
						bool fake_flag = false;
						// 진짜 곡선 flag 변수 선언
						bool curve_flag = false;

						// abs(lane_count) >= 4: 곡선
						// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
						if (abs(lane_count) >= 4)
						{
							// 가짜 곡선인지 확인
							// tmp_point_3 값이 있으면 이것과 end_point 비교
							if (tmp_point_3.x != 0)
							{
								// (tmp_point_3.y - end_point.y)의 값이
								// 3보다 크면 곡선
								if ((tmp_point_3.y - end_point.y) > 3)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
							else if (tmp_point_4.x != 0)
							{
								// (tmp_point_4.y - end_point.y)의 값이
								// 2보다 크면 곡선
								if ((tmp_point_4.y - end_point.y) > 2)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3, tmp_point_4 값이 없을 때
							// 기울기의 절대값이 1보다 크면 직선
							// 기울기의 절대값이 1보다 작거나 같으면 곡선
							else
							{
								int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
								if (gradient > 1) fake_flag = true;
								else curve_flag = true;
							}
						}

						// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
						// curve_flag == true: 곡선
						if (curve_flag == true)
						{
							// end_point.y 값이 right_y + 1 값과 같지 않다면
							// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
							if (end_point.y != (right_y + 1))
							{
								end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (right_y + 1)));
							}
						}

						// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
						else if (abs(lane_count) <= 3 || fake_flag == true)
						{
							// lane_count 값을 기준으로 직선의 열을 정한다.
							// lane_count 값이 양수면 start_point를 기준
							if (lane_count > 0)
							{
								end_point.x = start_point.x;
							}
							// lane_count 값이 음수이거나 0이면 end_point를 기준
							else if (lane_count <= 0)
							{
								start_point.x = end_point.x;
							}
						}

						// 포인트 저장
						// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
						start_point.y = 445 - (i * right_row);
						end_point.y = right_y + 1;
						right_lane.push_back(start_point);
						right_lane.push_back(end_point);
					}
				}
			}
		}

		// Check Right Lane Pixel and Draw Right Lane
		if (right_lane.empty())
		{
			std::cout << "오른쪽 차선 없음(못 찾음)" << std::endl;
			int x = right_lane_start_index;
			cv::Rect right_01(x, 406, 30, 40);
			cv::Mat lane_pixel_01 = img(right_01);
			cv::rectangle(img_check, right_01, cv::Scalar(255), 1);
			std::cout << lane_pixel_01 << std::endl;
			std::cout << "--------------------------------------" << std::endl;
		}
		else
		{
			std::cout << "오른쪽 차선 찾음" << std::endl;
			int x;
			// 직선
			if (right_lane[0].x == right_lane[1].x)
			{
				x = right_lane[0].x;
			}
			// 곡선
			else
			{
				// 우회전
				if (right_lane[0].x < right_lane[1].x)
				{
					x = right_lane[0].x;
				}
				// 좌회전
				else
				{
					x = right_lane[1].x;
				}
			}

			cv::Rect right_01(x, 406, 30, 40);
			cv::Mat lane_pixel_01 = img(right_01);
			cv::rectangle(img_check, right_01, cv::Scalar(255), 1);
			std::cout << lane_pixel_01 << std::endl;
			std::cout << "--------------------------------------" << std::endl;

			// Draw Right Lane
			cv::polylines(img, right_lane, false, cv::Scalar(255), 4);
		}
		// 오른쪽 차선------------끝-----------------------------------------------

		// 왼쪽 차선------------시작-----------------------------------------------
		// 윈도우 개수
		int left_window_num = 3;
		// 윈도우당 40행씩
		int left_row = 40;
		// 오른쪽 차선 포인트 저장 변수
		std::vector<cv::Point> left_lane;
		// y = 445(행)부터 시작
		int left_y = 445;

		// 슬라이딩 윈도우 개수만큼 반복
		for (int i = 0; i < left_window_num; i++)
		{
			// 찾은 선이 직선인지 곡선인지 판별하기 위한 변수
			int lane_count = 0;

			// 첫번째 슬라이딩 윈도우
			if (i == 0)
			{
				// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
				// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
				cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

				// start_point 찾으면 flag 올리기
				bool start_flag = false;

				// 중간에 선이 끊기면 flag 올리기
				bool lose_flag = false;

				// 임시 x(열) 값 저장 변수
				int x_tmp;

				// 윈도우당 총 40행이므로 40번 반복
				for (int r = 0; r < left_row; r++)
				{
					// 현재 y(행)의 시작 포인터 변수
					uchar *p = img.ptr<uchar>(left_y);

					// start_point 못 찾았으면 계속 찾는다
					if (start_flag == false)
					{
						// x = left_lane_start_index(열)부터 선 찾기 시작해서 x = x - 20 까지만 찾기
						// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
						int column = 20;
						int x = left_lane_start_index;
						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
								// end_point도 저장
								start_point = cv::Point(x, left_y);
								end_point = cv::Point(x, left_y);
								start_flag = true;
								x_tmp = x;
								break;
							}
							else x--;
						}
					}
					// start_point 찾았을 때
					else if (start_flag == true && lose_flag == false)
					{
						// 영상의 y(행), x_tmp(열)의 원소값
						// start_point 바로 위의 원소값부터 차례대로 확인
						int data = p[x_tmp];

						// 바로 위의 값이 있다면 그 다음 행으로 이동
						if (data == 255)
						{
							end_point = cv::Point(x_tmp, left_y);
						}

						// 바로 위의 값이 없다면 오른쪽 왼쪽 확인
						else if (data == 0)
						{
							// 바로 위의 값이 없다면 오른쪽부터 확인
							int data_left_01 = p[(x_tmp - 2)];
							int data_left_02 = p[(x_tmp - 1)];
							int data_right_01 = p[(x_tmp + 1)];
							int data_right_02 = p[(x_tmp + 2)];

							// 오른쪽에 있으면 오른쪽으로 이동
							if (data_right_02 == 255)
							{
								lane_count = lane_count + 2;
								x_tmp = x_tmp + 2;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							else if (data_right_01 == 255)
							{
								lane_count = lane_count + 1;
								x_tmp = x_tmp + 1;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							// 오른쪽에 값이 없고 왼쪽에 값이 있으면 왼쪽으로 이동
							else if (data_left_02 == 255)
							{
								lane_count = lane_count - 1;
								x_tmp = x_tmp - 1;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
							}
							else if (data_left_01 == 255)
							{
								lane_count = lane_count - 2;
								x_tmp = x_tmp - 2;
								end_point = cv::Point(x_tmp, left_y);

								// 임시 포인트 저장
								// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
								if (abs(lane_count) == 3)
								{
									tmp_point_3 = cv::Point(x_tmp, left_y);
								}
								else if (abs(lane_count) == 4)
								{
									tmp_point_4 = cv::Point(x_tmp, left_y);
								}
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
						// column, x 변수 선언
						int column, x;

						// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
						if (lane_count > 0)
						{
							// end_point의 x + 20 부터 start_point의 x - 5 까지 검색
							column = (end_point.x + 20) - (start_point.x - 5) + 1;
							x = end_point.x + 20;
						}
						else if (lane_count < 0)
						{
							// start_point의 x + 10 부터 end_point의 x - 5 까지 검색
							column = (start_point.x + 10) - (end_point.x - 5) + 1;
							x = start_point.x + 10;
						}
						else
						{
							// end_point의 x + 5 부터 start_point의 x - 15 까지 검색
							column = 15;
							x = end_point.x + 5;
						}

						for (int c = 0; c < column; c++)
						{
							// 영상의 y(행), x(열)의 원소값
							int data = p[x];
							if (data == 255)
							{
								// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
								end_point = cv::Point(x, left_y);
								lose_flag = false;
								x_tmp = x;
								// end_point 변경으로 lane_count 다시 설정
								lane_count = (end_point.x - start_point.x);
								break;
							}
							else x--;
						}
					}

					// 그 다음 행으로 이동(위로 이동)
					left_y--;
				}

				// 첫번째 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
				if (start_point.x != 0)
				{
					// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
					// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

					// 출력
					std::cout << "start_point: " << start_point << std::endl;
					std::cout << "end_point: " << end_point << std::endl;
					std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
					std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

					// 가짜 곡선 flag 변수 선언
					bool fake_flag = false;
					// 진짜 곡선 flag 변수 선언
					bool curve_flag = false;

					// abs(lane_count) >= 4: 곡선
					// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
					if (abs(lane_count) >= 4)
					{
						// 가짜 곡선인지 확인
						// tmp_point_3 값이 있으면 이것과 end_point 비교
						if (tmp_point_3.x != 0)
						{
							// (tmp_point_3.y - end_point.y)의 값이
							// 3보다 크면 곡선
							if ((tmp_point_3.y - end_point.y) > 3)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
						else if (tmp_point_4.x != 0)
						{
							// (tmp_point_4.y - end_point.y)의 값이
							// 2보다 크면 곡선
							if ((tmp_point_4.y - end_point.y) > 2)
							{
								curve_flag = true;
							}
							else
							{
								fake_flag = true;
							}
						}
						// tmp_point_3, tmp_point_4 값이 없을 때
						// 기울기의 절대값이 1보다 크면 직선
						// 기울기의 절대값이 1보다 작거나 같으면 곡선
						else
						{
							int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
							if (gradient > 1) fake_flag = true;
							else curve_flag = true;
						}
					}

					// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
					// curve_flag == true: 곡선
					if (curve_flag == true)
					{
						// lane_count >= 4: 우회전
						if (lane_count >= 4)
						{
							// left_lane_start_index 저장
							left_lane_start_index = start_point.x + 15;
						}
						// lane_count <= -4: 좌회전
						else if (lane_count <= -4)
						{
							// left_lane_start_index 저장
							left_lane_start_index = start_point.x + 10;
						}

						// end_point.y 값이 left_y + 1 값과 같지 않다면
						// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
						if (end_point.y != (left_y + 1))
						{
							end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (left_y + 1)));
						}
					}

					// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
					else if (abs(lane_count) <= 3 || fake_flag == true)
					{
						// left_lane_start_index 저장
						left_lane_start_index = start_point.x + 7;

						// lane_count 값을 기준으로 직선의 열을 정한다.
						// lane_count 값이 양수면 start_point를 기준
						if (lane_count > 0)
						{
							start_point.x = end_point.x;
						}
						// lane_count 값이 음수이거나 0이면 end_point를 기준
						else if (lane_count <= 0)
						{
							end_point.x = start_point.x;
						}
					}

					// 포인트 저장
					// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
					start_point.y = 445;
					end_point.y = left_y + 1;
					left_lane.push_back(start_point);
					left_lane.push_back(end_point);
				}
			}

			// 두번째부터의 슬라이딩 윈도우
			else
			{
				// 첫번째 슬라이딩 윈도우에서 못 찾으면 그 다음부터 찾지 않는다. 또는
				// 세번째 슬라이딩 윈도우부터 그 전의 슬라이딩 윈도우 못 찾으면
				// 그 다음부터 찾지 않는다.
				if (left_lane.empty() || (left_lane.size() != (i * 2)))
				{
					break;
				}
				// 슬라이딩 윈도우에서 찾았으면 그 다음 윈도우를 검색
				else
				{
					// 이전의 윈도우가 직선, 곡선이냐에 따라서 start_index를 설정한다.
					int start_index, index = (2 * i - 2) + 1;
					int judge_num = left_lane[index].x - left_lane[index - 1].x;
					// 직선일 때
					if (judge_num == 0) start_index = left_lane[index].x + 7;
					// 우회전일 때
					else if (judge_num > 0) start_index = left_lane[index].x + 15;
					// 좌회전일 때
					else start_index = left_lane[index].x + 10;

					// 찾은 선의 첫번째 포인트와 마지막 포인트 그리고 임시 포인트 변수 선언
					// 초기값 없이 변수 선언하면 Point는 (0, 0)으로 자동으로 초기화 되어있다.
					cv::Point start_point, end_point, tmp_point_3, tmp_point_4;

					// start_point 찾으면 flag 올리기
					bool start_flag = false;

					// 중간에 선이 끊기면 flag 올리기
					bool lose_flag = false;

					// 임시 x(열) 값 저장 변수
					int x_tmp;

					// 윈도우당 총 40행이므로 40번 반복
					for (int r = 0; r < left_row; r++)
					{
						// 현재 y(행)의 시작 포인터 변수
						uchar *p = img.ptr<uchar>(left_y);

						// start_point 못 찾았으면 계속 찾는다
						if (start_flag == false)
						{
							// x = left_lane_start_index(열)부터 선 찾기 시작해서 x = x - 20 까지만 찾기
							// y = 445(행)부터 시작해서 y = 445 - 40까지만 찾기(행은 윈도우당 40개씩)
							int column = 20;
							int x = start_index;
							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 처음으로 start_point 찾았으므로 저장 및 flag 올리기
									// end_point도 저장
									start_point = cv::Point(x, left_y);
									end_point = cv::Point(x, left_y);
									start_flag = true;
									x_tmp = x;
									break;
								}
								else x--;
							}
						}
						// start_point 찾았을 때
						else if (start_flag == true && lose_flag == false)
						{
							// 영상의 y(행), x_tmp(열)의 원소값
							// start_point 바로 위의 원소값부터 차례대로 확인
							int data = p[x_tmp];

							// 바로 위의 값이 있다면 그 다음 행으로 이동
							if (data == 255)
							{
								end_point = cv::Point(x_tmp, left_y);
							}

							// 바로 위의 값이 없다면 오른쪽 왼쪽 확인
							else if (data == 0)
							{
								// 바로 위의 값이 없다면 오른쪽부터 확인
								int data_left_01 = p[(x_tmp - 2)];
								int data_left_02 = p[(x_tmp - 1)];
								int data_right_01 = p[(x_tmp + 1)];
								int data_right_02 = p[(x_tmp + 2)];

								// 오른쪽에 있으면 오른쪽으로 이동
								if (data_right_02 == 255)
								{
									lane_count = lane_count + 2;
									x_tmp = x_tmp + 2;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								else if (data_right_01 == 255)
								{
									lane_count = lane_count + 1;
									x_tmp = x_tmp + 1;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								// 오른쪽에 값이 없고 왼쪽에 값이 있으면 왼쪽으로 이동
								else if (data_left_02 == 255)
								{
									lane_count = lane_count - 1;
									x_tmp = x_tmp - 1;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
								}
								else if (data_left_01 == 255)
								{
									lane_count = lane_count - 2;
									x_tmp = x_tmp - 2;
									end_point = cv::Point(x_tmp, left_y);

									// 임시 포인트 저장
									// 곡선이 진짜 곡선인지 가짜 곡선인지 판별하기 위한 변수
									if (abs(lane_count) == 3)
									{
										tmp_point_3 = cv::Point(x_tmp, left_y);
									}
									else if (abs(lane_count) == 4)
									{
										tmp_point_4 = cv::Point(x_tmp, left_y);
									}
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
							// column, x 변수 선언
							int column, x;

							// lane_count 값이 양수, 음수, 0에 따라 column, x값 변경
							if (lane_count > 0)
							{
								// end_point의 x + 20 부터 start_point의 x - 5 까지 검색
								column = (end_point.x + 20) - (start_point.x - 5) + 1;
								x = end_point.x + 20;
							}
							else if (lane_count < 0)
							{
								// start_point의 x + 10 부터 end_point의 x - 5 까지 검색
								column = (start_point.x + 10) - (end_point.x - 5) + 1;
								x = start_point.x + 10;
							}
							else
							{
								// end_point의 x + 5 부터 start_point의 x - 15 까지 검색
								column = 15;
								x = end_point.x + 5;
							}

							for (int c = 0; c < column; c++)
							{
								// 영상의 y(행), x(열)의 원소값
								int data = p[x];
								if (data == 255)
								{
									// 다시 end_point 찾았으므로 저장 및 lose_flag 내리기
									end_point = cv::Point(x, left_y);
									lose_flag = false;
									x_tmp = x;
									// end_point 변경으로 lane_count 다시 설정
									lane_count = (end_point.x - start_point.x);
									break;
								}
								else x--;
							}
						}

						// 그 다음 행으로 이동(위로 이동)
						left_y--;
					}

					// 슬라이딩 윈도우에서 선 못 찾았으면 저장 X
					if (start_point.x != 0)
					{
						// 첫번째 슬라이딩 윈도우 포인트 저장 및 직선인지 곡선인지 판별
						// 먼저 곡선인지 확인한다. 왜냐햐면 곡선이 가짜 곡선일 수 있기 때문이다.

						// 출력
						std::cout << "start_point: " << start_point << std::endl;
						std::cout << "end_point: " << end_point << std::endl;
						std::cout << "tmp_point_3: " << tmp_point_3 << std::endl;
						std::cout << "tmp_point_4: " << tmp_point_4 << std::endl;

						// 가짜 곡선 flag 변수 선언
						bool fake_flag = false;
						// 진짜 곡선 flag 변수 선언
						bool curve_flag = false;

						// abs(lane_count) >= 4: 곡선
						// 곡선이 진짜 곡선인지 가짜 곡선인지부터 판별
						if (abs(lane_count) >= 4)
						{
							// 가짜 곡선인지 확인
							// tmp_point_3 값이 있으면 이것과 end_point 비교
							if (tmp_point_3.x != 0)
							{
								// (tmp_point_3.y - end_point.y)의 값이
								// 3보다 크면 곡선
								if ((tmp_point_3.y - end_point.y) > 3)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3 값이 없고 tmp_point_4 값이 있으면 이것과 end_point 비교
							else if (tmp_point_4.x != 0)
							{
								// (tmp_point_4.y - end_point.y)의 값이
								// 2보다 크면 곡선
								if ((tmp_point_4.y - end_point.y) > 2)
								{
									curve_flag = true;
								}
								else
								{
									fake_flag = true;
								}
							}
							// tmp_point_3, tmp_point_4 값이 없을 때
							// 기울기의 절대값이 1보다 크면 직선
							// 기울기의 절대값이 1보다 작거나 같으면 곡선
							else
							{
								int gradient = (int)((start_point.y - end_point.y) / abs(start_point.x - end_point.x));
								if (gradient > 1) fake_flag = true;
								else curve_flag = true;
							}
						}

						// flag로 곡선인지 직선인지 판별 후 각자 맞게 포인트 설정
						// curve_flag == true: 곡선
						if (curve_flag == true)
						{
							// end_point.y 값이 left_y + 1 값과 같지 않다면
							// 기울기 값을 계산해서 end_point.x 값을 보정해야 한다.
							if (end_point.y != (left_y + 1))
							{
								end_point.x = start_point.x + (int)(((end_point.x - start_point.x) / (start_point.y - end_point.y)) * (start_point.y - (left_y + 1)));
							}
						}

						// abs(lane_count) <= 3 또는 가짜 곡선일 때: 직선
						else if (abs(lane_count) <= 3 || fake_flag == true)
						{
							// lane_count 값을 기준으로 직선의 열을 정한다.
							// lane_count 값이 양수면 start_point를 기준
							if (lane_count > 0)
							{
								start_point.x = end_point.x;
							}
							// lane_count 값이 음수이거나 0이면 end_point를 기준
							else if (lane_count <= 0)
							{
								end_point.x = start_point.x;
							}
						}

						// 포인트 저장
						// start_point.y, end_point.y 값은 현재 윈도우의 각 끝으로 이동
						start_point.y = 445 - (i * 40);
						end_point.y = left_y + 1;
						left_lane.push_back(start_point);
						left_lane.push_back(end_point);
					}
				}
			}

		}

		// Check Left Lane Pixel and Draw Left Lane
		if (left_lane.empty())
		{
			std::cout << "왼쪽 차선 없음(못 찾음)" << std::endl;
			int x = left_lane_start_index - 29;
			cv::Rect left_01(x, 406, 30, 40);
			cv::Mat lane_pixel_01 = img(left_01);
			cv::rectangle(img_check, left_01, cv::Scalar(255), 1);
			std::cout << lane_pixel_01 << std::endl;
			std::cout << "--------------------------------------" << std::endl;
		}
		else
		{
			std::cout << "왼쪽 차선 찾음" << std::endl;
			int x;
			// 직선
			if (left_lane[0].x == left_lane[1].x)
			{
				x = left_lane[0].x - 29;
			}
			// 곡선
			else
			{
				// 우회전
				if (left_lane[0].x < left_lane[1].x)
				{
					x = left_lane[1].x - 29;
				}
				// 좌회전
				else
				{
					x = left_lane[0].x - 29;
				}
			}

			cv::Rect left_01(x, 406, 30, 40);
			cv::Mat lane_pixel_01 = img(left_01);
			cv::rectangle(img_check, left_01, cv::Scalar(255), 1);
			std::cout << lane_pixel_01 << std::endl;
			std::cout << "--------------------------------------" << std::endl;

			// Draw Right Lane
			cv::polylines(img, left_lane, false, cv::Scalar(255), 4);
		}
		// 왼쪽 차선------------끝-----------------------------------------------

		// 중간값 출력
		/*std::cout << "left_x: " << left_lane[0].x << ", right_x: " << right_lane[0].x << std::endl;
		std::cout << "center_x: " << (int)((left_lane[0].x + right_lane[0].x) / 2) << std::endl;
		std::cout << "width: " << (left_lane[0].x + right_lane[0].x) << std::endl;*/

		// Show Image
		cv::imshow("Check Lane Pixel Image", img_check);
		cv::imshow("Check Lane", img);
		

		// Check Lane Pixel--------------------------------------
		/*cv::Rect right_01(415, 405, 20, 40);
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
		std::cout << "--------------------------------------" << std::endl;*/

		// Test Code
		std::vector<cv::Point> test;
		cv::Point p1 = cv::Point(1, 1);
		cv::Point p2 = cv::Point(2, 2);
		test.push_back(p1);
		test.push_back(p2);
		std::cout << "test.size(): " << test.size() << std::endl;


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

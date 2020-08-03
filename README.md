# Lane Detection
<div>
<img src="https://user-images.githubusercontent.com/55565351/87013566-ce4d0300-c205-11ea-9e0e-f909223a6efc.jpg" width="300" height="300"/>
<img src="https://user-images.githubusercontent.com/55565351/87013575-d016c680-c205-11ea-959f-a0c94a8997f4.jpg" width="300" height="300"/>
</div>

## 프로젝트 개요
* OpenCV를 이용한 Lane Detection

## 개발 환경
제목 | 내용
--------- | --------
OS | Windows 10 Pro
언어 | C++
카메라 | Pi Camera V2 + IMX219-D160
라이브러리 | OpenCV 4.2.0
통합 개발 환경 | Visual Studio 2017

## 차선 인식 과정
* **1st: 가우시안 블러링**
  + <이유>
     + 초점이 맞지 않은 사진처럼 영상을 부드럽게 만드는 필터링 기법, 잡음의 영향을 제거하는 전처리 과정으로 사용
  + <함수>
     + cv::GaussianBlur();
     
* **2nd: 색 공간 변환(BGR to Lab)**
  + <이유>
     + 조명의 영향을 제거하기 위해 사용
  + <함수>
     + cv::cvtColor();
     + cv::split();
     + cv::bitwise_not();
     + cv::medianBlur();
     + cv::add();
     + cv::merge();
     
* **3rd: 색 공간 변환(BGR to HSV)**
  + <이유>
     + 노란색 차선과 흰색 차선만 필터링하기 위해 사용
  + <함수>
     + cv::cvtColor();
     + cv::inRange();
     + cv::bitwise_or();
     
* **4th: 캐니 에지 검출기**
  + <이유>
     + 차선의 에지만 검출하기 위해 사용
  + <함수>
     + cv::Canny();
     
* **5th: 카메라 왜곡 보정**
  + <이유>
     + 160도 광각렌즈의 사용으로 영상에 왜곡이 있어 이를 보정하기 위해 사용
  + <함수>
     + cv::initUndistortRectifyMap();
     + cv::remap();
     
* **6th: 투시 변환**
  + <이유>
     + 영상의 원근감을 왜곡시켜 차선을 마치 하늘에서 바라보는 듯한 구조로 변환하기 위해 사용
  + <함수>
     + cv::getPerspectiveTransform();
     + cv::warpPerspective();

* **참고**
  + https://blog.naver.com/hirit808/221486800161
  + https://t9t9.com/60

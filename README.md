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
* **0st: 원본 이미지**
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621391-a2825b80-e24e-11ea-917e-e9aa5e692c2a.jpg" width="320" height="240"/>
       </div>
     
* **1st: 가우시안 블러링**
  + <이유>
     + 초점이 맞지 않은 사진처럼 영상을 부드럽게 만드는 필터링 기법, 잡음의 영향을 제거하는 전처리 과정으로 사용
  + <함수>
     + cv::GaussianBlur();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621479-bf1e9380-e24e-11ea-8f8a-9bb134e72ce7.jpg" width="320" height="240"/>
       </div>
     
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
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621573-dcebf880-e24e-11ea-9179-797bd6f41784.jpg" width="320" height="240"/>
       </div>
     
* **3rd: 색 공간 변환(BGR to HSV)**
  + <이유>
     + 노란색 차선과 흰색 차선만 필터링하기 위해 사용
  + <함수>
     + cv::cvtColor();
     + cv::inRange();
     + cv::bitwise_or();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621646-f5f4a980-e24e-11ea-9122-e06a31654c47.jpg" width="320" height="240"/>
       </div>
       
* **4th: 캐니 에지 검출기**
  + <이유>
     + 차선의 에지만 검출하기 위해 사용
  + <함수>
     + cv::Canny();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621661-fc832100-e24e-11ea-8013-9ea0013f5936.jpg" width="320" height="240"/>
       </div>
     
* **5th: 카메라 왜곡 보정**
  + <이유>
     + 160도 광각렌즈의 사용으로 영상에 왜곡이 있어 이를 보정하기 위해 사용
  + <함수>
     + cv::initUndistortRectifyMap();
     + cv::remap();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621680-03119880-e24f-11ea-9e99-964007d4beee.jpg" width="320" height="240"/>
       </div>
     
* **6th: 투시 변환**
  + <이유>
     + 영상의 원근감을 왜곡시켜 차선을 마치 하늘에서 바라보는 듯한 구조로 변환하기 위해 사용
  + <함수>
     + cv::getPerspectiveTransform();
     + cv::warpPerspective();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621691-060c8900-e24f-11ea-9aa7-ca055f2df359.jpg" width="320" height="240"/>
       </div>
       
* **7th: 차선 인식**
  + <이유>
     + 차선을 인식하기 위한 알고리즘 개발
     + 차선으로 간주되는 픽셀을 찾고 그 픽셀을 기준으로 바로 위의 행으로 이동한다.
     + 그 다음 행에서도 차선으로 간주되는 픽셀을 찾는다. 이와 같은 행동을 40번(40행) 반복
     + 반복이 끝나면 찾은 첫번째 픽셀과 마지막 픽셀의 행과 열을 기준으로 직선인지 곡선인지 판별한다.
     + 직선(곡선)의 포인트를 저장하고 선을 그린다.
  + <함수>
     + cv::Point();
     + cv::Mat::ptr();
     + cv::polylines();
  + <사진>
     + <div>
       <img src="https://user-images.githubusercontent.com/55565351/90621707-086ee300-e24f-11ea-912f-f1eb95235ef0.jpg" width="320" height="240"/>
       <img src="https://user-images.githubusercontent.com/55565351/90624096-64873680-e252-11ea-8135-10f965e45704.jpg" width="320" height="240"/>
       </div>

* **참고**
  + https://blog.naver.com/hirit808/221486800161
  + https://t9t9.com/60

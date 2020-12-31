#pragma once

#include "MvCameraControl.h"
#include <opencv2/opencv.hpp>

// 将vel格式的速度转成PLC的DB块写入速度
void vel2PLC(double vel[3], double* plc_speed) {
	*plc_speed = 27312.2626 * (vel[0] + vel[1] + 0.49 * vel[2]);
	*(plc_speed+1) = 27312.2626 * (vel[0] - vel[1] - 0.49 * vel[2]);
	*(plc_speed + 2) = 27312.2626 * (vel[0] + vel[1] - 0.49 * vel[2]);
	*(plc_speed + 3) = 27312.2626 * (vel[0] - vel[1] + 0.49 * vel[2]);

}

// 将海康摄像头数据转为opencv图像
bool hk2cv(MV_FRAME_OUT_INFO_EX* hk_imginfo, unsigned char* data, cv::Mat& src_img) {
	cv::Mat cv_img;
	if (hk_imginfo->enPixelType == PixelType_Gvsp_Mono8) {
		cv_img = cv::Mat(hk_imginfo->nHeight, hk_imginfo->nWidth, CV_8UC1, data);
	}
	else if (hk_imginfo->enPixelType == PixelType_Gvsp_RGB8_Packed) {


		for (unsigned int j = 0; j < hk_imginfo->nHeight; j++) {
			for (unsigned int i = 0; i < hk_imginfo->nWidth; i++) {
				unsigned char red = data[j * (hk_imginfo->nWidth * 3) + i * 3];
				data[j * (hk_imginfo->nWidth * 3) + i * 3] = data[j * (hk_imginfo->nWidth * 3) + i * 3 + 2];
				data[j * (hk_imginfo->nWidth * 3) + i * 3 + 2] = red;
			}
		}
		cv_img = cv::Mat(hk_imginfo->nHeight, hk_imginfo->nWidth, CV_8UC3, data);
	}
	else {
		printf("unsupported pixel format\n");
		return false;
	}

	if (cv_img.data == NULL) {
		return false;
	}
	cv_img.copyTo(src_img);
	return true;
}

// 黑白循迹图像处理
void processImage(cv::Mat& src_img, cv::Mat& dst_img) {

	cv::Mat blured_img, gray_img, mask;

	cv::GaussianBlur(src_img, blured_img, cv::Size(3, 3), 0);

	cv::Mat hsv_img;
	cv::cvtColor(blured_img, hsv_img, cv::COLOR_RGB2HSV);
	cv::Scalar lower_black(0, 0, 0);
	cv::Scalar uper_black(25, 75, 120);
	cv::inRange(hsv_img, lower_black, uper_black, mask);
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7), cv::Point(-1, -1));
	morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

	//cv::imshow("gray", mask);

	int width = mask.cols;
	int height = mask.rows;

	int search_top = 3 * height / 4;
	int search_bottom = search_top + 20;

	for (int i = 0; i < height - 2; i++) {
		if (i < search_top || i > search_bottom) {
			for (int j = 0; j < width; j++) {
				mask.at<uchar>(i, j) = 0;

			}
		}
	}
	//cv::imshow("mask", mask);
	mask.copyTo(dst_img);
}

void followlane(cv::Mat src_img, double* speed) {
	cv::Mat dst_img;
	processImage(src_img, dst_img);
	cv::Moments moments = cv::moments(dst_img);
	
	if (moments.m00 > 0) {
		int cx = int(moments.m10 / moments.m00);
		int cy = int(moments.m01 / moments.m00);
		cv::circle(src_img, cv::Point(cx, cy), 20, CV_RGB(255, 0, 0), -1);
		
		int err = cx - dst_img.cols / 2;

		*speed = 0.3;
		*(speed+2) = -(double)err / 500;

	}
}
#include "myutil.hpp"
#include "myrtmp.h"
#include "tcpclient.h"
#include "plc_joystick.h"
#include "snap7.h"

#include <sl/Camera.hpp>


#include <thread>

#pragma comment(lib, "snap7.lib")

struct ControlParams {
	int camera_tag;
	double x_speed;
	double y_speed;
	double w_speed;
};

struct StatusParams {
	bool local_control;
	int GPS_data;
	double v_speed;
	double w_speed;
};


// plc的IP地址
const char* plc_ip = "192.168.1.33";
// 服务器地址
const char* server_ip = "81.70.196.142";

// 推流地址
const char* rtmp_addr = "rtmp://81.70.196.142:8090/live";

// 服务器端口
const int tcp_port = 10127;

// 控制量
ControlParams c_params;
// 状态量
StatusParams s_params;

HANDLE s_mutex;

cv::Mat front_img;

cv::Mat back_img;

bool joy_flag, plc_flag, tcp_flag, frontimg_flag, backimg_flag, pushimg_flag;

unsigned int WINAPI pthJoystick(void*);
unsigned int WINAPI pthControlPLC(void*);
unsigned int WINAPI pthTcpclient(void*);
unsigned int WINAPI pthGetFrontImg(void*);
unsigned int WINAPI pthGetBackImg(void*);
unsigned int WINAPI pthPushImage(void*);

int main(int argc, char** argv) {

	memset(&c_params, 0, sizeof(ControlParams));
	memset(&s_params, 0, sizeof(StatusParams));

	c_params.camera_tag = 1;
	joy_flag = plc_flag = tcp_flag = frontimg_flag = backimg_flag = pushimg_flag = true;

	s_mutex = CreateMutex(NULL, false, NULL);
	
	HANDLE pth_joy = (HANDLE)_beginthreadex(NULL, 0, pthJoystick, NULL, 0, NULL);
	HANDLE pth_tcp = (HANDLE)_beginthreadex(NULL, 0, pthTcpclient, NULL, 0, NULL);
	HANDLE pth_plc = (HANDLE)_beginthreadex(NULL, 0, pthControlPLC, NULL, 0, NULL);
	HANDLE pth_frontimg = (HANDLE)_beginthreadex(NULL, 0, pthGetFrontImg, NULL, 0, NULL);
	HANDLE pth_backimg = (HANDLE)_beginthreadex(NULL, 0, pthGetBackImg, NULL, 0, NULL);
	HANDLE pth_pushimg = (HANDLE)_beginthreadex(NULL, 0, pthPushImage, NULL, 0, NULL);

	while (!(_kbhit() && _getch() == 0x1b)){
		
	}
	joy_flag = plc_flag = tcp_flag = frontimg_flag = backimg_flag = pushimg_flag = false;
	Sleep(1000);

	CloseHandle(pth_joy);
	CloseHandle(pth_tcp);
	CloseHandle(pth_plc);
	CloseHandle(pth_frontimg);
	CloseHandle(pth_backimg);
	CloseHandle(pth_pushimg);
	CloseHandle(s_mutex);
	return 0;
}


unsigned int WINAPI pthJoystick(void*) {

	PLCJoystick joystick(JOYSTICKID1);
	double *speed = NULL;
	
	while (joy_flag) {
		if (!joystick.getJsCaps()) {
			continue;
		}
		while (joy_flag) {
			if (!joystick.updateStatus()) {
				break;
			}
			WaitForSingleObject(s_mutex, INFINITE);
			if (!joystick.listen_flag) {
				if (s_params.local_control) {
					s_params.local_control = false;
					memset(&c_params, 0, sizeof(ControlParams));
				}
				ReleaseMutex(s_mutex);
				Sleep(100);
				continue;
			}
			joystick.listenJs();
			if (joystick.getDetectLane()) {
				// 黑白循迹
				if (front_img.empty()) {
					continue;
				}
				cv::Mat src_img;
				front_img.copyTo(src_img);
				followlane(src_img, speed);
				cv::imshow("detect_lane", src_img);
				cv::waitKey(30);
			}
			else {
				speed = joystick.getSpeed();
			}
			c_params.x_speed = speed[0];
			c_params.y_speed = speed[1];
			c_params.w_speed = speed[2];
			if (speed[0] >= 0) {
				c_params.camera_tag = 1;
			}
			else {
				c_params.camera_tag = -1;
			}
			s_params.local_control = true;

			cout << "x_speed:" << speed[0] << "\ty_speed" << speed[1] << "\tw_speed" << speed[2] << endl;
			ReleaseMutex(s_mutex);
			Sleep(50);
		}
		WaitForSingleObject(s_mutex, INFINITE);
		s_params.local_control = false;
		memset(&c_params, 0, sizeof(ControlParams));
		ReleaseMutex(s_mutex);
	}
	return 0;
}

unsigned int WINAPI pthControlPLC(void*) {
	// 定义snap7客户端
	TS7Client* snap7_client = new TS7Client();
	// 连接plc
	int ret = snap7_client->ConnectTo(plc_ip, 0, 0);
	if (ret != 0) {
		cout << "PLC 连接失败！！！" << endl;
		return -1;
	}
	double vel_speed[3], plc_speed[4];
	while (plc_flag) {
		WaitForSingleObject(s_mutex, INFINITE);
		vel_speed[0] = c_params.x_speed;
		vel_speed[1] = c_params.y_speed;
		vel_speed[2] = c_params.w_speed;
		ReleaseMutex(s_mutex);
		vel2PLC(vel_speed, plc_speed);
		//cout << "right_front:" << plc_speed[0] << endl;
		byte buff[8] = { 0 };//创建一个读写缓存区
		for (int i = 0; i < 8; i++) {
			if (i % 2) {
				buff[i] = (byte)(0xff & ((int)plc_speed[i / 2]));
			}
			else {
				buff[i] = (byte)(0xff & ((int)plc_speed[i / 2] >> 8));
			}
		}

		
		// 向PLC写数据（参数分别是DB块，块号，起始地址， 写多少， 写word类型，数据源开始地址）
		snap7_client->AsWriteArea(0x84, 4, 0, 4, 0x04, buff);
	}
	// 断开snap7连接
	snap7_client->Disconnect();
	return 0;
}

unsigned int WINAPI pthTcpclient(void*) {
	TcpClient tcp_client;
	while (tcp_flag) {
		bool ret_tcp = tcp_client.tcpInit(server_ip, tcp_port);
		while (ret_tcp && tcp_flag) {
			WaitForSingleObject(s_mutex, INFINITE);
			if (tcp_client.tcpSend(tcp_client.m_sock, (char*)&s_params, sizeof(StatusParams)) == false) {
				memset(&c_params, 0, sizeof(ControlParams));
				ReleaseMutex(s_mutex);
				break;
			}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
			
			if (s_params.local_control) {
				ReleaseMutex(s_mutex);
				Sleep(1);
				continue;
			}
			char buffer[1024];
			if (tcp_client.tcpRecv(tcp_client.m_sock, buffer) == false) {
				memset(&c_params, 0, sizeof(ControlParams));
				ReleaseMutex(s_mutex);
				break;
			}
			memcpy(&c_params, buffer, sizeof(ControlParams));
			// cout << c_params.left_back_speed << "\t" << c_params.left_front_speed << endl;
			ReleaseMutex(s_mutex);
			Sleep(10);
		}
	}
	return 0;
}

unsigned int WINAPI pthPushImage(void*) {
	
	cv::Mat f_image, b_image, image;
	int width = 1280;
	int height = 720;

	MyRTMP rtmp(width, height, 3, 24);
	

	if (rtmp.initRTMP(rtmp_addr) == false) {
		cout << "推流器初始化失败" << endl;
		return -1;
	}

	while (pushimg_flag) {
		WaitForSingleObject(s_mutex, INFINITE);
		if (c_params.camera_tag == 1) {
			if (front_img.empty()) {
				
				ReleaseMutex(s_mutex);
				continue;
			}
			front_img.copyTo(image);
			
			ReleaseMutex(s_mutex);
			
			
		}
		else if (c_params.camera_tag == -1) {
			if (back_img.empty()) {
				ReleaseMutex(s_mutex);
				continue;
			}
			back_img.copyTo(image);
			ReleaseMutex(s_mutex);
			cv::cvtColor(image, image, cv::COLOR_GRAY2BGRA);
		}
		else {
			
			ReleaseMutex(s_mutex);
			continue;
		}
		cv::resize(image, image, cv::Size(width, height));
		cv::imshow("push", image);
		rtmp.startRTMP(image.data, image.elemSize());
		cv::waitKey(10);
	}
	return 0;
}

unsigned int WINAPI pthGetFrontImg(void*) {
	//定义相机
	sl::Camera zed;
	//初始化相机
	sl::InitParameters init_p;

	init_p.camera_resolution = sl::RESOLUTION::HD720;
	init_p.camera_fps = 25;
	init_p.depth_mode = sl::DEPTH_MODE::NONE;

	sl::ERROR_CODE open_state = zed.open(init_p);
	if (open_state != sl::ERROR_CODE::SUCCESS) {
		cout << "fail open zed camera" << endl;
		return -1;
	}

	//获取相机信息
	auto camera_info = zed.getCameraInformation();
	int new_width = camera_info.camera_configuration.resolution.width;
	int new_height = camera_info.camera_configuration.resolution.height;

	sl::Mat zed_img;

	while (frontimg_flag)
	{
		if (zed.grab() == sl::ERROR_CODE::SUCCESS) {
			//获取图像
			zed.retrieveImage(zed_img, sl::VIEW::RIGHT);
			//WaitForSingleObject(s_mutex, INFINITE);
			front_img = cv::Mat((int)zed_img.getHeight(), (int)zed_img.getWidth(), CV_8UC4, zed_img.getPtr<sl::uchar1>(sl::MEM::CPU));
			//cout << "zed_channel:" << front_img.channels() << endl;

			//ReleaseMutex(s_mutex);
			cv::waitKey(10);
		}
	}
	zed.close();

	return 0;
}
unsigned int WINAPI pthGetBackImg(void*) {
	int ret = MV_OK;
	void* handle = NULL;

	MV_CC_DEVICE_INFO_LIST hk_devices;
	memset(&hk_devices, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
	ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &hk_devices);
	if (ret != MV_OK) {
		cout << "enum devices faild!" << endl;
		return -1;
	}
	if (hk_devices.nDeviceNum > 0) {
		MV_CC_DEVICE_INFO* hk_camera = hk_devices.pDeviceInfo[0];
		
	}
	else {
		cout << "no device found" << endl;
		return -1;
	}

	ret = MV_CC_CreateHandle(&handle, hk_devices.pDeviceInfo[0]);
	if (ret != MV_OK) {
		return -1;
	}
	ret = MV_CC_OpenDevice(handle);
	if (ret != MV_OK) {
		return -1;
	}

	ret = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
	if (ret != MV_OK) {
		return -1;
	}

	MVCC_INTVALUE hk_param;
	memset(&hk_param, 0, sizeof(MVCC_INTVALUE));
	ret = MV_CC_GetIntValue(handle, "PayloadSize", &hk_param);
	if (ret != MV_OK) {
		return -1;
	}
	unsigned int payload_size = hk_param.nCurValue;

	// load config
	ret = MV_CC_FeatureLoad(handle, "FeatureFile.ini");
	if (ret != MV_OK) {
		cout << "loading config file faild" << endl;
		return -1;
	}

	// start grabbing images
	ret = MV_CC_StartGrabbing(handle);
	if (ret != MV_OK) {
		cout << "grab image failed!" << endl;
		return -1;
	}
	MV_FRAME_OUT_INFO_EX hk_imginfo;
	memset(&hk_imginfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
	unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * (payload_size));
	if (data == NULL) {
		return -1;
	}

	while (backimg_flag) {
		ret = MV_CC_GetOneFrameTimeout(handle, data, payload_size, &hk_imginfo, 1000);
		if (ret != MV_OK) {
			free(data);
			data = NULL;
			return -1;
		}
		//WaitForSingleObject(s_mutex, INFINITE);
		if (hk2cv(&hk_imginfo, data, back_img) == false) {
			ReleaseMutex(s_mutex);
			continue;
		}
		//cout << "hk_channel:" << back_img.channels() << endl;

		//ReleaseMutex(s_mutex);
		
		cv::waitKey(10);
	}
	// stop grap image
	ret = MV_CC_StopGrabbing(handle);
	if (ret != MV_OK) {
		return -1;

	}
	// close device
	ret = MV_CC_CloseDevice(handle);
	if (ret != MV_OK) {
		return -1;

	}
	ret = MV_CC_DestroyHandle(handle);
	if (ret != MV_OK) {
		return -1;

	}
	return 0;
}



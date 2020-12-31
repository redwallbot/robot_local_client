#pragma once

#include "common.h"

class Point {
public:
	double x, y;

	void init() {
		x = 0.0;
		y = 0.0;
	}

};

class MyJoyStick
{
public:
	//手柄ID
	UINT joy_id;
	//手柄信息
	JOYCAPS joy_caps;
	//手柄状态
	JOYINFOEX* joy_info = new JOYINFOEX();
	//连接状态码
	int ret_code;
	//监听标识符
	bool listen_flag;
	//之前方向按钮
	int pre_dire_btn;
	//当前方向按钮
	int cur_dire_btn;
	//左摇杆
	Point xy_point;
	//右摇杆
	Point uv_point;
	//之前的按钮号
	int pre_btn;
	//当前按钮号
	int cur_btn;


	MyJoyStick(UINT joy_id) {
		this->joy_id = joy_id;
		ret_code = 0;
		listen_flag = false;
		pre_dire_btn = 65535;
		cur_dire_btn = 65535;
		xy_point.init();
		uv_point.init();
		pre_btn = 0;
		cur_btn = 0;

		joy_info->dwSize = sizeof(JOYINFOEX);
		joy_info->dwFlags = (int)JOY_RETURNBUTTONS;
	}

	//获取手柄信息
	bool getJsCaps() {
		ret_code = joyGetDevCaps(joy_id, &joy_caps, sizeof JOYCAPS);
		if (ret_code) {
			// cout << "error code:  " << ret_code << endl;
			return false;
		}
		
		cout << "产品标识符：" << joy_caps.wPid << endl;
		cout << "产品名称：" << joy_caps.szPname << endl;
		cout << "最小x坐标：" << joy_caps.wXmin << endl;
		cout << "最大x坐标：" << joy_caps.wXmax << endl;
		cout << "最小y坐标：" << joy_caps.wYmin << endl;
		cout << "最大y坐标：" << joy_caps.wYmax << endl;
		cout << "最小z坐标：" << joy_caps.wZmin << endl;
		cout << "最大z坐标：" << joy_caps.wZmax << endl;
		cout << "按钮数量：" << joy_caps.wNumButtons << endl;
		cout << "当被joySetCapture函数捕获，支持的最小轮询频率：" << joy_caps.wPeriodMin << endl;
		cout << "当被joySetCapture函数捕获，支持的最大轮询频率：" << joy_caps.wPeriodMax << endl;
		cout << "最小舵值（第四轴）：" << joy_caps.wRmin << endl;
		cout << "最大舵值（第四轴）：" << joy_caps.wRmax << endl;
		cout << "最小u坐标（第五轴）：" << joy_caps.wUmin << endl;
		cout << "最大u坐标（第五轴）：" << joy_caps.wUmax << endl;
		cout << "最小v坐标（第六轴）：" << joy_caps.wVmin << endl;
		cout << "最大v坐标（第六轴）：" << joy_caps.wVmax << endl;
		cout << "手柄功能：" << int(joy_caps.wCaps) << endl;
		cout << "手柄支持最大轴数：" << joy_caps.wMaxAxes << endl;
		cout << "当前使用轴数：" << joy_caps.wNumAxes << endl;
		cout << "支持最大按键数：" << joy_caps.wMaxButtons << endl;
		cout << "驱动：" << joy_caps.szOEMVxD << endl;
		cout << endl;
		return true;
	}

	//更新手柄状态
	bool updateStatus() {

		ret_code = joyGetPosEx(joy_id, joy_info);
		if (ret_code) {
			// cout << "error code:  " << ret_code << endl;
			return false;
		}
		
		cur_dire_btn = joy_info->dwPOV;

		xy_point.x = joy_info->dwXpos - 32767.0;
		xy_point.y = 32511.0 - joy_info->dwYpos;

		uv_point.x = joy_info->dwZpos - 32767.0;
		uv_point.y = 32511.0 - joy_info->dwRpos;


		cur_btn = joy_info->dwButtons;
		if (cur_btn == 512){
			listen_flag = true;
		}
		else if (cur_btn == 256){
			listen_flag = false;
		}
		return true;
	}

	//监听手柄
	virtual void listenJs() {
		if (!listen_flag) return;
		
		if (cur_dire_btn != pre_dire_btn){
			switch (cur_dire_btn){
			case 0:
				cout << "方向键上" << endl;
				break;
			case 18000:
				cout << "方向键下" << endl;
				break;
			case 27000:
				cout << "方向键左" << endl;
				break;
			case 9000:
				cout << "方向键右" << endl;
				break;
			default:
				break;
			}
			pre_dire_btn = cur_dire_btn;
		}

		// cout << "xy坐标：（" << xy_point.x << "," << xy_point.y << "）" << endl;
		// cout << "uv坐标：（" << uv_point.x << "," << uv_point.y << "）" << endl;

		if (cur_btn != pre_btn){
			switch (cur_btn) {
			case 2:
				cout << "A" << endl;
				break;
			case 4:
				cout << "B" << endl;
				break;
			case 1:
				cout << "X" << endl;
				break;
			case 8:
				cout << "Y" << endl;
				break;
			case 16:
				cout << "LB" << endl;
				break;
			case 32:
				cout << "LR" << endl;
				break;
			case 64:
				cout << "LT" << endl;
				break;
			case 128:
				cout << "RT" << endl;
				break;
			case 256:
				cout << "Back" << endl;
				break;
			case 512:
				cout << "Start" << endl;
				break;
			default:
				break;
			}
			pre_btn = cur_btn;
		}

		cout << endl;
	}

	virtual ~MyJoyStick() {
		if (joy_info != NULL){
			delete joy_info;
			joy_info = NULL;
		}
	}
};

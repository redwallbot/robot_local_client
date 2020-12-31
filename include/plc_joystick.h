#pragma once
#include "myjoystick.hpp"

class PLCJoystick : public MyJoyStick {

public:

	PLCJoystick(UINT joy_id);

	bool getDetectLane();

	double* getSpeed();

	virtual void listenJs();

	virtual ~PLCJoystick();


private:
	// 开启黑白循迹
	bool detect_lane;
	double xy_maxspeed;
	// 定义x、y方向速度以及z轴旋转角速度（采用机器人右手坐标系）
	double* speed = new double[3];

	
};


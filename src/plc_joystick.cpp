#include "plc_joystick.h"

#define XY_MAX_SPEED 1.0123
#define XY_MEDIUM_SPEED 0.5
#define W_MAX_SPEED 0.7

#define DRIFTING_VALUE 0.1

PLCJoystick::PLCJoystick(UINT joy_id) : MyJoyStick(joy_id) {
	detect_lane = false;
	xy_maxspeed = 0.0;
	speed = NULL;
}

bool PLCJoystick::getDetectLane() {
	return detect_lane;
}

double* PLCJoystick::getSpeed() {
	return speed;
}

void PLCJoystick::listenJs(){
	// cout << listen_flag << endl;
	if (!listen_flag) {
		detect_lane = false;
		return;
	}
	
	switch (cur_dire_btn){
	case 0:
		//cout << "方向键上" << endl;
		speed = new double[] {XY_MAX_SPEED, 0.0, 0.0};

		return;
	case 18000:
		//cout << "方向键下" << endl;
		speed = new double[] {-XY_MAX_SPEED, 0.0, 0.0};

		return;
	case 27000:
		//cout << "方向键左" << endl;
		speed = new double[] {0.0, XY_MAX_SPEED, 0.0};

		return;
	case 9000:
		//cout << "方向键右" << endl;
		speed = new double[] {0.0, -XY_MAX_SPEED, 0.0};

		return;
	default:
		break;
	}

	// cout << "xy坐标：（" << xy_point.x << "," << xy_point.y << "）" << endl;
	// cout << "uv坐标：（" << uv_point.x << "," << uv_point.y << "）" << endl;

	double v_cos = 0.0, v_sin = 0.0;
	if (xy_point.x != 0 || xy_point.y != 0) {
		v_cos = xy_point.x / (sqrt(xy_point.x * xy_point.x + xy_point.y * xy_point.y));
		v_sin = xy_point.y / (sqrt(xy_point.x * xy_point.x + xy_point.y * xy_point.y));
	}

	if (xy_point.x * xy_point.y == 0) {
		xy_maxspeed = XY_MAX_SPEED;
	}
	else {
		double k = fabs(xy_point.y / xy_point.x);
		xy_maxspeed = XY_MAX_SPEED / (1 + k) * sqrt(1 + k * k);
	}

	double v_level = uv_point.y / 32511.0;
	double w_level = uv_point.x / 32767.0;

	if (v_level > DRIFTING_VALUE && (v_cos != 0.0 || v_sin != 0.0)) {
		if (v_level < 0.4) {
			v_level *= 0.75;
		}
		speed = new double[] {xy_maxspeed * v_level * v_sin, -xy_maxspeed * v_level * v_cos, 0.0};
	}
	else if (v_level < -DRIFTING_VALUE) {
		speed = new double[] {XY_MEDIUM_SPEED* v_level, 0.0, 0.0};

	}
	else if (fabs(w_level) > DRIFTING_VALUE && fabs(v_level) < DRIFTING_VALUE) {
		speed = new double[] {0.0, 0.0, - w_level * W_MAX_SPEED};

	}
	else {

		speed = new double[] {0.0, 0.0, 0.0};
	}

	switch (cur_btn) {
	case 2:
		// cout << "A" << endl;
		speed = new double[] {0.2, 0.0, 0.0};
		break;
	case 4:
		// cout << "B" << endl;
		speed = new double[] {0.5, 0.0,0.0};

		break;
	case 1:
		// cout << "X" << endl;
		detect_lane = true;

		break;
	case 8:
		// cout << "Y" << endl;
		detect_lane = false;

		break;
	/* case 16:
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
		break; */
	default:
		break;
	}
}

PLCJoystick::~PLCJoystick() {
	if (joy_info != NULL)
	{
		delete joy_info;
		joy_info = NULL;
	}
	if (speed != NULL) {
		delete speed;
		speed = NULL;
	}
}
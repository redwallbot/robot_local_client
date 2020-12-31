#pragma once

#include "common.h"


extern "C"{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

typedef unsigned char uchar;

class MyRTMP {
private:
	//像素格式转换上下文
	SwsContext* m_sc;
	//输出数据结构
	AVFrame* m_frame;
	//编码器上下文
	AVCodecContext* m_acc;
	//rtmp flv 封装器
	AVFormatContext* m_afc;

	AVPacket m_pack;
	//定义编码器
	AVCodec* m_codec;
	//定义视频流
	AVStream* m_stream;

	// 帧数
	int m_fps;

	// 图像宽，高，通道数
	int m_width;
	int m_height;
	int m_channel;

	//状态标识符
	int m_ret ;
	int m_vpts;

public:
	MyRTMP(const int width, const int height, const int channel, const int fps=24);

	// 初始化
	// push_add:推流地址
	bool initRTMP(const char* push_add);

	// 开始推流
	// data:图像数据
	bool startRTMP(const uchar* data, int ele_size);

	// 打印错误信息
	void printError(int error_code);

	~MyRTMP();
};
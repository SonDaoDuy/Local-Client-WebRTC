#include "stdafx.h"
#include "customvideocapturer.h"
#include <Windows.h>
#include <iostream>
#include <pthread.h>
#include <time.h>
//#include <sys/time.h>


#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc\api\video\i420_buffer.h"

#include <memory>

using std::endl;

namespace videocapture {

#define STREAM "C:/Users/duyson/Desktop/batmat.mp4"

	pthread_t g_pthread;
	unsigned short port = 1234;
	UDPSocket* main_sock_ = new UDPSocket(port);

	CustomVideoCapturer::CustomVideoCapturer(int deviceId)
	{
		//tao socket

		
		
		m_stopped = false;
		//m_VCapture.open(STREAM);
		
	}

	CustomVideoCapturer::~CustomVideoCapturer()
	{
	}

	cricket::CaptureState CustomVideoCapturer::Start(const cricket::VideoFormat& capture_format)
	{
		std::cout << "Start" << endl;
		if (capture_state() == cricket::CS_RUNNING) {
			std::cout << "Start called when it's already started." << endl;
			return capture_state();
		}

		//kiem tra socket co available ko
		/*while (!m_VCapture.isOpened()) {
			std::cout << "Capturer is not open -> will try to reopen" << endl;
			m_VCapture.open(STREAM);
		}*/
		//get a reference to the current thread so we can send the frames to webrtc
		//on the same thread on which the capture was started
		m_startThread = rtc::Thread::Current();

		//start frame grabbing thread
		pthread_create(&g_pthread, NULL, grabCapture, (void*)this);

		SetCaptureFormat(&capture_format);
		return cricket::CS_RUNNING;
	}

	void CustomVideoCapturer::Stop()
	{
		std::cout << "Stop" << endl;
		if (capture_state() == cricket::CS_STOPPED) {
			std::cout << "Stop called when it's already stopped." << endl;
			return;
		}
		//kill the socket
		//closesocket(sock_);
		
		m_startThread = nullptr;
		stop_mtx.lock();
		m_stopped = true;
		/*::closesocket(1234);
		sock_ = nullptr;*/
		stop_mtx.unlock();
		SetCaptureFormat(NULL);
		SetCaptureState(cricket::CS_STOPPED);
		Sleep(1000);
	}

	/*static */void* CustomVideoCapturer::grabCapture(void* arg)
	{
		//lay data tu socket va truyen vao current frame
		char buffer[BUF_LEN]; // Buffer for echo string
		int recvMsgSize; // Size of received message
		string sourceAddress; // Address of datagram source
		unsigned short sourcePort; // Port of datagram source

		CustomVideoCapturer *vc = (CustomVideoCapturer*)arg;
		cv::Mat frame;

		if (nullptr == vc) {
			std::cout << "VideoCapturer pointer is null" << std::endl;
			return 0;
		}
		bool stopped = false;
		uint64_t prevTimestamp = 0;
		uint64_t elapsed_time = 33333333;
		while (!stopped) {
			//nhan anh tu client
			//vc->img_mtx.lock();
			do {
				recvMsgSize = main_sock_->recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
			} while (recvMsgSize > sizeof(int) && !stopped);
			vc->img_mtx.lock();
			int total_pack = ((int *)buffer)[0];			
			cout << "expecting length of packs:" << total_pack << endl;
			char * longbuf = new char[PACK_SIZE * total_pack];
			for (int i = 0; i < total_pack; i++) {
				recvMsgSize = main_sock_->recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
				if (recvMsgSize != PACK_SIZE) {
					cerr << "Received unexpected size pack:" << recvMsgSize << endl;
					continue;
				}
				memcpy(&longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
			}

			cout << "Received packet from " << sourceAddress << ":" << sourcePort << endl;
			
			cv::Mat rawData = cv::Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
			cv::Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);
			if (frame.size().width == 0) {
				cerr << "decode failure!" << endl;
			}
			free(longbuf);
			//vc->m_VCapture.read(frame);
			// Dua anh vao media track
			cv::Mat bgra(frame.rows, frame.cols, CV_8UC4);
			//opencv reads the stream in BGR format by default
			cv::cvtColor(frame, bgra, CV_RGB2RGBA);
			//cv::imshow("recv", frame);
			rtc::scoped_refptr<webrtc::I420Buffer> vframe;
			vframe = webrtc::I420Buffer::Create(bgra.cols, bgra.rows, bgra.cols, (bgra.cols + 1) / 2, (bgra.cols + 1) / 2);
			//webrtc::I420Buffer::SetBlack(vframe);
			if (0 != webrtc::ConvertToI420(webrtc::VideoType::kARGB, bgra.ptr(), 0, 0, bgra.cols, bgra.rows, 0, webrtc::kVideoRotation_0, vframe)) {
				std::cout << "Failed to convert frame to i420" << std::endl;
			}

			webrtc::VideoFrame webrtc_frame(vframe, webrtc::kVideoRotation_0, prevTimestamp);
			prevTimestamp += elapsed_time;

			//forward the frame to the video capture start thread
			vc->setOnFrame(webrtc_frame, bgra.cols, bgra.rows);
			vc->img_mtx.unlock();
			Sleep(40);

			//change state
			vc->stop_mtx.lock();
			stopped = vc->m_stopped;
			vc->stop_mtx.unlock();
		}
		return 0;
	}

	void CustomVideoCapturer::setOnFrame(webrtc::VideoFrame webrtc_frame, int width, int height) {
		OnFrame(webrtc_frame, width, height);
	}

	/*void CustomVideoCapturer::SignalFrameCapturedOnStartThread(const cricket::CapturedFrame* frame)
	{
		SignalFrameCaptured(this, frame);
	}*/

	bool CustomVideoCapturer::IsRunning()
	{
		return capture_state() == cricket::CS_RUNNING;
	}

	bool CustomVideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
	{
		if (!fourccs)
			return false;
		fourccs->push_back(cricket::FOURCC_I420);
		return true;
	}

	bool CustomVideoCapturer::GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format)
	{
		if (!best_format)
			return false;

		// Use the desired format as the best format.
		best_format->width = desired.width;
		best_format->height = desired.height;
		best_format->fourcc = cricket::FOURCC_I420;
		best_format->interval = desired.interval;
		return true;
	}

	bool CustomVideoCapturer::IsScreencast() const
	{
		return false;
	}

} // namespace videocapture
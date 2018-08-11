#pragma once

#include "opencv2/videoio.hpp"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc\rtc_base\thread.h"
#include "webrtc\media\base\device.h"
#include "webrtc\media\base\videocapturerfactory.h"
#include "PracticalSocket.h"
#include <mutex>

namespace videocapture {

	class CustomVideoCapturer :
		public cricket::VideoCapturer
	{
	public:
		CustomVideoCapturer(int deviceId);
		virtual ~CustomVideoCapturer();

		// cricket::VideoCapturer implementation.
		virtual cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override;
		virtual void Stop() override;
		virtual bool IsRunning() override;
		virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;
		virtual bool GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format) override;
		virtual bool IsScreencast() const override;
		void setOnFrame(webrtc::VideoFrame webrtc_frame, int width, int height);

	private:
		//DISALLOW_COPY_AND_ASSIGN(CustomVideoCapturer);

		static void* grabCapture(void* arg);

		//to call the SignalFrameCaptured call on the main thread
		//void SignalFrameCapturedOnStartThread(const cricket::CapturedFrame* frame);

		cv::VideoCapture m_VCapture; //opencv capture object
		rtc::Thread*  m_startThread; //video capture thread
		bool m_stopped;
		std::mutex stop_mtx;
		std::mutex img_mtx;
		UDPSocket* sock_;
	};


	class VideoCapturerFactoryCustom : public cricket::VideoDeviceCapturerFactory
	{
	public:
		VideoCapturerFactoryCustom() {}
		virtual ~VideoCapturerFactoryCustom() {}
		//std::unique_ptr<cricket::VideoCapturer> m_capturer;

		virtual std::unique_ptr<cricket::VideoCapturer> Create(const cricket::Device& device) {

			// XXX: WebRTC uses device name to instantiate the capture, which is always 0.
			//CustomVideoCapturer new_device(atoi(device.id.c_str()));
			std::unique_ptr<cricket::VideoCapturer> m_capturer(new CustomVideoCapturer(atoi(device.id.c_str())));
			return m_capturer;
		}
	};

} // namespace videocapture#pragma once

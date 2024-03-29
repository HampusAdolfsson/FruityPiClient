#include <cstdio>
#include <iostream>
#include <bitset>
#include "Logger.h"
#include "AudioVisualizer.h"

#pragma comment(lib, "winmm.lib")

#define BUFFERS_PER_S 120

#define RETURN_ON_ERROR(mres) do {\
						if (mres) {\
							LOGSEVERE("Audiovisualizer got error: %d, line %d", mres, __LINE__);\
							return false;\
						}\
						} while(0)

static constexpr Color BaseColor = { 0xbd, 0x52, 0xf2 };

AudioVisualizer::AudioVisualizer(const DWORD &devId, const WAVEFORMATEX &format, const std::string &addr, const int &port)
	: deviceId(devId),
	pwfx(format),
	colorClient(addr, port),
	isRunning(false) {}

bool AudioVisualizer::initialize()
{
	bool opened = openDevice();
	if (!opened) return false;

	int buffer_len = pwfx.nBlockAlign * (pwfx.nSamplesPerSec / BUFFERS_PER_S);
	buffer.reserve(buffer_len * NUM_BUFFERS);
	MMRESULT mres;
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		waveHeaders[i].dwBufferLength = buffer_len;
		waveHeaders[i].lpData = buffer.data() + buffer_len * i;
		mres = waveInPrepareHeader(waveInHandle, waveHeaders + i, sizeof(waveHeaders[i]));
		RETURN_ON_ERROR(mres);
	}
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		mres = waveInAddBuffer(waveInHandle, waveHeaders + i, sizeof(waveHeaders[i]));
		RETURN_ON_ERROR(mres);
	}
	return true;
}

bool AudioVisualizer::start()
{
	MMRESULT mres = waveInStart(waveInHandle);
	RETURN_ON_ERROR(mres);
	isRunning = true;
	return true;
}

bool AudioVisualizer::stop()
{
	MMRESULT mres = waveInStop(waveInHandle);
	RETURN_ON_ERROR(mres);

	isRunning = false;
	return true;
}

bool AudioVisualizer::openDevice()
{

	DWORD threadId;
	handlerThread = std::thread(&AudioVisualizer::handleWaveMessages, this);
	threadId = GetThreadId(handlerThread.native_handle());

	MMRESULT mres = waveInOpen(&waveInHandle, deviceId, &pwfx, threadId, 0, CALLBACK_THREAD);
	RETURN_ON_ERROR(mres);
	return true;
}

AudioVisualizer::~AudioVisualizer()
{

	if (isRunning) waveInStop(waveInHandle);
	MMRESULT mres;
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		mres = waveInUnprepareHeader(waveInHandle, waveHeaders + i, sizeof(waveHeaders[i]));
		if (mres) break;
	}
	MMRESULT mres2 = waveInClose(waveInHandle);
	if (!mres && !mres2 && handlerThread.joinable())
	{
		handlerThread.join();
	}
	else
	{
		handlerThread.detach();
	}
}

void AudioVisualizer::handleWaveMessages()
{
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		switch (msg.message)
		{
		case MM_WIM_DATA: {
			if (pwfx.wBitsPerSample != 16) printf("%s %d : WARNING! This code only supports 16-bit samples.\nPlease change the code.\n", __FILE__, __LINE__); // For performance reasons
			WAVEHDR *hdr = (WAVEHDR*)msg.lParam;
			if (hdr->dwBytesRecorded > 0)
			{
				size_t sampleSize = pwfx.wBitsPerSample / 8; // TODO: benchmark doing stuff here instead
				colorClient.sendColor(colorStrategy.getColor(hdr->lpData, hdr->dwBytesRecorded, sampleSize));
			}
			waveInAddBuffer(waveInHandle, waveHeaders, sizeof(WAVEHDR));
			continue;
		}
		case MM_WIM_OPEN:
			continue;
		case MM_WIM_CLOSE:
			return;
		}
	}
}

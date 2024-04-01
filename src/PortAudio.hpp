#pragma once

#include <iostream>
#include <stdexcept>
#include <portaudio.h>

class PortAudio
{
	struct Error : std::runtime_error
	{
		Error(const std::string &s) : std::runtime_error("portaudio: " + s) {}
	};

public:
	class Stream
	{
		PaStream *stream;
		int sample_size;

		Stream(int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback, void *userData)
		{
			PaError err;
			if ((err = Pa_OpenDefaultStream(&stream, numInputChannels, numOutputChannels, sampleFormat, sampleRate, framesPerBuffer, streamCallback, userData)))
				throw Error(Pa_GetErrorText(err));
			if ((err = Pa_StartStream(stream)))
				throw Error(Pa_GetErrorText(err));
			sample_size = framesPerBuffer;
		}

		friend class PortAudio;

	public:
		~Stream()
		{
			PaError err;
			if ((err = Pa_StopStream(stream)))
				std::cerr << Pa_GetErrorText(err) << '\n';
			if ((err = Pa_CloseStream(stream)))
				std::cerr << Pa_GetErrorText(err) << '\n';
		}

		Stream(const Stream &) = delete;
		Stream &operator=(const Stream &) = delete;
		Stream(Stream &&) = delete;
		Stream &operator=(Stream &&) = delete;

		void reopen(int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback = NULL, void *userData = NULL)
		{
			PaError err;
			if ((err = Pa_StopStream(stream)))
				throw Error(Pa_GetErrorText(err));
			if ((err = Pa_CloseStream(stream)))
				throw Error(Pa_GetErrorText(err));

			if ((err = Pa_OpenDefaultStream(&stream, numInputChannels, numOutputChannels, sampleFormat, sampleRate, framesPerBuffer, streamCallback, userData)))
				throw Error(Pa_GetErrorText(err));
			if ((err = Pa_StartStream(stream)))
				throw Error(Pa_GetErrorText(err));

			sample_size = framesPerBuffer;
		}

		void write(const float *const buffer, const size_t n_frames)
		{
			PaError err;
			if ((err = Pa_WriteStream(stream, buffer, n_frames)))
				throw Error(Pa_GetErrorText(err));
		}
	};

	PortAudio()
	{
		PaError err;
		if ((err = Pa_Initialize()))
			throw Error(Pa_GetErrorText(err));
	}

	~PortAudio()
	{
		PaError err;
		if ((err = Pa_Terminate()))
			std::cerr << Pa_GetErrorText(err) << '\n';
	}

	PortAudio(const PortAudio &) = delete;
	PortAudio &operator=(const PortAudio &) = delete;
	PortAudio(PortAudio &&) = delete;
	PortAudio &operator=(PortAudio &&) = delete;

	Stream stream(int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback = NULL, void *userData = NULL)
	{
		return Stream(numInputChannels, numOutputChannels, sampleFormat, sampleRate, framesPerBuffer, streamCallback, userData);
	}
};
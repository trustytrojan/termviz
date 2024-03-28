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

	class Stream
	{
		PaStream *stream;

	public:
		Stream(int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback, void *userData)
		{
			PaError err;
			if ((err = Pa_OpenDefaultStream(&stream, numInputChannels, numOutputChannels, sampleFormat, sampleRate, framesPerBuffer, streamCallback, userData)))
				throw Error(Pa_GetErrorText(err));
			if ((err = Pa_StartStream(stream)))
				throw Error(Pa_GetErrorText(err));
		}

		~Stream()
		{
			PaError err;
			if ((err = Pa_StopStream(stream)))
				std::cerr << Pa_GetErrorText(err) << '\n';
			if ((err = Pa_CloseStream(stream)))
				std::cerr << Pa_GetErrorText(err) << '\n';
		}

		PaError write(const void *const buffer, const size_t n_frames)
		{
			return Pa_WriteStream(stream, buffer, n_frames);
		}
	};

public:
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

	Stream stream(int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback = NULL, void *userData = NULL)
	{
		return Stream(numInputChannels, numOutputChannels, sampleFormat, sampleRate, framesPerBuffer, streamCallback, userData);
	}
};
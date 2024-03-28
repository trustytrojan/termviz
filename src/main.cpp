#include <kiss_fftr.h>
#include <sndfile.hh>

#include "Args.hpp"
#include "ColorUtils.hpp"
#include "PortAudio.hpp"
#include "SmoothedAmplitudes.hpp"
#include "TerminalSize.hpp"

void _main(const Args &args)
{
	// open audio file
	SndfileHandle sf(args.audio_file);

	// kissfft initialization
	const auto cfg = kiss_fftr_alloc(args.fft_size, 0, NULL, NULL);

	// initialize PortAudio, create stream
	PortAudio pa;
	auto pa_stream = pa.stream(0, sf.channels(), paFloat32, sf.samplerate(), args.fft_size, NULL, NULL);

	// define some constants
	const int freqdata_len = (args.fft_size / 2) + 1;
	const auto fftsize_inv = args.spectrum.multiplier / args.fft_size;
	const auto logmax = log(freqdata_len);
	const auto sqrtmax = sqrt(freqdata_len);

	// arrays to store fft and audio data
	float timedata[args.fft_size];
	kiss_fft_cpx freqdata[freqdata_len];
	float buffer[args.fft_size * sf.channels()];

	// get terminal size, initialize amplitudes array
	TerminalSize tsize;
	std::vector<float> amplitudes(tsize.width);

	// start reading, playing, and processing audio
	while (1)
	{
		{ // if terminal size changes, resize amplitudes array if width changes
			const TerminalSize new_tsize;

			if (tsize.width != new_tsize.width)
			{
				amplitudes.resize(new_tsize.width);
				tsize.width = new_tsize.width;
			}

			if (tsize.height != new_tsize.height)
				tsize.height = new_tsize.height;
		}

		{ // read audio into buffer, write to output stream to play
			// sf_readf_float reads FRAMES, where each FRAME is a COLLECTION of samples, one FOR EACH CHANNEL.
			const auto frames_read = sf.readf(buffer, args.fft_size);

			// break on end of file
			if (!frames_read)
				break;

			// play the audio as it is read
			pa_stream.write(buffer, frames_read);

			// we can't process anything less than fft_size
			if (frames_read != args.fft_size)
				break;
		}

		// only consider the last channel (for now)
		for (int i = 0; i < args.fft_size; ++i)
			timedata[i] = buffer[i * sf.channels()];

		// perform fft: frequency range to amplitude values are stored in freqdata
		kiss_fftr(cfg, timedata, freqdata);

		// zero out array since we are creating sums
		for (auto &a : amplitudes)
			a = 0;

		// map frequency bins of freqdata to amplitudes
		for (auto i = 0; i < freqdata_len; ++i)
		{
			const auto [re, im] = freqdata[i];
			const float amplitude = sqrt((re * re) + (im * im));
			int index;
			switch (args.scale)
			{
			case Args::Scale::LINEAR:
				index = (float)i / freqdata_len * tsize.width;
				break;
			case Args::Scale::LOG:
				index = log(i ? i : 1) / logmax * tsize.width;
				break;
			case Args::Scale::SQRT:
				index = sqrt(i) / sqrtmax * tsize.width;
				break;
			default:
				throw std::logic_error("impossible!!!!!!!");
			}
			amplitudes[std::min(index, tsize.width - 1)] += amplitude;
		}

		// clear the terminal
		std::cout << "\ec";

		// disable color if requested (during runtime)
		// if (!args.color)
		// 	std::cout << "\e[39m";

		// apply spline only if scale isn't linear
		if (args.scale != Args::Scale::LINEAR)
			amplitudes = SmoothedAmplitudes(amplitudes);

		// print the spectrum
		for (int i = 0; i < tsize.width; ++i)
		{
			// calculate height based on amplitude
			int bar_height = fftsize_inv * amplitudes[i] * tsize.height;

			if (args.color)
			{
				const auto [h, s, v] = args.hsv;
				const auto [r, g, b] = ColorUtils::hsvToRgb(((float)i / tsize.width) + h, s, v);
				std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
			}

			// move cursor to (height, i)
			// remember that (0, 0) in a terminal is the top-left corner, so positive y moves the cursor down.
			std::cout << "\e[" << tsize.height << ';' << i << 'f';

			// draw the bar upwards `bar_height` high
			for (int j = 0; j < bar_height; ++j)
				// print character, move cursor up 1, move cursor left 1
				std::cout << ((j == bar_height - 1) ? args.spectrum.peak_char : args.spectrum.character) << "\e[1A\e[1D";
		}
	}

	// resource cleanup
	kiss_fftr_free(cfg);
}

#include <signal.h>

void sigint_handler(int)
{
	std::cout << "\ec";
	exit(0);
}

int main(const int argc, const char *const *const argv)
{
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
		perror("signal");
	try
	{
		_main(Args(argc, argv));
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

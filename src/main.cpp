#include <sndfile.hh>

#include "Args.hpp"
#include "FrequencySpectrum.hpp"
#include "ColorUtils.hpp"
#include "PortAudio.hpp"
#include "TerminalSize.hpp"

void _main(const Args &args)
{
	// initialize resources
	SndfileHandle sf(args.audio_file);
	FrequencySpectrum fs(args.fft_size, args.scale);
	PortAudio pa;
	auto pa_stream = pa.stream(0, sf.channels(), paFloat32, sf.samplerate(), args.fft_size, NULL, NULL);

	// used to normalize the bar height: more samples mean higher amplitudes in the spectrum array.
	// numerator used to be `1.`, then saw that this is an easy way to add a multiplier argument.
	const auto fftsize_inv = args.spectrum.multiplier / args.fft_size;

	// arrays to store fft input and audio data
	float timedata[args.fft_size];
	float buffer[args.fft_size * sf.channels()];

	// get terminal size, initialize spectrum array
	TerminalSize tsize;
	std::vector<float> spectrum(tsize.width);

	// start reading, playing, and processing audio
	while (1)
	{
		{ // if terminal size changes, resize amplitudes array if width changes
			const TerminalSize new_tsize;

			if (tsize.width != new_tsize.width)
			{
				spectrum.resize(new_tsize.width);
				tsize.width = new_tsize.width;
			}

			if (tsize.height != new_tsize.height)
				tsize.height = new_tsize.height;
		}

		{ // read audio into buffer and play it
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

		// render the spectrum into the array
		fs.render(timedata, spectrum);

		// clear the terminal
		// std::cout << "\ec";

		// print the spectrum
		for (int i = 0; i < tsize.width; ++i)
		{
			// calculate height based on amplitude
			// `spectrum[i]` is usually between [0, 1] scaled by `args.fft_size`,
			// so we need to multiply by its inverse to normalize it.
			int bar_height = fftsize_inv * spectrum[i] * tsize.height;

			switch (args.color)
			{
			case Args::ColorType::WHEEL:
			{
				const auto [h, s, v] = args.hsv;
				const auto [r, g, b] = ColorUtils::hsvToRgb(((float)i / tsize.width) + h, s, v);
				std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
				break;
			}

			case Args::ColorType::SOLID:
			{
				const auto [r, g, b] = args.rgb;
				std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
				break;
			}

			case Args::ColorType::NONE:
				break;

			default:
				throw std::logic_error("?????");
			}

			// move cursor to (height, i)
			// remember that (0, 0) in a terminal is the top-left corner, so positive y moves the cursor down.
			std::cout << "\e[" << tsize.height << ';' << i << 'f';

			// draw the bar upwards `bar_height` high
			for (int j = 0; j < bar_height; ++j)
			{
				// print character, move cursor up 1, move cursor left 1
				char character;

				if (args.spectrum.peak_char && j == bar_height - 1)
					character = args.spectrum.peak_char;
				else
					character = args.spectrum.characters[j % args.spectrum.characters.length()];

				std::cout << character << "\e[1A\e[1D";
			}
		}
	}
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
		std::cerr << argv[0] << ": " << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

#pragma once

#include <sndfile.hh>
#include "FrequencySpectrum.hpp"
#include "ColorUtils.hpp"
#include "PortAudio.hpp"
#include "TerminalSize.hpp"

class termviz
{
public:
	enum class ColorType
	{
		NONE,
		WHEEL,
		SOLID
	};

private:
	using Scale = FrequencySpectrum::Scale;
	using InterpType = FrequencySpectrum::InterpType;

	// the most important value
	int sample_size = 2048;

	// audio file
	SndfileHandle sf;

	// clean spectrum generator
	FrequencySpectrum fs;

	// terminal width and height
	TerminalSize tsize;

	// intermediate arrays
	std::vector<float> timedata, audio_buffer, spectrum;

	// audio
	PortAudio pa;
	PortAudio::Stream pa_stream;

	// color
	ColorType color_type = ColorType::WHEEL;
	std::tuple<float, float, float> hsv{0.9, 0.7, 1};
	std::tuple<int, int, int> rgb{255, 0, 255};

	// characters
	char peak_char;
	std::string characters;

	// color wheel rotation
	float wheel_time = 0;
	float wheel_rate = 0;

	// spectrum - final multiplier
	float multiplier = 3;

	// output stream, default std::cout
	std::ostream &ostr;

public:
	termviz(const std::string &path, std::ostream &ostr = std::cout)
		: sf(path),
		  ostr(ostr),
		  fs(sample_size),
		  timedata(sample_size),
		  audio_buffer(sample_size * sf.channels()),
		  spectrum(tsize.width),
		  pa_stream(pa.stream(0, sf.channels(), paFloat32, sf.samplerate(), sample_size)) {}

	void start()
	{
		while (render_frame())
			;
		ostr << "\ec";
	}

	termviz &set_sample_size(const int sample_size)
	{
		this->sample_size = sample_size;
		fs.set_fft_size(sample_size);
		timedata.resize(sample_size);
		audio_buffer.resize(sample_size * sf.channels());
		pa_stream.reopen(0, 2, paFloat32, sf.samplerate(), sample_size);
		return *this;
	}

	termviz &set_characters(const std::string &characters)
	{
		this->characters = characters;
		return *this;
	}

	termviz &set_peak_char(const char peak_char)
	{
		this->peak_char = peak_char;
		return *this;
	}

	termviz &set_color_type(const ColorType color_type)
	{
		this->color_type = color_type;
		return *this;
	}

	termviz &set_wheel_rate(const float wheel_rate)
	{
		this->wheel_rate = wheel_rate;
		return *this;
	}

	termviz &set_rgb(const std::tuple<int, int, int> rgb)
	{
		this->rgb = rgb;
		return *this;
	}

	termviz &set_hsv(const std::tuple<float, float, float> hsv)
	{
		this->hsv = hsv;
		return *this;
	}

	/**
	 * Set interpolation type.
	 * @param interp new interpolation type to use
	 */
	termviz &set_interp_type(const InterpType interp_type)
	{
		fs.set_interp_type(interp_type);
		return *this;
	}

	/**
	 * Set the spectrum's frequency scale.
	 * @param scale new scale to use
	 */
	termviz &set_scale(const Scale scale)
	{
		fs.set_scale(scale);
		return *this;
	}

	/**
	 * Set the nth-root to use when using the `NTH_ROOT` scale.
	 * @param nth_root new nth_root to use
	 * @throws `std::invalid_argument` if `nth_root` is zero
	 */
	termviz &set_nth_root(const int nth_root)
	{
		fs.set_nth_root(nth_root);
		return *this;
	}

	termviz &set_multiplier(const float multiplier)
	{
		this->multiplier = multiplier;
		return *this;
	}

private:
	void check_tsize_update()
	{
		const TerminalSize new_tsize;

		if (tsize.width != new_tsize.width)
		{
			spectrum.resize(new_tsize.width);
			tsize.width = new_tsize.width;
		}

		if (tsize.height != new_tsize.height)
			tsize.height = new_tsize.height;
	}

	void copy_channel_to_timedata(const int channel_num)
	{
		if (channel_num <= 0)
			throw std::invalid_argument("channel_num <= 0");
		if (channel_num > sf.channels())
			throw std::invalid_argument("channel_num > sf.channels()");
		for (int i = 0; i < sample_size; ++i)
			timedata[i] = audio_buffer[i * sf.channels() + channel_num];
	}

	bool render_frame()
	{
		check_tsize_update();
		const auto frames_read = sf.readf(audio_buffer.data(), sample_size);
		if (!frames_read)
			return false;
		pa_stream.write(audio_buffer.data(), sample_size);
		if (frames_read != sample_size)
			return false;
		copy_channel_to_timedata(1);
		fs.render(timedata.data(), spectrum);
		ostr << "\ec";
		print_spectrum();
		return true;
	}

	void print_spectrum()
	{
		for (int i = 0; i < tsize.width; ++i)
		{
			// calculate height based on amplitude
			int bar_height = spectrum[i] * tsize.height;

			// apply coloring if necessary
			switch (color_type)
			{
			case ColorType::WHEEL:
			{
				const auto [h, s, v] = hsv;
				const auto [r, g, b] = ColorUtils::hsvToRgb(((float)i / tsize.width) + h + wheel_time, s, v);
				ostr << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
				break;
			}

			case ColorType::SOLID:
			{
				// need to print everytime because clearing the terminal also clears color modes
				const auto [r, g, b] = rgb;
				ostr << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
				break;
			}

			case ColorType::NONE:
				break;

			default:
				throw std::logic_error("?????");
			}

			// move cursor to (height, i)
			// remember that (0, 0) in a terminal is the top-left corner, so positive y moves the cursor down.
			ostr << "\e[" << tsize.height << ';' << i << 'f';

			// draw the bar upwards `bar_height` high
			for (int j = 0; j < bar_height; ++j)
			{
				// print character, move cursor up 1, move cursor left 1
				char character;

				if (peak_char && j == bar_height - 1)
					character = peak_char;
				else
					character = characters[j % characters.length()];

				ostr << character << "\e[1A\e[1D";
			}
		}

		wheel_time += wheel_rate;
	}
};
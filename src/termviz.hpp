#pragma once

#include "ColorUtils.hpp"
#include "FrequencySpectrum.hpp"
#include "PortAudio.hpp"
#include "TerminalSize.hpp"
#include <mutex>
#include <sndfile.hh>

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

	// in case multiple threads use this object!
	std::mutex mutex;

	// the most important value
	int sample_size = 3000;

	// audio file
	SndfileHandle sf;

	// clean spectrum generator
	FrequencySpectrum fs;

	// terminal width and height
	TerminalSize tsize;
	bool stereo_mirrored = true;

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

public:
	termviz(const std::string &audio_file) :
		sf(audio_file),
		fs(sample_size),
		timedata(sample_size),
		audio_buffer(sample_size *sf.channels()),
		spectrum(stereo_mirrored ? (tsize.width / 2) : tsize.width),
		pa_stream(pa.stream(0, sf.channels(), paFloat32, sf.samplerate(), sample_size)) {}

	void start()
	{
		while (render_frame());
		std::cout << "\ec";
	}

	termviz &set_sample_size(const int sample_size)
	{
		mutex.lock();
		this->sample_size = sample_size;
		fs.set_fft_size(sample_size);
		timedata.resize(sample_size);
		audio_buffer.resize(sample_size * sf.channels());
		pa_stream.reopen(0, 2, paFloat32, sf.samplerate(), sample_size);
		mutex.unlock();
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
			spectrum.resize(stereo_mirrored ? (new_tsize.width / 2) : new_tsize.width);
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
		mutex.lock();
		check_tsize_update();
		const auto frames_read = sf.readf(audio_buffer.data(), sample_size);
		if (!frames_read)
			return false;
		pa_stream.write(audio_buffer.data(), sample_size);
		if (frames_read != sample_size)
			return false;
		std::cout << "\ec";
		if (stereo_mirrored)
		{
			copy_channel_to_timedata(1);
			fs.render(timedata.data(), spectrum);
			print_mirrored_1st_half();

			copy_channel_to_timedata(2);
			fs.render(timedata.data(), spectrum);
			print_mirrored_2nd_half();
		}
		else
		{
			copy_channel_to_timedata(1);
			fs.render(timedata.data(), spectrum);
			print_spectrum_full();
		}
		mutex.unlock();
		return true;
	}

	void print_spectrum_full()
	{
		for (int i = 0; i < tsize.width; ++i)
		{
			apply_coloring(i, [this](const int i) { return (float)i / tsize.width; });
			move_to_column(i);
			print_bar(multiplier * spectrum[tsize.width - i] * tsize.height);
		}
		wheel_time += wheel_rate;
	}

	void print_mirrored_1st_half()
	{
		const auto half_width = tsize.width / 2;
		for (int i = half_width; i >= 0; --i)
		{
			apply_coloring(i, [half_width](const int i) { return ((float)half_width - i) / half_width; });
			move_to_column(i);
			print_bar(multiplier * spectrum[half_width - i] * tsize.height);
		}
	}

	void print_mirrored_2nd_half()
	{
		const auto half_width = tsize.width / 2;
		for (int i = half_width; i < tsize.width; ++i)
		{
			apply_coloring(i, [half_width](const int i) { return (float)i / half_width; });
			move_to_column(i);
			print_bar(multiplier * spectrum[i - half_width] * tsize.height);
		}
	}

	void apply_coloring(const int i, const std::function<float(int)> &ratio_calc)
	{
		switch (color_type)
		{
		case ColorType::WHEEL:
		{
			const auto [h, s, v] = hsv;
			const auto [r, g, b] = ColorUtils::hsvToRgb(ratio_calc(i) + h + wheel_time, s, v);
			std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
			break;
		}

		case ColorType::SOLID:
		{
			// need to print everytime because clearing the terminal also clears color modes
			const auto [r, g, b] = rgb;
			std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
			break;
		}

		case ColorType::NONE:
			break;

		default:
			throw std::logic_error("?????");
		}
	}

	void move_to_column(const int i)
	{
		// move cursor to (height, i)
		// remember that (0, 0) in a terminal is the top-left corner, so positive y moves the cursor down.
		std::cout << "\e[" << tsize.height << ';' << i << 'f';
	}

	void print_bar(const int height)
	{
		if (!height)
			return;

		// height = (int)height;
		int j = 0;

		// until height - 1 to account for peak_char
		for (; j < height - 1; ++j)
			// print character, move cursor up 1, move cursor left 1
			std::cout << characters[j % characters.length()] << "\e[1A\e[1D";

		// print peak_char if set, otherwise next character in characters
		std::cout << (peak_char ? peak_char : characters[j % characters.length()]);
	}
};
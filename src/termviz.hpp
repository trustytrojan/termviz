#pragma once

#include <mutex>
#include <sndfile.hh>
#include "ColorUtils.hpp"
#include "FrequencySpectrum.hpp"
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

	using Scale = FrequencySpectrum::Scale;
	using InterpType = FrequencySpectrum::InterpType;
	using AccumulationMethod = FrequencySpectrum::AccumulationMethod;
	using WindowFunction = FrequencySpectrum::WindowFunction;

private:
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
	bool stereo = (sf.channels() == 2);
	bool mirrored = false;

	// intermediate arrays
	std::vector<float>
		timedata = std::vector<float>(sample_size),
		audio_buffer = std::vector<float>(sample_size * sf.channels()),
		spectrum = std::vector<float>(stereo ? (tsize.width / 2) : tsize.width);

	// audio
	PortAudio pa;
	PortAudio::Stream pa_stream = pa.stream(0, sf.channels(), paFloat32, sf.samplerate(), sample_size);

	// color
	ColorType color_type = ColorType::WHEEL;
	std::tuple<int, int, int> solid_rgb{255, 0, 255};

	// characters
	char peak_char = 0;
	std::string characters = "#";

	// color wheel rotation
	struct
	{
		float time = 0, rate = 0;
		std::tuple<float, float, float> hsv{0.9, 0.7, 1};
	} wheel;

	// spectrum - final multiplier
	float multiplier = 3;

public:
	termviz(const std::string &audio_file) : sf(audio_file), fs(sample_size) {}

	/**
	 * Start rendering the spectrum to the terminal!
	 * @note Blocks until finished.
	 */
	void start()
	{
		while (render_frame())
			;
		std::cout << "\ec";
	}

	/**
	 * Set the sample chunk size to use in internal calculations.
	 * @note Smaller values increase responsiveness, but decrease accuracy. Larger values do the opposite.
	 * @note This method is thread safe.
	 * @param sample_size new sample size to use
	 * @return reference to self
	 */
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

	/**
	 * Set the character(s) to print (in order) as the bar is printed upwards.
	 * @param characters new characters to use
	 * @return reference to self
	 */
	termviz &set_characters(const std::string &characters)
	{
		this->characters = characters;
		return *this;
	}

	/**
	 * Set the character to print at the peak of a spectrum bar.
	 * @param peak_char new peak char to use
	 * @return reference to self
	 */
	termviz &set_peak_char(const char peak_char)
	{
		this->peak_char = peak_char;
		return *this;
	}

	/**
	 * Set the spectrum coloring type.
	 * @param color_type new coloring type to use
	 * @return reference to self
	 */
	termviz &set_color_type(const ColorType color_type)
	{
		this->color_type = color_type;
		return *this;
	}

	/**
	 * Set the rate at which the color wheel rotates during playback.
	 * A sane default is 0.005, though there is no restriction.
	 * A value of 0 disables rotation altogether.
	 * @param rate new wheel rate to use
	 * @return reference to self
	 */
	termviz &set_wheel_rate(const float rate)
	{
		wheel.rate = rate;
		return *this;
	}

	/**
	 * Set the color to use when coloring the spectrum with a solid color.
	 * You will only see the change if the color type is set to `SOLID`.
	 * @param rgb (red, green, blue) tuple
	 * @return reference to self
	 */
	termviz &set_solid_color(const std::tuple<int, int, int> rgb)
	{
		this->solid_rgb = rgb;
		return *this;
	}

	/**
	 * Set the hue offset, saturation, and value (brightness) of the color wheel.
	 * You will only see the change if the color type is set to `WHEEL`.
	 * @param hsv (hue, saturation, value) tuple
	 * @return reference to self
	 */
	termviz &set_wheel_hsv(const std::tuple<float, float, float> hsv)
	{
		wheel.hsv = hsv;
		return *this;
	}

	/**
	 * Set interpolation type.
	 * @param interp new interpolation type to use
	 * @returns reference to self
	 */
	termviz &set_interp_type(const InterpType interp_type)
	{
		fs.set_interp_type(interp_type);
		return *this;
	}

	/**
	 * Set the spectrum's frequency scale.
	 * @param scale new scale to use
	 * @returns reference to self
	 */
	termviz &set_scale(const Scale scale)
	{
		fs.set_scale(scale);
		return *this;
	}

	/**
	 * Set the nth-root to use when using the `NTH_ROOT` scale.
	 * @param nth_root new nth_root to use
	 * @returns reference to self
	 * @throws `std::invalid_argument` if `nth_root` is zero
	 */
	termviz &set_nth_root(const int nth_root)
	{
		fs.set_nth_root(nth_root);
		return *this;
	}

	/**
	 * Set frequency bin accumulation method.
	 * @note Choosing `SUM` results in more visible detail in the treble frequencies, at the cost of their amplitudes being visually exaggerated.
	 * @note Choosing `MAX` results in a more true-to-waveform frequency distribution, however treble frequencies aren't very visible.
	 * @param interp new accumulation method to use
	 * @returns reference to self
	 */
	termviz &set_accum_method(const AccumulationMethod method)
	{
		fs.set_accum_method(method);
		return *this;
	}

	/**
	 * Set window function.
	 * @param interp new window function to use
	 * @returns reference to self
	 */
	termviz &set_window_function(const WindowFunction wf)
	{
		fs.set_window_func(wf);
		return *this;
	}

	/**
	 * Set the multiplier to multiply the spectrum's height by.
	 * @param multiplier new multiplier to use
	 * @return reference to self
	 */
	termviz &set_multiplier(const float multiplier)
	{
		this->multiplier = multiplier;
		return *this;
	}

	/**
	 * Enable or disable a mirrored spectrum with stereo support.
	 * For the mirrored spectrum to actually be stereo, the audio must be stereo. Otherwise the same channel of audio is rendered twice.
	 */
	termviz &set_stereo(const bool b)
	{
		stereo = b;
		return *this;
	}

private:
	void check_tsize_update()
	{
		const TerminalSize new_tsize;

		if (tsize.width != new_tsize.width)
		{
			spectrum.resize(stereo ? (new_tsize.width / 2) : new_tsize.width);
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

		try
		{
			pa_stream.write(audio_buffer.data(), sample_size);
		}
		catch (const PortAudio::Error &e)
		{
			if (!strstr(e.what(), "Output underflowed"))
				throw;
		}

		if (frames_read != sample_size)
			return false;
		std::cout << "\ec";

		if (color_type == ColorType::SOLID)
		{
			// clearing the terminal also clears color modes
			const auto [r, g, b] = solid_rgb;
			std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
		}

		if (stereo)
			for (int i = 1; i <= 2; ++i)
			{
				copy_channel_to_timedata(i);
				fs.render(timedata.data(), spectrum);
				print_half(i);
			}
		else
		{
			copy_channel_to_timedata(1);
			fs.render(timedata.data(), spectrum);
			print_spectrum_full();
		}

		wheel.time += wheel.rate;

		mutex.unlock();
		return true;
	}

	void print_spectrum_full()
	{
		for (int i = 0; i < tsize.width; ++i)
		{
			if (color_type == ColorType::WHEEL)
				apply_wheel_coloring(i, [this](const int i)
									 { return (float)i / tsize.width; });
			move_to_column(i);
			print_bar(multiplier * spectrum[tsize.width - i] * tsize.height);
		}
	}

	void print_spectrum_full_backwards()
	{
		for (int i = tsize.width - 1; i >= 0; --i)
		{
			if (color_type == ColorType::WHEEL)
				apply_wheel_coloring(i, [this](const int i)
									 { return (float)(tsize.width - i) / tsize.width; });
			move_to_column(i);
			print_bar(multiplier * spectrum[tsize.width - i] * tsize.height);
		}
	}

	void print_half(int half)
	{
		const auto half_width = tsize.width / 2;

		if (half == 1)
			for (int i = half_width; i >= 0; --i)
			{
				if (color_type == ColorType::WHEEL)
					apply_wheel_coloring(i, [half_width](const int i)
										 { return (float)(half_width - i) / half_width; });
				move_to_column(i);
				print_bar(multiplier * spectrum[half_width - i] * tsize.height);
			}

		else if (half == 2)
			for (int i = half_width; i < tsize.width; ++i)
			{
				if (color_type == ColorType::WHEEL)
					apply_wheel_coloring(i, [half_width](const int i)
										 { return (float)i / half_width; });
				move_to_column(i);
				print_bar(multiplier * spectrum[i - half_width] * tsize.height);
			}
	}

	void apply_wheel_coloring(const int i, const std::function<float(int)> &ratio_calc)
	{
		if (color_type != ColorType::WHEEL)
			throw std::logic_error("termviz::apply_wheel_coloring: color_type != ColorType::WHEEL");
		const auto [h, s, v] = wheel.hsv;
		const auto [r, g, b] = ColorUtils::hsvToRgb(ratio_calc(i) + h + wheel.time, s, v);
		std::cout << "\e[38;2;" << r << ';' << g << ';' << b << 'm';
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
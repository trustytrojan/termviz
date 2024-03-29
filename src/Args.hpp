#pragma once

#include <argparse/argparse.hpp>
#include "FrequencySpectrum.hpp"

using argparse::ArgumentParser;

struct Args : private ArgumentParser
{
	enum class ColorType
	{
		WHEEL,
		SOLID,
		NONE
	};

	std::string audio_file;
	int fft_size;

	float multiplier;
	std::string characters;
	char peak_char;
	int nth_root = 0;

	// spectrum scale
	FrequencySpectrum::Scale scale;
	FrequencySpectrum::InterpolationType interp;

	ColorType color;
	std::tuple<float, float, float> hsv{0, 1, 1};
	std::tuple<int, int, int> rgb;

	Args(const int argc, const char *const *const argv)
		: ArgumentParser(argv[0])
	{
		add_argument("audio_file")
			.help("audio file to visualize and play");
		add_argument("-n", "--sample-size")
			.help("number of samples (or frames of samples) to process at a time\n- higher -> increases accuracy\n- lower -> increases responsiveness")
			.default_value(2048)
			.scan<'i', int>()
			.validate();
		add_argument("-c", "--spectrum-chars")
			.help("characters to render columns with\nif more than 1 character is given, --peak-char is recommended")
			.default_value("#");
		add_argument("--peak-char")
			.help("character to print at the highest point for each column")
			.default_value("");
		add_argument("-s", "--scale")
			.help("spectrum frequency scale")
			.choices("linear", "log", "nth-root")
			.default_value("log")
			.validate();
		add_argument("--nth-root")
			.help("set the root to use with '--scale nth-root'")
			.default_value(2.f)
			.scan<'f', float>()
			.validate();
		add_argument("-i", "--interpolation")
			.help("spectrum interpolation type")
			.choices("none", "linear", "cspline", "cspline_hermite")
			.default_value("cspline")
			.validate();
		add_argument("-m", "--multiplier")
			.help("multiply spectrum amplitude by this amount")
			.default_value(3.f)
			.scan<'f', float>()
			.validate();
		add_argument("--color")
			.help("enable a colorful spectrum!")
			.choices("wheel", "solid", "none")
			.default_value("wheel")
			.validate();
		add_argument("--hsv")
			.help("requires '--color wheel'\nchoose a hue offset for the color wheel, saturation, and brightness\nvalues must be between [0, 1]")
			.nargs(3)
			.validate();
		add_argument("--rgb")
			.help("required by '--color solid'\nrenders the spectrum with a solid color\nmust provide space-separated rgb integers")
			.nargs(3)
			.validate();

		try
		{
			parse_args(argc, argv);
			copy_arg_values();
		}
		catch (const std::exception &e)
		{
			// print error and help to stderr
			std::cerr << argv[0] << ": " << e.what() << '\n'
					  << *this;

			// just exit here since we don't want to print anything after the help
			// call _Exit instead of exit to not trigger the exit handler in main.cpp
			_Exit(EXIT_FAILURE);
		}
	}

private:
	void copy_arg_values()
	{
		audio_file = get("audio_file");

		if ((fft_size = get<int>("-n")) & 1)
			throw std::invalid_argument("sample size must be even!");

		characters = get("-c");
		multiplier = get<float>("-m");

		// nth root
		try
		{
			nth_root = get<float>("--nth-root");
		}
		catch (const std::logic_error &e)
		{
		}

		if (!nth_root)
			throw std::invalid_argument("nth_root cannot be zero!");

		// peak character
		try
		{
			peak_char = get("--peak-char").front();
		}
		catch (const std::logic_error &e)
		{
			peak_char = 0;
		}

		{ // interpolation type
			const auto &interp_str = get("-i");
			if (interp_str == "none")
				interp = FrequencySpectrum::InterpolationType::NONE;
			else if (interp_str == "linear")
				interp = FrequencySpectrum::InterpolationType::LINEAR;
			else if (interp_str == "cspline")
				interp = FrequencySpectrum::InterpolationType::CSPLINE;
			else if (interp_str == "cspline_hermite")
				interp = FrequencySpectrum::InterpolationType::CSPLINE_HERMITE;
			else
				throw std::logic_error("????");
		}

		{ // spectrum coloring type
			const auto &color_str = get("--color");
			if (color_str == "wheel")
			{
				color = ColorType::WHEEL;
				const auto &hsv_strs = get<std::vector<std::string>>("--hsv");
				if (hsv_strs.size())
					hsv = {std::stof(hsv_strs[0]), std::stof(hsv_strs[1]), std::stof(hsv_strs[2])};
			}
			else if (color_str == "solid")
			{
				color = ColorType::SOLID;
				const auto &rgb_strs = get<std::vector<std::string>>("--rgb");
				if (!rgb_strs.size())
					throw std::invalid_argument("option '--rgb' required by '--color solid'");
				rgb = {std::stoi(rgb_strs[0]), std::stoi(rgb_strs[1]), std::stoi(rgb_strs[2])};
			}
			else if (color_str == "none")
				color = ColorType::NONE;
			else
				throw std::logic_error("?????????");
		}

		{ // frequency scale (x-axis)
			const auto &scale = get("-s");
			if (scale == "linear")
				this->scale = FrequencySpectrum::Scale::LINEAR;
			else if (scale == "log")
				this->scale = FrequencySpectrum::Scale::LOG;
			else if (scale == "nth-root")
				this->scale = FrequencySpectrum::Scale::NTH_ROOT;
			else
				throw std::logic_error("impossible!!!!");
		}
	}
};
#pragma once

#include <memory>
#include <argparse/argparse.hpp>
#include "FrequencySpectrum.hpp"
#include "termviz.hpp"

using argparse::ArgumentParser;

class Args : private ArgumentParser
{
	using ColorType = termviz::ColorType;
	using Scale = FrequencySpectrum::Scale;
	using InterpType = FrequencySpectrum::InterpType;
	using AccumulationMethod = FrequencySpectrum::AccumulationMethod;
	using WindowFunction = FrequencySpectrum::WindowFunction;

public:
	Args(const int argc, const char *const *const argv)
		: ArgumentParser(argv[0])
	{
		add_argument("audio_file")
			.help("audio file to visualize and play");

		add_argument("-n", "--sample-size")
			.help("number of samples (or frames of samples) to process at a time\n- higher -> increases accuracy\n- lower -> increases responsiveness")
			.default_value(3000)
			.scan<'i', int>()
			.validate();

		add_argument("-c", "--spectrum-chars")
			.help("characters to render columns with\nif more than 1 character is given, --peak-char is recommended")
			.default_value("#");
		add_argument("--peak-char")
			.help("character to print at the highest point for each column");

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

		add_argument("-a", "--accum")
			.help("frequency bin accumulation method\n- 'sum': greater treble detail, exaggerated amplitude\n- 'max': less treble detail, true-to-waveform amplitude")
			.choices("sum", "max")
			.nargs(1)
			.default_value("sum")
			.validate();

		add_argument("-w", "--window")
			.help("set window function to use, or 'none'.\nwindow functions can reduce 'wiggling' in bass frequencies.\nhowever they can reduce overall amplitude, so adjust '-m' accordingly.")
			.choices("none", "hanning", "hamming", "blackman")
			.default_value("blackman")
			.validate();

		add_argument("-i", "--interpolation")
			.help("spectrum interpolation type")
			.choices("none", "linear", "cspline", "cspline_hermite")
			.default_value("cspline")
			.validate();

		add_argument("-m", "--multiplier")
			.help("multiply spectrum amplitude by this amount")
			.default_value(4.f)
			.scan<'f', float>()
			.validate();

		add_argument("--color")
			.help("enable a colorful spectrum!")
			.choices("wheel", "solid", "none")
			.default_value("wheel")
			.validate();
		add_argument("--wheel-rate")
			.help("requires '--color wheel'\nmoves the colors on the spectrum with time!\nvalue must be between [0, 1] - 0.005 is a good start")
			.default_value(0.f)
			.scan<'f', float>()
			.validate();
		add_argument("--hsv")
			.help("requires '--color wheel'\nchoose a hue offset for the color wheel, saturation, and brightness\nvalues must be between [0, 1]")
			.nargs(3)
			.validate();
		add_argument("--rgb")
			.help("requires '--color solid'\nrenders the spectrum with a solid color\nmust provide space-separated rgb integers")
			.nargs(3)
			.validate();

		try
		{
			parse_args(argc, argv);
		}
		catch (const std::exception &e)
		{
			// print error and help to stderr
			std::cerr << argv[0] << ": " << e.what() << '\n'
					  << *this;

			// just exit here since we don't want to print anything after the help
			_Exit(EXIT_FAILURE);
		}
	}

	std::unique_ptr<termviz> to_termviz()
	{
		std::unique_ptr<termviz> tv(new termviz(get("audio_file")));

		int fft_size;
		if ((fft_size = get<int>("-n")) & 1)
			throw std::invalid_argument("sample size must be even!");
		tv->set_sample_size(fft_size);

		tv->set_characters(get("-c"));
		tv->set_multiplier(get<float>("-m"));

		// peak character
		try
		{
			tv->set_peak_char(get("--peak-char").front());
		}
		catch (const std::logic_error &e)
		{
		}

		{ // accumulation method
			const auto &acc_method_str = get("-a");
			if (acc_method_str == "sum")
				tv->set_accum_method(AccumulationMethod::SUM);
			else if (acc_method_str == "max")
				tv->set_accum_method(AccumulationMethod::MAX);
			else
				throw std::invalid_argument("unknown accumulation methpd: " + acc_method_str);
		}

		{ // interpolation type
			const auto &interp_str = get("-i");
			if (interp_str == "none")
				tv->set_interp_type(InterpType::NONE);
			else if (interp_str == "linear")
				tv->set_interp_type(InterpType::LINEAR);
			else if (interp_str == "cspline")
				tv->set_interp_type(InterpType::CSPLINE);
			else if (interp_str == "cspline_hermite")
				tv->set_interp_type(InterpType::CSPLINE_HERMITE);
			else
				throw std::invalid_argument("unknown interpolation type: " + interp_str);
		}

		{ // spectrum coloring type
			const auto &color_str = get("--color");
			if (color_str == "wheel")
			{
				tv->set_color_type(ColorType::WHEEL);
				const auto &hsv_strs = get<std::vector<std::string>>("--hsv");
				if (hsv_strs.size())
					tv->set_wheel_hsv({std::stof(hsv_strs[0]), std::stof(hsv_strs[1]), std::stof(hsv_strs[2])});
				tv->set_wheel_rate(get<float>("--wheel-rate"));
			}
			else if (color_str == "solid")
			{
				tv->set_color_type(ColorType::SOLID);
				const auto &rgb_strs = get<std::vector<std::string>>("--rgb");
				if (rgb_strs.size())
					tv->set_solid_color({std::stoi(rgb_strs[0]), std::stoi(rgb_strs[1]), std::stoi(rgb_strs[2])});
			}
			else if (color_str == "none")
				tv->set_color_type(ColorType::NONE);
			else
				throw std::invalid_argument("unknown coloring type: " + color_str);
		}

		{ // frequency scale (x-axis)
			const auto &scale_str = get("-s");
			if (scale_str == "linear")
				tv->set_scale(Scale::LINEAR);
			else if (scale_str == "log")
				tv->set_scale(Scale::LOG);
			else if (scale_str == "nth-root")
			{
				tv->set_scale(Scale::NTH_ROOT);
				const auto nth_root = get<float>("--nth-root");
				if (!nth_root)
					throw std::invalid_argument("nth_root cannot be zero!");
				tv->set_nth_root(nth_root);
			}
			else
				throw std::invalid_argument("unknown scale: " + scale_str);
		}

		return tv;
	}
};
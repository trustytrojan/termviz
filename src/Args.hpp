#pragma once

#include <argparse/argparse.hpp>

using argparse::ArgumentParser;

struct Args : private ArgumentParser
{
	enum class Scale
	{
		LINEAR,
		LOG,
		SQRT
	};

	std::string audio_file;
	int fft_size;

	struct
	{
		float multiplier;
		char character, peak_char;
	} spectrum;

	// spectrum scale
	Scale scale;

	// whether color is enabled
	bool color;

	// hue offset, saturation, value
	std::tuple<float, float, float> hsv{0, 1, 1};

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
		add_argument("-c", "--spectrum-char")
			.help("character to render the spectrum with")
			.default_value("#");
		add_argument("--peak-char")
			.help("character to print at the highest point of frequency bins");
		add_argument("-s", "--scale")
			.help("spectrum frequency scale")
			.choices("linear", "log", "sqrt")
			.default_value("log")
			.validate();
		add_argument("-m", "--multiplier")
			.help("multiply spectrum amplitude by this amount")
			.default_value(3.f)
			.scan<'f', float>()
			.validate();
		add_argument("--color")
			.help("display a colorful spectrum!")
			.flag();
		add_argument("--hsv")
			.help("choose a hue offset, saturation, and brightness for --color\nvalues must be between [0, 1]\nwill be ignored if --color is absent")
			.nargs(3)
			.validate();

		try
		{
			parse_args(argc, argv);
		}
		catch (const std::exception &e)
		{
			// print error and help to stderr
			std::cerr << argv[0] << ": " << e.what() << '\n' << *this;

			// just exit here since we don't want to print anything after the help
			exit(EXIT_FAILURE);
		}

		audio_file = get("audio_file");

		if ((fft_size = get<int>("-n")) & 1)
			throw std::invalid_argument("sample size must be even!");

		spectrum.character = get("-c").front();
		spectrum.multiplier = get<float>("-m");

		try
		{
			spectrum.peak_char = get("--peak-char").front();
		}
		catch (const std::logic_error &e)
		{
			spectrum.peak_char = spectrum.character;
		}

		if ((color = get<bool>("--color")))
		{
			try
			{
				const auto &hsv_strs = get<std::vector<std::string>>("--hsv");
				if (!hsv_strs.size())
					throw std::logic_error("????");
				hsv = {std::stof(hsv_strs[0]), std::stof(hsv_strs[1]), std::stof(hsv_strs[2])};
			}
			catch (const std::invalid_argument &e)
			{
				std::cerr << "all arguments to --hsv must represent floating point numbers\n";
				throw;
			}
			catch (const std::logic_error &e)
			{
				// --hsv wasn't passed, which is fine, we have defaults
			}
		}

		const auto &scale = get("-s");
		if (scale == "linear")
			this->scale = Scale::LINEAR;
		else if (scale == "log")
			this->scale = Scale::LOG;
		else if (scale == "sqrt")
			this->scale = Scale::SQRT;
		else
			throw std::logic_error("impossible!!!!");
	}
};
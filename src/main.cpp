#include <sndfile.hh>
#include "Args.hpp"
#include "termviz.hpp"

void exit_handler()
{
	std::cout << "\ec";
}

int main(const int argc, const char *const *const argv)
{
	atexit(exit_handler);
	try
	{
		Args args(argc, argv);
		termviz(args.audio_file)
			.set_scale(args.scale)
			.set_interp_type(args.interp)
			.set_nth_root(args.nth_root)
			.set_sample_size(args.fft_size)
			.set_characters(args.characters)
			.set_peak_char(args.peak_char)
			.set_color_type(args.color)
			.set_wheel_rate(args.wheel_rate)
			.set_rgb(args.rgb)
			.set_hsv(args.hsv)
			.set_multiplier(args.multiplier)
			.start();
	}
	catch (const std::exception &e)
	{
		std::cerr << argv[0] << ": " << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

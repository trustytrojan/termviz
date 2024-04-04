#include <signal.h>
#include <sndfile.hh>
#include "Args.hpp"
#include "termviz.hpp"

int main(const int argc, const char *const *const argv)
{
	try
	{
		Args(argc, argv).to_termviz()->start();
	}
	catch (const std::exception &e)
	{
		std::cerr << argv[0] << ": " << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

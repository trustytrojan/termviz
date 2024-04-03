#pragma once

#include <sys/ioctl.h>
#include <unistd.h>

struct TerminalSize
{
	int width, height;

	TerminalSize()
	{
		winsize ws;
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
			throw std::runtime_error(std::string("ioctl: ") + strerror(errno));
		width = ws.ws_col;
		height = ws.ws_row;
	}
};
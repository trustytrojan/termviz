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
		{
			perror("ioctl");
			throw std::runtime_error("ioctl() failed");
		}

		width = ws.ws_col;
		height = ws.ws_row;
	}
};
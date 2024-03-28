# termviz
terminal audio frequency spectrum visualizer

## features
- customizable sample size (`-n`) to vary responsiveness and precision
- if your terminal supports truecolor, termviz can render a full 8-bit rgb spectrum
	- the color spectrum is customizable using the `--hsv` argument
- customizable frequency scale: can choose from `linear`, `log`, or `sqrt` (more to come)

## building
1. install project [dependencies](#dependencies) on your system
2. run `make` in the root directory

## dependencies
- [libsndfile/libsndfile](https://github.com/libsndfile/libsndfile)
- [PortAudio/portaudio](https://github.com/PortAudio/portaudio)
- [p-ranav/argparse](https://github.com/p-ranav/argparse)
- [mborgerding/kissfft](https://github.com/mborgerding/kissfft) - needs to be compiled and installed on your system either as a dynamic or static library; you can follow its build guide [here](https://github.com/mborgerding/kissfft?tab=readme-ov-file#building).
- [ttk592/spline](https://github.com/ttk592/spline) - already included [in this repo](/src/spline.hpp).

## todo
- figure out better dependency management
- add interpolation options other than cubic spline
- add more color options than just the color wheel, such as solid, striped, etc
- for color wheel, add option to move color wheel during playback
- add a tui for playback controls, settings, and audio metadata (currently looking at [TermOx](https://github.com/a-n-t-h-o-n-y/TermOx))
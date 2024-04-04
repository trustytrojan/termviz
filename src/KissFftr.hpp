#pragma once

#include <kiss_fftr.h>

class KissFftr
{
	kiss_fftr_cfg cfg;

public:
	/**
	 * Initializes a `kiss_fftr_cfg` internally.
	 * `kissfft` requires an even `fft_size` because the data it outputs is half that size.
	 * @param fft_size nonzero sample chunk size fed into the `transform` method
	 * @throws `std::invalid_argument` if `fft_size` is zero or not even.
	 */
	KissFftr(const int fft_size)
	{
		if (!fft_size)
			throw std::invalid_argument("fft_size must be nonzero");
		if (fft_size & 1)
			throw std::invalid_argument("fft_size must be even");
		cfg = kiss_fftr_alloc(fft_size, false, NULL, NULL);
	}

	~KissFftr()
	{
		kiss_fftr_free(cfg);
	}

	/**
	 * Set the FFT size, aka the sample chunk size fed into the `transform` method.
	 * `kissfft` requires an even `fft_size` because the data it outputs is half that size.
	 * @param fft_size new fft size to use
	 * @throws `std::invalid_argument` if `fft_size` is zero or not even
	 */
	void set_fft_size(const int fft_size)
	{
		if (!fft_size)
			throw std::invalid_argument("fft_size must be nonzero");
		if (fft_size & 1)
			throw std::invalid_argument("fft_size must be even");
		kiss_fftr_free(cfg);
		cfg = kiss_fftr_alloc(fft_size, false, NULL, NULL);
	}

	/**
	 * Perform FFT on an input waveform, passed as `timedata`.
	 * The output frequency data will be written to `freqdata`.
	 * The `kissfft` library requires that `timedata` be an array of size `fft_size`, and that `freqdata` be of size `fft_size / 2 + 1`.
	 * If the sizes of the arrays are less than `kissfft` expects, memory errors may occur.
	 * @param timedata input wave data
	 * @param freqdata output frequency data
	 * @throws `std::invalid_argument` if either `timedata` or `freqdata` are null
	 */
	void transform(const float *const timedata, kiss_fft_cpx *const freqdata)
	{
		if (!timedata)
			throw std::invalid_argument("timedata is null");
		if (!freqdata)
			throw std::invalid_argument("freqdata is null");
		kiss_fftr(cfg, timedata, freqdata);
	}
};
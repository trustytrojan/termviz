#pragma once

#include <kiss_fftr.h>

class KissFftr
{
	kiss_fftr_cfg cfg;

public:
	KissFftr(const int fft_size, const bool inverse = false)
		: cfg(kiss_fftr_alloc(fft_size, inverse, NULL, NULL)) {}

	~KissFftr()
	{
		kiss_fftr_free(cfg);
	}

	void set_cfg(const int fft_size, const bool inverse = false)
	{
		kiss_fftr_free(cfg);
		cfg = kiss_fftr_alloc(fft_size, inverse, NULL, NULL);
	}

	void transform(const float *const timedata, kiss_fft_cpx *const freqdata)
	{
		kiss_fftr(cfg, timedata, freqdata);
	}
};
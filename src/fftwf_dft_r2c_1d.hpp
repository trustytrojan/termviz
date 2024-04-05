#pragma once

#include <fftw3.h>

class fftwf_dft_r2c_1d
{
	int N;
	float *in;
	int output_size;
	fftwf_complex *out;
	fftwf_plan p;

	void init(const int N)
	{
		this->N = N;
		in = (float *)fftwf_malloc(sizeof(float) * N);
		output_size = N / 2 + 1;
		out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * output_size);
		p = fftwf_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
	}

	void cleanup()
	{
		fftwf_destroy_plan(p);
		fftwf_free(in);
		fftwf_free(out);
	}

public:
	fftwf_dft_r2c_1d(const int N) { init(N); }
	~fftwf_dft_r2c_1d() { cleanup(); }

	void set_n(const int N)
	{
		if (this->N == N)
			return;
		cleanup();
		init(N);
	}

	void execute()
	{
		fftwf_execute(p);
	}

	float *get_input()
	{
		return in;
	}

	const fftwf_complex *get_output()
	{
		return out;
	}

	int get_output_size() const
	{
		return output_size;
	}
};
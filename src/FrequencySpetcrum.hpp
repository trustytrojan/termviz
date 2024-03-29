#pragma once

#include <stdexcept>
#include <vector>
#include "KissFftr.hpp"
#include "spline.hpp"

class FrequencySpectrum
{
public:
	enum class Scale
	{
		LINEAR,
		LOG,
		SQRT
	};

private:
	KissFftr kf;
	std::vector<kiss_fft_cpx> freqdata;
	double logmax, sqrtmax;
	tk::spline spline;
	tk::spline::spline_type spline_type;
	Scale scale;

public:
	FrequencySpectrum(const int fft_size,
					  const Scale scale = Scale::LOG,
					  const tk::spline::spline_type spline_type = tk::spline::cspline)
		: kf(fft_size),
		  freqdata(fft_size / 2 + 1),
		  logmax(log(freqdata.size())),
		  sqrtmax(sqrt(freqdata.size())),
		  spline_type(spline_type),
		  scale(scale) {}

	void set_fft_size(const int fft_size)
	{
		kf.set_cfg(fft_size);
		freqdata.resize(fft_size / 2 + 1);
		logmax = log(freqdata.size());
		sqrtmax = sqrt(freqdata.size());
	}

	/**
	 * Set the type of spline to interpolate missing points with.
	 * @param spline_type new spline type to use
	 */
	void set_spline_type(const tk::spline::spline_type spline_type)
	{
		this->spline_type = spline_type;
	}

	/**
	 * Set the spectrum's frequency scale.
	 * @param scale new scale to use
	 */
	void set_scale(const Scale scale)
	{
		this->scale = scale;
	}

	/**
	 * Render the spectrum for `this->fft_size` samples of input.
	 * @param timedata Pointer to wave data. Must be of size `this->fft_size`, otherwise dire things may happen.
	 * @param spectrum Output vector to store the rendered spectrum.
	 *                 The size of this vector is used for bin-mapping of frequencies,
	 *                 so make sure you set it correctly.
	 */
	void render(const float *const timedata, std::vector<float> &spectrum)
	{
		// perform fft: frequency range to amplitude values are stored in freqdata
		kf.transform(timedata, freqdata.data());

		// zero out array since we are creating sums
		std::ranges::fill(spectrum, 0);

		// map frequency bins of freqdata to amplitudes
		for (auto i = 0; i < (int)freqdata.size(); ++i)
		{
			const auto [re, im] = freqdata[i];
			const float amplitude = sqrt((re * re) + (im * im));
			spectrum[calc_index(i, (int)spectrum.size() - 1)] += amplitude;
		}

		// apply spline if scale is not linear (no gaps are present when linear)
		if (scale != Scale::LINEAR)
			smooth_spectrum(spectrum);
	}

private:
	/**
	 * Calculate the index to add amplitudes to in the spectrum vector (in `render`).
	 * The returned index will be in the range `[0, max_index]`.
	 */
	int calc_index(const int i, const int max_index)
	{
		return std::max(0, std::min((int)(calc_index_ratio(i) * max_index), max_index));
	}

	float calc_index_ratio(const float i)
	{
		switch (scale)
		{
		case Scale::LINEAR:
			return i / freqdata.size();
		case Scale::LOG:
			return log(i ? i : 1) / logmax;
		case Scale::SQRT:
			return sqrt(i) / sqrtmax;
		default:
			throw std::logic_error("impossible!!!!!!!");
		}
	}

	void smooth_spectrum(std::vector<float> &spectrum)
	{
		// separate the nonzero values (y's) and their indices (x's)
		std::vector<double> nonzero_values, indices;
		for (int i = 0; i < (int)spectrum.size(); ++i)
		{
			if (!spectrum[i])
				continue;
			nonzero_values.push_back(spectrum[i]);
			indices.push_back(i);
		}

		// tk::spline::set_points throws if there are less than 3 points
		// plus, if there are less than 3 points, we wouldn't be smoothing anything
		if (indices.size() < 3)
			return;

		spline.set_points(indices, nonzero_values, spline_type);

		// only copy spline values to fill in the gaps
		for (int i = 0; i < (int)spectrum.size(); ++i)
			spectrum[i] = spectrum[i] ? spectrum[i] : spline(i);
	}
};
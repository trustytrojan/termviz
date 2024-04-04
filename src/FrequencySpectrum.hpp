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
		NTH_ROOT
	};

	enum class InterpType
	{
		NONE,
		LINEAR = tk::spline::linear,
		CSPLINE = tk::spline::cspline,
		CSPLINE_HERMITE = tk::spline::cspline_hermite
	};

private:
	KissFftr kf;
	std::vector<kiss_fft_cpx> freqdata;
	int nth_root = 2;
	tk::spline spline;
	InterpType interp;
	Scale scale;
	float fftsize_inv;

	struct
	{
		double log, sqrt, cbrt, nthroot;
		void set(const FrequencySpectrum &fs)
		{
			const auto max = fs.freqdata.size();
			log = ::log(max);
			sqrt = ::sqrt(max);
			cbrt = ::cbrt(max);
			nthroot = ::pow(max, 1. / fs.nth_root);
		}
	} scale_max;

public:
	/**
	 * Initialize frequency spectrum renderer.
	 * @param fft_size sample chunk size fed into the `transform` method
	 */
	FrequencySpectrum(const int fft_size,
					  const Scale scale = Scale::LOG,
					  const InterpType interp = InterpType::CSPLINE)
		: kf(fft_size),
		  freqdata(fft_size / 2 + 1),
		  interp(interp),
		  scale(scale)
	{
		scale_max.set(*this);
	}

	/**
	 * Set the FFT size used in the `kissfft` library.
	 * @param fft_size new fft size to use
	 * @throws `std::invalid_argument` if `fft_size` is not even
	 */
	void set_fft_size(const int fft_size)
	{
		kf.set_fft_size(fft_size);
		freqdata.resize(fft_size / 2 + 1);
		fftsize_inv = 1. / fft_size;
		scale_max.set(*this);
	}

	/**
	 * Set interpolation type.
	 * @param interp new interpolation type to use
	 */
	void set_interp_type(const InterpType interp_type)
	{
		this->interp = interp_type;
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
	 * Set the nth-root to use when using the `NTH_ROOT` scale.
	 * @param nth_root new nth_root to use
	 * @throws `std::invalid_argument` if `nth_root` is zero
	 */
	void set_nth_root(const int nth_root)
	{
		if (!nth_root)
			throw std::invalid_argument("preventing division by zero");
		this->nth_root = nth_root;
	}

	/**
	 * Render the spectrum for `fft_size` samples of input. This was the amount set in the constructor or in `set_fft_size`.
	 * @param timedata Pointer to wave data. Must be of size `fft_size`, otherwise dire things may happen.
	 * @param spectrum Output vector to store the rendered spectrum.
	 *                 The size of this vector is used for bin-mapping of frequencies,
	 *                 so make sure you set it correctly.
	 * @throws `std::invalid_argument` if `timedata` is null
	 */
	void render(const float *const timedata, std::vector<float> &spectrum)
	{
		// perform fft: frequency range to amplitude values are stored in freqdata
		// throws if either argument is null
		kf.transform(timedata, freqdata.data());

		// zero out array since we are creating sums
		std::ranges::fill(spectrum, 0);

		// map frequency bins of freqdata to spectrum
		for (auto i = 0; i < (int)freqdata.size(); ++i)
		{
			const auto [re, im] = freqdata[i];
			const float amplitude = sqrt((re * re) + (im * im));
			const auto index = calc_index(i, (int)spectrum.size() - 1);

			// max strategy - best strategy overall, but there is no signature of treble frequencies!
			// spectrum[index] = std::max(spectrum[index], amplitude);

			// TODO: need to find a stragety to preserve treble signature without losing it
			
			// sum strategy - works PERFECTLY, but might be exaggerating treble frequencies
			// until i find a new strategy, i'm sticking with this because it looks cooler
			spectrum[index] += amplitude;
		}

		// downscale all amplitudes by 1 / fft_size
		// this is because with smaller fft_size's, frequency bins are bigger
		// so more frequencies get lumped together, causing higher amplitudes per bin.
		for (auto &a : spectrum)
			a *= fftsize_inv;

		// apply interpolation if necessary
		if (interp != InterpType::NONE && scale != Scale::LINEAR)
			interpolate(spectrum);
	}

private:
	int calc_index(const int i, const int max_index)
	{
		return std::max(0, std::min((int)(calc_index_ratio(i) * max_index), max_index - 1));
	}

	float calc_index_ratio(const float i)
	{
		switch (scale)
		{
		case Scale::LINEAR:
			return i / freqdata.size();
		case Scale::LOG:
			return log(i ? i : 1) / scale_max.log;
		case Scale::NTH_ROOT:
			switch (nth_root)
			{
			case 1:
				return i / freqdata.size();
			case 2:
				return sqrt(i) / scale_max.sqrt;
			case 3:
				return cbrt(i) / scale_max.cbrt;
			default:
				return pow(i, 1 / nth_root) / scale_max.nthroot;
			}
		default:
			throw std::logic_error("impossible!!!!!!!");
		}
	}

	void interpolate(std::vector<float> &spectrum)
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

		spline.set_points(indices, nonzero_values, (tk::spline::spline_type)interp);

		// only copy spline values to fill in the gaps
		for (int i = 0; i < (int)spectrum.size(); ++i)
			spectrum[i] = spectrum[i] ? spectrum[i] : spline(i);
	}
};
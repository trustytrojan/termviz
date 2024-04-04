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

	enum class AccumulationMethod
	{
		SUM,
		MAX
	};

	enum class WindowFunction
	{
		NONE,
		HANNING,
		HAMMING,
		BLACKMAN
	};

private:
	// fft size
	int fft_size;
	float fftsize_inv = 1.f / fft_size;

	// nth root
	int nth_root = 2;
	float nth_root_inverse = 1.f / nth_root;

	// kiss_fftr initialization
	KissFftr kf = KissFftr(fft_size);

	// vector to store output of fft
	std::vector<kiss_fft_cpx> freqdata = std::vector<kiss_fft_cpx>(fft_size / 2 + 1);

	// interpolation
	tk::spline spline;
	InterpType interp = InterpType::CSPLINE;

	// output spectrum scale
	Scale scale = Scale::LOG;

	// method for accumulating amplitudes in frequency bins
	AccumulationMethod am = AccumulationMethod::SUM;

	// window function
	WindowFunction wf = WindowFunction::BLACKMAN;

	// struct to hold the "max"s used in `calc_index_ratio`
	struct
	{
		double log, sqrt, cbrt, nthroot;
		void set(const FrequencySpectrum &fs)
		{
			const auto max = fs.freqdata.size();
			log = ::log(max);
			sqrt = ::sqrt(max);
			cbrt = ::cbrt(max);
			nthroot = ::pow(max, fs.nth_root_inverse);
		}
	} scale_max;

public:
	/**
	 * Initialize frequency spectrum renderer.
	 * @param fft_size sample chunk size fed into the `transform` method
	 */
	FrequencySpectrum(const int fft_size) : fft_size(fft_size)
	{
		scale_max.set(*this);
	}

	/**
	 * Set the FFT size used in the `kissfft` library.
	 * @param fft_size new fft size to use
	 * @returns reference to self
	 * @throws `std::invalid_argument` if `fft_size` is not even
	 */
	FrequencySpectrum &set_fft_size(const int fft_size)
	{
		kf.set_fft_size(fft_size);
		freqdata.resize(fft_size / 2 + 1);
		fftsize_inv = 1. / fft_size;
		scale_max.set(*this);
		return *this;
	}

	/**
	 * Set interpolation type.
	 * @param interp new interpolation type to use
	 * @returns reference to self
	 */
	FrequencySpectrum &set_interp_type(const InterpType interp)
	{
		this->interp = interp;
		return *this;
	}

	/**
	 * Set window function.
	 * @param interp new window function to use
	 * @returns reference to self
	 */
	FrequencySpectrum &set_window_func(const WindowFunction wf)
	{
		this->wf = wf;
		return *this;
	}

	/**
	 * Set frequency bin accumulation method.
	 * @param interp new accumulation method to use
	 * @returns reference to self
	 */
	FrequencySpectrum &set_accum_method(const AccumulationMethod am)
	{
		this->am = am;
		return *this;
	}

	/**
	 * Set the spectrum's frequency scale.
	 * @param scale new scale to use
	 * @returns reference to self
	 */
	FrequencySpectrum &set_scale(const Scale scale)
	{
		this->scale = scale;
		return *this;
	}

	/**
	 * Set the nth-root to use when using the `NTH_ROOT` scale.
	 * @param nth_root new nth_root to use
	 * @returns reference to self
	 * @throws `std::invalid_argument` if `nth_root` is zero
	 */
	FrequencySpectrum &set_nth_root(const int nth_root)
	{
		if (!nth_root)
			throw std::invalid_argument("FrequencySpectrun::set_nth_root: nth_root cannot be zero!");
		this->nth_root = nth_root;
		nth_root_inverse = 1.f / nth_root;
		return *this;
	}

	/**
	 * Render the spectrum for `fft_size` samples of input. This was the amount set in the constructor or in `set_fft_size`.
	 * @param timedata Pointer to wave data. Must be of size `fft_size`, otherwise dire things may happen.
	 * @param spectrum Output vector to store the rendered spectrum.
	 *                 The size of this vector is used for bin-mapping of frequencies,
	 *                 so make sure you set it correctly.
	 * @throws `std::invalid_argument` if `timedata` is null
	 */
	void render(float *const timedata, std::vector<float> &spectrum)
	{
		apply_window_func(timedata);

		// perform fft: frequency range to amplitude values are stored in freqdata
		// throws if either argument is null
		kf.transform(timedata, freqdata.data());

		// zero out array since we are accumulating
		std::ranges::fill(spectrum, 0);

		// map frequency bins of freqdata to spectrum
		for (auto i = 0; i < (int)freqdata.size(); ++i)
		{
			const auto [re, im] = freqdata[i];
			const float amplitude = sqrt((re * re) + (im * im));
			const auto index = calc_index(i, spectrum.size());

			switch (am)
			{
			case AccumulationMethod::SUM:
				spectrum[index] += amplitude;
				break;
			
			case AccumulationMethod::MAX:
				spectrum[index] = std::max(spectrum[index], amplitude);
				break;
			
			default:
				throw std::logic_error("FrequencySpectrum::render: switch(accum_type): default case hit");
			}
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
	void apply_window_func(float *const timedata)
	{
		switch (wf)
		{
		case WindowFunction::HANNING:
			for (int i = 0; i < fft_size; ++i)
				timedata[i] *= 0.5f * (1 - cos(2 * M_PI * i / (fft_size - 1)));
			break;

		case WindowFunction::HAMMING:
			for (int i = 0; i < fft_size; ++i)
				timedata[i] *= 0.54f - 0.46f * cos(2 * M_PI * i / (fft_size - 1));
			break;

		case WindowFunction::BLACKMAN:
			for (int i = 0; i < fft_size; ++i)
				timedata[i] *= 0.42f - 0.5f * cos(2 * M_PI * i / (fft_size - 1)) + 0.08f * cos(4 * M_PI * i / (fft_size - 1));
			break;

		case WindowFunction::NONE:
			break;

		default:
			throw std::logic_error("FrequencySpectrum::apply_window_func: default case hit");
		}
	}

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
			// TODO: make log curve shift with fft_size (better approach: horizontal shrink)
			// this will avoid the shifting down of all frequencies as fft_size decreases.
			return std::log(i ? i : 1) / scale_max.log;
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
				return pow(i, nth_root_inverse) / scale_max.nthroot;
			}
		default:
			throw std::logic_error("FrequencySpectrum::calc_index_ratio: default case hit");
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
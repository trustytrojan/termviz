#pragma once

#include <stdexcept>
#include "spline.hpp"

struct SmoothedAmplitudes : std::vector<float>
{
	SmoothedAmplitudes(std::vector<float> &amplitudes)
		: std::vector<float>(amplitudes.size())
	{
		// Separate the nonzero values (y's) and their indices (x's)
		std::vector<double> nonzero_values, indices;

		for (int i = 0; i < (int)size(); ++i)
		{
			if (!amplitudes[i])
				continue;
			nonzero_values.push_back(amplitudes[i]);
			indices.push_back(i);
		}

		if (indices.size() >= 3)
		{
			// Create a cubic spline interpolation based on the nonzero values
			tk::spline s(indices, nonzero_values);

			// Use the cubic spline to estimate a value for each position in the list
			for (int i = 0; i < (int)size(); ++i)
				at(i) = amplitudes[i] ? amplitudes[i] : s(i);
		}
		else
			this->swap(amplitudes);
	}
};

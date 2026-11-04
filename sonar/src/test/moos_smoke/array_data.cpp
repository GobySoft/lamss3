// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Usage: array_data <file> <elements> <spacing_m> <samples> <sample_rate_hz> <frequency_hz>
//                   <bearing_deg> <seed>
//
// Writes the raw float32 time series pLamssCBF reads for a DURIP, HYD or VSA array (all samples
// of element 1, then element 2, ...): a plane wave of one frequency arriving at <bearing_deg>
// from the array axis (90 is broadside) on a uniform line array, sound speed 1500 m/s, in white
// noise of the same power on each element.

#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 9)
    {
        std::cerr << "usage: array_data <file> <elements> <spacing_m> <samples> "
                     "<sample_rate_hz> <frequency_hz> <bearing_deg> <seed>"
                  << std::endl;
        return 2;
    }
    const int elements = std::stoi(argv[2]);
    const double spacing = std::stod(argv[3]);
    const int samples = std::stoi(argv[4]);
    const double sample_rate = std::stod(argv[5]);
    const double frequency = std::stod(argv[6]);
    const double bearing = std::stod(argv[7]) * M_PI / 180;
    const double sound_speed = 1500;
    std::mt19937 rng(std::stoul(argv[8]));
    std::normal_distribution<float> noise(0, M_SQRT1_2);

    std::ofstream out(argv[1], std::ios::binary);
    std::vector<float> series(samples);
    for (int e = 0; e < elements; ++e)
    {
        const double delay = e * spacing * std::cos(bearing) / sound_speed;
        for (int s = 0; s < samples; ++s)
            series[s] = std::sin(2 * M_PI * frequency * (s / sample_rate - delay)) + noise(rng);
        out.write(reinterpret_cast<const char*>(series.data()), samples * sizeof(float));
    }
    return out.good() ? 0 : 1;
}

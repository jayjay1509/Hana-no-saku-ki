#pragma once

struct SimulationSettings
{
    bool useMaxFlow = false;
    float maxFlow = 0.25f;

    bool useViscosity = false;
    float viscosity = 1.0f;

    bool useCompressibility = false;
    float compressibility = 0.25f;

    bool alternateUpdateDirection = false;
    bool stableNoise = false;
    bool circularBrush = false;
};

extern SimulationSettings settings;

void resetSettings();

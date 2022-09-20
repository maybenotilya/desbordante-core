#pragma once

#include "Datasets.h"
#include "FDAlgorithm.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "ProgramOptionStrings.h"

template <typename T>
class AlgorithmTest : public LightDatasets, public HeavyDatasets, public ::testing::Test {
protected:
    std::unique_ptr<FDAlgorithm> CreateAlgorithmInstance(
        std::filesystem::path const& path, char separator = ',', bool has_header = true) {
        namespace posr = program_option_strings;

        FDAlgorithm::Config c{ .data = path, .separator = separator, .has_header = has_header };
        c.special_params[posr::kError] = 0.0;
        c.special_params[posr::kSeed] = 0;
        return std::make_unique<T>(c);
    }
};

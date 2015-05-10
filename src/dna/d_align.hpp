#ifndef GI_D_ALIGN_HPP
#define GI_D_ALIGN_HPP

#include "analyzer.hpp"

namespace Spike
{
    struct DAlignStats
    {
        // Overall performance for the base-level
        Confusion m;
        
        // Overlal LOR for the base-level
        Sensitivity s;
    };

    struct DAlign
    {
        struct Options : public AnalyzerOptions
        {
            // Empty Implementation
        };

        static DAlignStats analyze(const std::string &file, const Options &options = Options());
    };
}

#endif
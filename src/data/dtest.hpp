#ifndef DIFF_TEST_HPP
#define DIFF_TEST_HPP

#include <cmath>
#include "data/data.hpp"

namespace Anaquin
{
    struct DiffTest
    {
        enum class Status
        {
            Tested,
            NotTested,
        };

        ChrID cID;

        GeneID gID;
        IsoformID iID;
        
        Measured samp1 = NAN;
        Measured samp2 = NAN;
        
        // Log-fold ratio
        LogFold logF_;

        // The p-value and q-value under null hypothesis
        Probability p, q;

        Status status = Status::Tested;
        
        /*
         * Optional inputs (not always available)
         */
        
        // Normalized average counts
        double mean = NAN;

        // Standard error for the log-fold
        double logFSE = NAN;
    };
}

#endif

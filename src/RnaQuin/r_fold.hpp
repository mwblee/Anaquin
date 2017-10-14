/*
 * Copyright (C) 2016 - Garvan Institute of Medical Research
 *
 *  Ted Wong, Bioinformatic Software Engineer at Garvan Institute.
 */

#ifndef R_FOLD_HPP
#define R_FOLD_HPP

#include "data/dtest.hpp"
#include <boost/format.hpp>
#include "stats/analyzer.hpp"

// Defined in resources.cpp
extern Anaquin::Scripts PlotTLODR();

// Defined in resources.cpp
extern Anaquin::Scripts PlotTROC();

// Defined in resources.cpp
extern Anaquin::Scripts PlotTMA();

// Defined in resources.cpp
extern Anaquin::FileName LadRef();

// Defined in resources.cpp
extern Anaquin::FileName GTFRef();

namespace Anaquin
{
    struct RFold : public Analyzer
    {
        enum class Metrics
        {
            Gene,
            Isoform,
        };

        enum class Format
        {
            DESeq2,
            edgeR,
            Sleuth,
            Cuffdiff,
            Anaquin            
        };
        
        struct Options : public AnalyzerOptions
        {
            Options() {}
            Format format;
            Metrics metrs;
        };

        struct Stats : public SequinStats, public MappingStats, public AnalyzerStats, public HistStats
        {
            struct Data
            {
                // Expcted log-fold ratio
                Concent exp;
                
                // Measured log-fold ratio
                Concent obs;
                
                Measured samp1 = NAN;
                Measured samp2 = NAN;
                
                // Standard deviation
                double se = NAN;
                
                // Base mean
                double mean = NAN;
                
                Probability p = NAN;
                Probability q = NAN;
            };
            
            std::map<SequinID, Data> data;
            
            /*
             * Optional inputs
             */
            
            // Normalized average counts for the replicates
            std::vector<double> means;
            
            // Log-fold ratios standard deviation
            std::vector<double> ses;
        };

        /*
         * Classify and construct a vector of TP/FP/TN/FN, given the q-values and expected
         * fold-changes. The threshold for the TP classification is also required.
         */
        
        static std::vector<std::string> classify(const std::vector<double> &,
                                                 const std::vector<double> &,
                                                 double qCut,
                                                 double foldCut);

        static Stats analyze(const FileName &, const Options &o);
        static void  report (const FileName &, const Options &o = Options());
    };
}

#endif

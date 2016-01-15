/*
 * Copyright (C) 2016 - Garvan Institute of Medical Research
 *
 *  Ted Wong, Bioinformatic Software Engineer at Garvan Institute.
 */

#ifndef T_DIFFS_HPP
#define T_DIFFS_HPP

#include "stats/analyzer.hpp"

namespace Anaquin
{
    struct TDiffs : public Analyzer
    {
        enum class Software
        {
            Cuffdiff,
            edgeR,
            DESeq2,
        };
        
        enum class Metrics
        {
            Gene,
            Isoform,
            Exon
        };

        struct Options : public DoubleMixtureOptions
        {
            Options() {}

            Metrics  metrs = Metrics::Gene;
            Software soft  = Software::Cuffdiff;
        };

        struct Stats : public MappingStats
        {
            struct Data : public LinearStats
            {
                // Detected sequins
                std::vector<GenericID> seqs;
                
                // Raw probabilities
                std::vector<double> ps;
                
                // Q-probability (controlling multiple testing)
                std::vector<double> qs;
                
                // Raw probabilities
                std::vector<double> logFCs;
            };
            
            std::map<ChromoID, Data> data;

            Limit limit;
        };

        /*
         * Classify and construct a vector of TP/FP/TN/FN, given a vector of q-values and expected fold-changes. The threshold
         * for the TP classification is also required.
         */
        
        static std::vector<std::string> classify(const std::vector<double> &, const std::vector<double> &, double qCut, double foldCut);
        
        static Stats analyze(const FileName &, const Options &o);
        static Stats analyze(const std::vector<DiffTest> &, const Options &o);

        static void report(const FileName &, const Options &o = Options());
    };
}

#endif

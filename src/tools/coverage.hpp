#ifndef COVERAGE_TOOL_HPP
#define COVERAGE_TOOL_HPP

#include <vector>
#include "data/types.hpp"
#include "parsers/parser.hpp"
#include "stats/analyzer.hpp"
#include "data/intervals.hpp"
#include "data/alignment.hpp"

namespace Anaquin
{
    struct CoverageTool
    {
        struct Stats : public AlignmentStats
        {
            Intervals inters;
        };

        // Whether to proceed with the alignment
        typedef std::function<bool (const Alignment &, const ParserProgress &)> AlignFunctor;

        // Whether to proceed with the coverage
        typedef std::function<bool (const ChromoID &, Base, Base, Coverage)> CoverageFunctor;

        struct CoverageReportOptions
        {
            // Filename for the summary statistics
            FileName summary;
            
            // Filename for the generated bedgraph
            FileName bedGraph;

            // Number of sequins
            Counts refs;
            
            // Size of all sequins
            Base length;
            
            // Where the data should be written
            std::shared_ptr<Writer> writer;
        };
        
        // Analyze a BAM file sorted by position
        static Stats stats(const FileName &, AlignFunctor);

        // Report a BAM file sorted by position
        static void report(const Stats &, const CoverageReportOptions &, CoverageFunctor);
    };
}

#endif
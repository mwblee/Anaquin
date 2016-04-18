#include "TransQuin/t_coverage.hpp"

using namespace Anaquin;

TCoverage::Stats TCoverage::stats(const FileName &file, const Options &o)
{
    o.analyze(file);
    
    const auto &r = Standard::instance().r_trans;
    
    return CoverageTool::stats_(file, r.geneHist(ChrT), [&](const Alignment &align, const ParserProgress &)
    {
        if (align.cID == ChrT)
        {
            return r.match(align.l, MatchRule::Contains);
        }
        
        return (const TransData *) nullptr;
    });
}

void TCoverage::report(const FileName &file, const TCoverage::Options &o)
{
    const auto &r    = Standard::instance().r_trans;
    const auto stats = TCoverage::stats(file, o);

    CoverageTool::CoverageBedGraphOptions bo;

    /*
     * Generating bedgraph for the standards
     */
    
    bo.writer = o.writer;
    bo.file   = "TransCoverage_chrT.bedgraph";

    CoverageTool::bedGraph(stats, bo, [&](const ChrID &id, Base i, Base j, Coverage)
    {
        // Filter to the regions in the standards
        return r.findExon(ChrT, Locus(i, j), MatchRule::Contains);
    });

    /*
     * Generating summary statistics
     */
    
    o.info("Generating TransCoverage_summary.stats");
    
    CoverageTool::CoverageReportOptions to;
    
    to.writer   = o.writer;
    to.summary  = "TransCoverage_summary.stats";
    to.refs     = r.hist().size();
    to.length   = r.size();

    CoverageTool::summary(stats, to, [&](const ChrID &id, Base i, Base j, Coverage)
    {
        // Filter to the regions in the standards
        return r.match(Locus(i, j), MatchRule::Contains);
    });
}
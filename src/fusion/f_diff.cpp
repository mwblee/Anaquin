#include "fusion/f_diff.hpp"
#include "fusion/f_discover.hpp"
#include "fusion/f_classify.hpp"
#include "parsers/parser_stab.hpp"
#include "parsers/parser_star_fusion.hpp"

using namespace Anaquin;

FDiff::Stats FDiff::stats(const FileName &chim, const FileName &splice, const Options &o)
{
    FDiff::Stats stats;
    const auto &r = Standard::instance().r_fus;

    // Measured abundance for the normal genes
    std::map<SequinID, Counts> normals;
    
    // Measured abundance for the fusion genes
    std::map<SequinID, Counts> fusions;
    
    /*
     * Parse the normal junctions
     */
    
    ParserSTab::parse(Reader(splice), [&](const ParserSTab::Chimeric &c, const ParserProgress &)
    {
        if (c.id == Standard::chrT) { stats.n_chrT++; }
        else                        { stats.n_expT++; }

        const SequinData *match;

        if (c.id == Standard::chrT && (match = r.findSplice(c.l)))
        {
            normals[match->id] = c.unique;
            stats.h.at(match->id)++;
        }
    });

    /*
     * Parse the chimeric junctions
     */
    
    ParserStarFusion::parse(Reader(chim), [&](const ParserStarFusion::Fusion &f, const ParserProgress &)
    {
        const auto r = FClassify::classifyFusion(f, o);

        switch (r.code)
        {
            case FClassify::Code::Genome:
            case FClassify::Code::GenomeChrT: { stats.n_expT++; }
            case FClassify::Code::Positive:
            case FClassify::Code::Negative:   { stats.n_chrT++; }
        }
        
        if (r.code == FClassify::Code::Positive)
        {
            fusions[r.match->id] = f.reads;
            stats.h.at(r.match->id)++;
        }
    });

    o.info("Found " + std::to_string(normals.size()) + " introns");
    o.info("Found " + std::to_string(fusions.size()) + " fusions");
    
    /*
     * Compare the genes that are detected in both conditions.
     */
    
    for (const auto &i : normals)
    {
        for (const auto &j : fusions)
        {
            if (r.normalToFusion(i.first) == j.first)
            {
                // Either the normal ID or fusion ID can be used
                const auto expected = r.findSpliceChim(i.first);
                
                // Measured fold change between normal and fusion gene
                const auto measured = static_cast<double>(i.second) / j.second;
                
                stats.add(i.first + " - " + j.first, expected->fold(), measured);
            }
        }
    }

    stats.ss = Standard::instance().r_fus.limit(stats.h);

    return stats;
}

void FDiff::report(const FileName &splice, const FileName &chim, const Options &o)
{
    const auto stats = FDiff::stats(splice, chim, o);

    /*
     * Generating summary statistics
     */

    o.info("Generating summary statistics");
    AnalyzeReporter::linear("FusionDiff_summary.stats", splice + " & " + chim, stats, "fusions", o.writer);

    /*
     * Generating Bioconductor
     */
    
    o.info("Generating Bioconductor");
    AnalyzeReporter::scatter(stats, "", "FusionDiff", "Expected Fold", "Measured Fold", "Expected log2 fold change of mixture A and B", "Expected log2 fold change of mixture A and B", o.writer);
}
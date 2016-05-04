#include "data/pachter.hpp"
#include "VarQuin/v_kallele.hpp"
#include "parsers/parser_kallisto.hpp"

using namespace Anaquin;

// Defined in resources.cpp
extern Scripts PlotVAllele();

VKAllele::Stats VKAllele::analyze(const FileName &file1, const FileName &file2, const Options &o)
{
    // Run quantification in Kallisto
    const auto abundFile = Pachter::externalQuant(o.index, file1, file2);

    VKAllele::Stats stats = VAllele::analyze(abundFile);
    
    return stats;
}

void VKAllele::report(const FileName &file1, const FileName &file2, const Options &o)
{
    const auto &stats = analyze(file1, file2, o);
    
    o.info("Generating statistics");
    
    /*
     * Generating summary statistics
     */

    o.info("Generating VarKAllele_summary.stats");
    o.writer->open("VarKAllele_summary.stats");
    o.writer->write(StatsWriter::inflectSummary(o.rChrT,
                                                o.rGeno,
                                                (file1 + " & " + file2),
                                                stats.hist,
                                                stats,
                                                stats.all,
                                                "sequins"));
    o.writer->close();

    /*
     * Generating CSV for all sequins
     */

    o.info("Generating VarKAllele_quins.csv");
    o.writer->open("VarKAllele_quins.csv");
    o.writer->write(StatsWriter::writeCSV(stats.all, "expected", "measured"));
    o.writer->close();
    
    /*
     * Generating for allele vs allele
     */

    o.info("Generating VarKAllele_allele.R");
    o.writer->open("VarKAllele_allele.R");
    o.writer->write(RWriter::createScript("VarKAllele_quins.csv", PlotVAllele()));
    o.writer->close();
}
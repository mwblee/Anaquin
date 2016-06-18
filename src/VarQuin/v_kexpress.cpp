/*
 * Copyright (C) 2016 - Garvan Institute of Medical Research
 *
 *  Ted Wong, Bioinformatic Software Engineer at Garvan Institute.
 */

#include "data/pachter.hpp"
#include "VarQuin/VarQuin.hpp"
#include "VarQuin/v_kexpress.hpp"
#include "parsers/parser_kallisto.hpp"

using namespace Anaquin;

// Defined in resources.cpp
extern Scripts PlotScatter();

// Defined by Kallisto
extern int __main__(int argc, char *argv[]);

VKExpress::Stats VKExpress::analyze(const FileName &file1, const FileName &file2, const Options &o)
{
    const auto &r = Standard::instance().r_var;

    VKExpress::Stats stats;
    
    // Initialize the distribution for each sequin
    stats.hist = r.hist();
    
    stats.n_gen = NAN;

    // Run quantification in Kallisto
    const auto abundFile = Pachter::externalQuant(o.index, file1, file2);

    /*
     * Parsing the generated files. We're interested in the file listing the abundance.
     */
    
    ParserKallisto::parse(Reader(abundFile), [&](const ParserKallisto::Data &d, const ParserProgress &)
    {
        const auto m = r.match(d.id);
        
        if (m)
        {
            // Expected abundance
            const auto known = m->mixes.at(Mix_1);
            
            // Measured abundance
            const auto measured = d.abund;
            
            stats.add(d.id, known, measured);
            
            stats.n_syn++;
            stats.hist.at(d.id)++;            
        }
    });
    
    return stats;
}

void VKExpress::report(const FileName &file1, const FileName &file2, const Options &o)
{
    const auto &stats = analyze(file1, file2, o);

    o.info("Generating statistics");
    
    /*
     * Generating summary statistics
     */

    o.info("Generating VarKExpress_summary.stats");
    o.writer->open("VarKExpress_summary.stats");
    o.writer->write(StatsWriter::inflectSummary(o.rAnnot,
                                                o.rAnnot,
                                                (file1 + " & " + file2),
                                                stats.hist,
                                                stats,
                                                stats,
                                                "sequins"));
    o.writer->close();
    
    /*
     * Generating CSV for all sequins
     */
    
    o.info("Generating VarKExpress_sequins.csv");
    o.writer->open("VarKExpress_sequins.csv");
    o.writer->write(StatsWriter::writeCSV(stats));
    o.writer->close();

    /*
     * Generating for expression vs expression
     */

    o.info("Generating VarKExpress_express.R");
    o.writer->open("VarKExpress_express.R");
    o.writer->write(RWriter::createScript("VarKExpress_sequins.csv", PlotScatter()));
    o.writer->close();
}
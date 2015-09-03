#include "variant/v_allele.hpp"
#include "parsers/parser_vcf.hpp"

using namespace Anaquin;

static double alleleFreq(const BaseSeq &m)
{
    assert(m.sequins.size() == 2);
    
    const auto ref = m.sequins.begin()->first;
    const auto var = m.sequins.rbegin()->first;
    
    // Abundance for the reference
    const auto r = m.sequins.at(ref).abund();
    
    // Abundance for the variant
    const auto v = m.sequins.at(var).abund();
    
    // Abundance ratio of reference to variant DNA standard
    return v / (r + v);
}

VAllele::Stats VAllele::analyze(const std::string &file, const Options &options)
{
    VAllele::Stats stats;
    const auto &s = Standard::instance();

    options.info("Parsing VCF file");

    ParserVCF::parse(file, [&](const VCFVariant &var, const ParserProgress &)
    {
        Variation match;

        if (classify(stats.m, var, [&](const VCFVariant &)
        {
            // Can we find this variant?
            if (!s.v_vars.count(var.l))
            {
                return Negative;
            }

            match = s.v_vars.at(var.l);

            // Does the variant match with the meta?
            if (match.type != var.type || match.alt != var.alt || match.ref != var.ref)
            {
                return Negative;
            }

            assert(s.bases_1.count(match.id));
            
            const auto &base = s.bases_1.at(match.id);
            
            /*
             * Plotting the relative allele frequency that is established by differences
             * in the concentration of reference and variant DNA standards.
             */
            
            // The measured coverage is the number of base calls aligned and used in variant calling
            const auto measured = (double) var.dp_a / (var.dp_r + var.dp_a);

            // The known coverage for allele frequnece
            const auto known = alleleFreq(base);

            stats.add(match.id, known, measured);
  
            return Positive;
        }))
        {
            stats.h.at(match)++;
        }
    });

    options.info("Generating statistics");

    /*
     * Calculate the proportion of genetic variation with alignment coverage
     */
    
    // Measure of variant detection independent to sequencing depth or coverage
    stats.efficiency = stats.m.sn() / stats.covered;
    
    // Create a script for allele frequency
    AnalyzeReporter::linear(stats, "VarDiscover_allele", "Allele Frequence", options.writer);

    {
        /*
         * Generate summary statistics
         */
        
        const std::string format = "%1%\t%2%\t%3%";
        
        options.writer->open("VarAllele_summary.stats");
        options.writer->write((boost::format(format) % "sn" % "sp" % "detect").str());
        options.writer->write((boost::format(format) % stats.m.sn()
                                                     % stats.m.sp()
                                                     % stats.covered).str());
        
        for (const auto &p : stats.h)
        {
            options.writer->write((boost::format("%1%-%2%\t%3%") % p.first.id % p.first.l.start % p.second).str());
        }
        
        options.writer->close();
    }

    return stats;
}
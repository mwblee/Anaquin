#include "VarQuin/v_discover.hpp"

using namespace Anaquin;

// Defined in resources.cpp
extern Scripts PlotLODR_V();

// Defined in resources.cpp
extern Scripts PlotROC_V();

VDiscover::Stats VDiscover::analyze(const FileName &file, const Options &o)
{
    const auto &r = Standard::instance().r_var;

    VDiscover::Stats stats;

    // Initialize the distribution for each sequin
    stats.hist = r.hist();

    auto &chrT = stats.chrT;

    parseVariant(file, o.caller, [&](const VariantMatch &m)
    {
        if (m.query.cID == ChrT)
        {
            stats.n_chrT++;
            
            const auto p = isnan(m.query.p) ? 0.0 : m.query.p;
            
            if (m.match && m.ref && m.alt)
            {
                stats.hist.at(m.match->id)++;
                
                if (p <= o.sign)
                {
                    chrT.tps.push_back(m);
                }
                else
                {
                    chrT.fns.push_back(m);
                }
            }
            else
            {
                if (!m.seq)
                {
                    o.warn("Variant [" + std::to_string(m.query.l.start) + "] not aligned with any of the sequins. Ignored.");
                }
                else
                {
                    if (p <= o.sign)
                    {
                        chrT.fps.push_back(m);
                    }
                    else
                    {
                        chrT.tns.push_back(m);
                    }
                }
            }
        }
        else
        {
            stats.n_endo++;
            stats.endo.push_back(m.query);
        }
    });

    for (const auto &i : chrT.tps)
    {
        chrT.m.tp()++;
        
        switch (i.query.type())
        {
            case Mutation::SNP:       { chrT.m_snp.tp()++; break; }
            case Mutation::Deletion:
            case Mutation::Insertion: { chrT.m_ind.tp()++; break; }
        }
    }

    for (const auto &i : chrT.fps)
    {
        chrT.m.fp()++;
        
        switch (i.query.type())
        {
            case Mutation::SNP:       { chrT.m_snp.fp()++; break; }
            case Mutation::Deletion:
            case Mutation::Insertion: { chrT.m_ind.fp()++; break; }
        }
    }

    for (const auto &i : chrT.tns)
    {
        chrT.m.tn()++;

        switch (i.query.type())
        {
            case Mutation::SNP:       { chrT.m_snp.tn()++; break; }
            case Mutation::Deletion:
            case Mutation::Insertion: { chrT.m_ind.tn()++; break; }
        }
    }

    for (const auto &i : chrT.fns)
    {
        chrT.m.fn()++;
        
        switch (i.query.type())
        {
            case Mutation::SNP:       { chrT.m_snp.fn()++; break; }
            case Mutation::Deletion:
            case Mutation::Insertion: { chrT.m_ind.fn()++; break; }
        }
    }
    
    chrT.m_snp.nq() = chrT.dSNP();
    chrT.m_snp.nr() = r.countSNPs();

    chrT.m_ind.nq() = chrT.dInd();
    chrT.m_ind.nr() = r.countIndels();

    chrT.m.nq() = stats.chrT.m_snp.nq() + stats.chrT.m_ind.nq();
    chrT.m.nr() = stats.chrT.m_snp.nr() + stats.chrT.m_ind.nr();
    
    return stats;
}

static void writeClass(const FileName &file,
                       const VDiscover::Stats::ChrTStats &stats,
                       const VDiscover::Options &o)
{
    const std::string format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%\t%7%\t%8%\t%9%";
    
    o.writer->open(file);
    o.writer->write((boost::format(format) % "Sequin"
                                           % "Position"
                                           % "Label"
                                           % "PValue"
                                           % "RefRead"
                                           % "VarRead"
                                           % "ERatio"
                                           % "EAlleleF"
                                           % "Type").str());

    auto f = [&](const std::vector<VDiscover::Stats::ChrTData> &x, const std::string &label)
    {
        for (const auto &i : x)
        {
            std::string type;
            
            switch (i.query.type())
            {
                case Mutation::SNP:       { type = "SNP";   break; }
                case Mutation::Deletion:
                case Mutation::Insertion: { type = "Indel"; break; }
            }
            
            o.writer->write((boost::format(format) % i.seq->id
                                                   % i.query.l.start
                                                   % label
                                                   % (isnan(i.query.p) ? "-" : std::to_string(i.query.p))
                                                   % i.query.readR
                                                   % i.query.readV
                                                   % i.eFold
                                                   % i.eAllFreq
                                                   % type).str());
        }
    };

    f(stats.tps, "TP");
    f(stats.fps, "FP");
    f(stats.tns, "TN");
    f(stats.fns, "FN");

    o.writer->close();
}

static void writeSummary(const FileName &file, const FileName &src, const VDiscover::Stats &stats, const VDiscover::Options &o)
{
    const auto &r = Standard::instance().r_var;
    
    const auto summary = "Summary for input: %1%\n\n"
                         "   ***\n"
                         "   *** Number of variants called in the synthetic and experiment\n"
                         "   ***\n\n"
                         "   Synthetic:  %2% variants\n"
                         "   Experiment: %3% variants\n\n"
                         "   ***\n"
                         "   *** Reference annotation (Synthetic)\n"
                         "   ***\n\n"
                         "   File: %4%\n\n"
                         "   Synthetic:  %5% SNPs\n"
                         "   Synthetic:  %6% indels\n"
                         "   Synthetic:  %7% variants\n\n"
                         "   ************************************************************\n"
                         "   ***                                                      ***\n"
                         "   ***        Statistics for the synthetic discovery        ***\n"
                         "   ***                                                      ***\n"
                         "   ************************************************************\n\n"
                         "   Detected:    %8% SNPs\n"
                         "   Detected:    %9% indels\n"
                         "   Detected:    %10% variants\n\n"
                         "   Signficiance: %11%\n\n"
                         "   Significant: %12% SNPs\n"
                         "   Significant: %13% indels\n"
                         "   Significant: %14% variants\n\n"
                         "   True Positives:   %15% SNPS\n"
                         "   True Positives:   %16% indels\n"
                         "   True Positives:   %17% variants\n\n"
                         "   False Positives:  %18% SNPS\n"
                         "   False Positives:  %19% SNPS\n"
                         "   False Positives:  %20% variants\n\n"
                         "   ***\n"
                         "   *** Performance metrics (Overall)\n"
                         "   ***\n\n"
                         "   Sensitivity: %21$.2f\n"
                         "   Specificity: %22$.2f\n"
                         "   Precision:   %23$.2f\n\n"
                         "   ***\n"
                         "   *** Performance metrics (SNP)\n"
                         "   ***\n\n"
                         "   Sensitivity: %24$.2f\n"
                         "   Specificity: %25$.2f\n"
                         "   Precision:   %26$.2f\n\n"
                         "   ***\n"
                         "   *** Performance metrics (Indel)\n"
                         "   ***\n\n"
                         "   Sensitivity: %27$.2f\n"
                         "   Specificity: %28$.2f\n"
                         "   Precision:   %29$.2f\n\n";

    o.writer->open("VarDiscover_summary.stats");    
    o.writer->write((boost::format(summary) % src
                                            % stats.chrT.dTot()
                                            % stats.endo.size()
                                            % o.rChrT
                                            % r.countSNPs()
                                            % r.countIndels()
                                            % r.countVars()           // 7
                                            % stats.chrT.dSNP()
                                            % stats.chrT.dInd()
                                            % stats.chrT.dTot()
                                            % o.sign                  // 11
                                            % stats.chrT.sSNP()
                                            % stats.chrT.sInd()
                                            % stats.chrT.sTot()
                                            % stats.chrT.tpSNP()
                                            % stats.chrT.tpInd()
                                            % stats.chrT.tpTot()
                                            % stats.chrT.fpSNP()
                                            % stats.chrT.fpInd()
                                            % stats.chrT.fpTot()
                                            % stats.chrT.m.sn()       // 21
                                            % stats.chrT.m.sp()       // 22
                                            % stats.chrT.m.pc()       // 23
                                            % stats.chrT.m_snp.sn()   // 24
                                            % stats.chrT.m_snp.sp()   // 25
                                            % stats.chrT.m_snp.pc()   // 26
                                            % stats.chrT.m_ind.sn()   // 27
                                            % stats.chrT.m_ind.sp()   // 28
                                            % stats.chrT.m_ind.pc()   // 29
                     ).str());
    o.writer->close();
}

void VDiscover::report(const FileName &file, const Options &o)
{
    o.logInfo("Significance level: " + std::to_string(o.sign));

    const auto stats = analyze(file, o);

    o.logInfo("Number of true positives:  " + std::to_string(stats.chrT.tps.size()));
    o.logInfo("Number of false positives: " + std::to_string(stats.chrT.fps.size()));
    o.logInfo("Number of true negatives: "  + std::to_string(stats.chrT.tns.size()));
    o.logInfo("Number of false negaitves: " + std::to_string(stats.chrT.fns.size()));

    o.info("Generating statistics");

    /*
     * Generate summary statistics
     */
    
    writeSummary("VarDiscover_summary.stats", file, stats, o);

    /*
     * Generating classified statistics for the variants
     */
    
    writeClass("VarDiscover_quins.csv", stats.chrT, o);

    /*
     * Generating ROC curve
     */
    
    o.info("Generating VarDiscover_ROC.R");
    o.writer->open("VarDiscover_ROC.R");
    o.writer->write(RWriter::createScript("VarDiscover_quins.csv", PlotROC_V()));
    o.writer->close();

    /*
     * Generating probability curve (only if probability is given, for instance, not possible with GATK)
     */

    if (o.caller == Caller::VarScan)
    {
        o.info("Generating VarDiscover_Prob.R");
        o.writer->open("VarDiscover_Prob.R");
        o.writer->write(RWriter::createScript("VarDiscover_quins.csv", PlotLODR_V()));
        o.writer->close();
    }
}
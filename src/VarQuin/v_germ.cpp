#include "VarQuin/v_germ.hpp"
#include "writers/vcf_writer.hpp"
#include "parsers/parser_vcf.hpp"
#include "writers/json_writer.hpp"

using namespace Anaquin;

typedef VGerm::SStats SStats;
typedef VGerm::Options Options;
typedef SequinVariant::Context Context;

inline std::string ctx2Str(Context x)
{
    switch (x)
    {
        case Context::Cancer:        { return "Cancer";                    }
        case Context::LowGC:         { return "LowGC";                     }
        case Context::HighGC:        { return "HighGC";                    }
        case Context::Common:        { return "Common";                    }
        case Context::VeryLowGC:     { return "VeryLowGC";                 }
        case Context::VeryHighGC:    { return "VeryHighGC";                }
        case Context::LongHompo:     { return "LongHomopolymer";           }
        case Context::ShortHompo:    { return "ShortHomopolymer";          }
        case Context::ShortDinRep:   { return "ShortDinucleotideRepeat";   }
        case Context::LongDinRep:    { return "LongDinucleotideRepeat";    }
        case Context::ShortQuadRep:  { return "ShortQuadNucleotideRepeat"; }
        case Context::LongQuadRep:   { return "LongQuadNucleotideRepeat";  }
        case Context::ShortTrinRep:  { return "ShortTrinucleotideRepeat";  }
        case Context::LongTrinRep:   { return "LongTrinucleotideRepeat";   }
    }
}

static Scripts createROC(const FileName &f1, const FileName &f2)
{
    extern Scripts PlotVGROC();
    extern Path __output__;
    extern std::string __full_command__;
    
    return (boost::format(PlotVGROC()) % date()
                                       % __full_command__
                                       % __output__
                                       % f1
                                       % f2
                                       % "Qual"
                                       % "'Mutation'"
                                       % "'NULL'").str();
}

EStats VGerm::analyzeE(const FileName &file, const Options &o)
{
    const auto &r = Standard::instance().r_var;
    const auto r2 = Standard::instance().r_var.regs2();
    
    EStats stats;

    stats.g2c[Genotype::Homozygous];
    stats.g2c[Genotype::Heterzygous];
    stats.v2c[Variation::SNP];
    stats.v2c[Variation::Deletion];
    stats.v2c[Variation::Inversion];
    stats.v2c[Variation::Insertion];
    stats.v2c[Variation::Duplication];
    
    if (!file.empty())
    {
        auto w = VCFWriter();
        w.open(o.work + "/VarMutation_sample.vcf");
        
        ParserVCF::parse(file, [&](const Variant &x)
        {
            DInter *d = nullptr;

            if (o.filter == VCFFilter::Passed && x.filter != Filter::Pass)
            {
                return;
            }
            else if ((d = contains(r2, x.cID, x.l)))
            {
                w.write(x.hdr, x.line);

                auto t = x;
                t.name = d->name();
                stats.g2c[t.gt]++;
                stats.v2c[t.type()]++;
                stats.vs.insert(t);
                
                /*
                 * Can we extract information for this variant from sequin regions?
                 */

                const auto sv = r.findVar(d->name());
                A_ASSERT(sv);
                
                if (sv->isGCLow() || sv->isGCHigh())
                {
                    stats.gc++;
                }
                else if (sv->isRepeat())
                {
                    stats.rep++;
                }
            }
        });

        w.close();
    }

    return stats;
}

VGerm::SStats VGerm::analyzeS(const FileName &file, const Options &o)
{
    const auto &r = Standard::instance().r_var;
    const auto r2 = Standard::instance().r_var.regs2();

    VGerm::SStats stats;
    
    typedef SequinVariant::Context Context;
    
    auto gts = std::set<Genotype>
    {
        Genotype::Homozygous,
        Genotype::Heterzygous
    };

    auto muts = std::set<Variation>
    {
        Variation::SNP,
        Variation::Deletion,
        Variation::Insertion,
    };
    
    auto ctx = std::set<Context>
    {
        Context::LowGC,
        Context::HighGC,
        Context::Common,
        Context::LongHompo,
        Context::VeryLowGC,
        Context::VeryHighGC,
        Context::ShortDinRep,
        Context::LongDinRep,
        Context::ShortHompo,
        Context::LongQuadRep,
        Context::LongTrinRep,
        Context::ShortQuadRep,
        Context::ShortTrinRep,
    };

    for (auto &i : gts)  { stats.g2c[i]; }
    for (auto &i : ctx)  { stats.c2c[i]; }
    for (auto &i : muts) { stats.v2c[i]; }
    
    o.analyze(file);

    auto wTP = VCFWriter(); wTP.open(o.work + "/VarMutation_TP.vcf");
    auto wFP = VCFWriter(); wFP.open(o.work + "/VarMutation_FP.vcf");
    
    ParserVCF::parse(file, [&](const Variant &x)
    {
        if (o.filter == VCFFilter::Passed && x.filter != Filter::Pass)
        {
            return;
        }
        else if (!contains(r2, x.cID, x.l))
        {
            return;
        }
        
        auto findMatch = [&](const Variant &query)
        {
            Match m;

            m.qry = query;
            m.var = nullptr;

            // Can we match by position?
            if ((m.var = r.findV1(query.cID, query.l)))
            {
                // Match by reference allele?
                m.ref = m.var->ref == query.ref;
                
                // Match by alternative allele?
                m.alt = m.var->alt == query.alt;
            }
            
            if (m.var)
            {
                m.rID = m.var->name;
                A_ASSERT(!m.rID.empty());
            }
            else
            {
                MergedIntervals<> inters;
                
                try
                {
                    // We should to search where the FPs are
                    inters = r.mInters(x.cID);
                    
                    A_ASSERT(inters.size());
                    
                    const auto m2 = inters.contains(x.l);
                    
                    // Can we find the corresponding region for the FP?
                    if (m2)
                    {
                        m.rID = m2->id();
                        A_ASSERT(!m.rID.empty());
                    }
                }
                catch (...) {}
            }
            
            return m;
        };
        
        const auto m = findMatch(x);
        
        // Matched if the position and alleles agree
        const auto matched = m.var && m.ref && m.alt;
        
        if (matched)
        {
            if (isGerm(m.var->name))
            {
                wTP.write(x.hdr, x.line);
                
                const auto key = m.var->key();
                
                stats.tps.push_back(m);
                
                const auto exp = r.af(m.var->name);
                const auto obs = m.qry.allF;
                
                A_ASSERT(!isnan(exp));
                
                // Eg: 2821292107
                const auto x = toString(key);
                
                // Add for all variants
                stats.oa.add(x, exp, obs);
                
                // Add for mutation type
                stats.m2a[m.qry.type()].add(x, exp, obs);
            }
        }
        else
        {
            // Ignore anything that is somatic
            if (!r.findV2(x.cID, x.l))
            {
                wFP.write(x.hdr, x.line);
                
                // FP because the variant is not found in the reference
                stats.fps.push_back(m);
            }
        }
    });
    
    wTP.close();
    wFP.close();
    
    /*
     * Determine the quantification limits
     */
    
    stats.oa.limit = stats.oa.limitQuant();

    for (auto &i : stats.m2a)
    {
        i.second.limit = i.second.limitQuant();
    }

    /*
     * Determine the classification performance
     */

    auto forTP = [&]()
    {
        for (auto &i : stats.tps)
        {
            // This shouldn't fail...
            const auto &sv = r.findSeqVar1(i.var->key());
            
            // Overall performance
            stats.oc.tp()++;
            
            // Performance by genotype
            stats.g2c[sv.gt].tp()++;
            
            // Performance by GC
            if (sv.isGCLow() || sv.isGCHigh())
            {
                stats.gc2c.tp()++;
            }
            
            // Performance by repeats
            if (sv.isRepeat())
            {
                stats.r2c.tp()++;
            }
            
            // Performance by GC context
            stats.c2c[sv.ctx].tp()++;
            
            // Performance by variation
            stats.v2c[i.qry.type()].tp()++;
        }
    };
    
    auto forFP = [&]()
    {
        for (auto &i : stats.fps)
        {
            // Overall performance
            stats.oc.fp()++;
            
            // Performance by mutation
            stats.v2c[i.qry.type()].fp()++;
            
            // Performance by genotype
            stats.g2c[i.qry.gt].fp()++;
        }
    };

    forTP();
    forFP();

    for (auto &mut : muts)
    {
        stats.v2c[mut].nr() = r.nType1(mut);
        stats.v2c[mut].nq() = stats.v2c[mut].tp() + stats.v2c[mut].fp();
        stats.v2c[mut].fn() = stats.v2c[mut].nr() - stats.v2c[mut].tp();
        stats.oc.nr() += r.nType1(mut);
    }

    stats.oc.fn() = stats.oc.nr() - stats.oc.tp();
    
    /*
     * Performance by context
     */
    
    for (auto &i : ctx)
    {
        stats.c2c[i].nr() = r.nCtx1(i);
        stats.c2c[i].nq() = stats.c2c[i].tp() + stats.c2c[i].fp();
        stats.c2c[i].fn() = stats.c2c[i].nr() - stats.c2c[i].tp();
    }
    
    /*
     * Performance by genotype
     */
    
    for (auto &i : gts)
    {
        stats.g2c[i].nr() = r.nGeno1(i);
        stats.g2c[i].nq() = stats.g2c[i].tp() + stats.g2c[i].fp();
        stats.g2c[i].fn() = stats.g2c[i].nr() - stats.g2c[i].tp();
    }
    
    /*
     * Performance by repeats
     */
    
    stats.r2c.nr() = r.nRep();
    stats.r2c.fn() = stats.r2c.nr() - stats.r2c.tp();

    /*
     * Performance by GC contents
     */
    
    stats.gc2c.nr() = r.nGCHigh() + r.nGCLow();
    stats.gc2c.fn() = stats.gc2c.nr() - stats.gc2c.tp();

    A_ASSERT(stats.oc.nr() >= stats.oc.fn());
 
    for (const auto &i : r.v1())
    {
        if (!stats.findTP(i.name) && isGerm(i.name))
        {
            VGerm::Match m;
            
            m.var = r.findV1(i.cID, i.l);
            m.rID = i.name;
            A_ASSERT(m.var);
            
            stats.fns.push_back(m);
        }
    }
    
    return stats;
}

static void writeQuins(const FileName &file,
                       const VGerm::SStats &ss,
                       const VGerm::Options &o)
{
    const auto &r = Standard::instance().r_var;
    const auto format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%\t%7%\t%8%\t%9%\t%10%\t%11%\t%12%\t%13%";

    o.generate(file);
    o.writer->open(file);
    o.writer->write((boost::format(format) % "Name"
                                           % "Chrom"
                                           % "Position"
                                           % "Label"
                                           % "ReadR"
                                           % "ReadV"
                                           % "Depth"
                                           % "ExpFreq"
                                           % "ObsFreq"
                                           % "Qual"
                                           % "Genotype"
                                           % "Context"
                                           % "Mutation").str());
    for (const auto &i : r.v1())
    {
        if (isGerm(i.name))
        {
            // Can we find this sequin?
            const auto isTP = ss.findTP(i.name);
            
            // This shouldn't fail...
            const auto &sv = r.findSeqVar1(i.key());
            
            if (isTP)
            {
                // Called variant (if found)
                const auto &c = isTP->qry;
                
                o.writer->write((boost::format(format) % i.name
                                 % i.cID
                                 % i.l.start
                                 % "TP"
                                 % c.readR
                                 % c.readV
                                 % c.depth
                                 % r.af(i.name)
                                 % c.allF
                                 % toString(c.qual)
                                 % gt2str(sv.gt)
                                 % ctx2Str(sv.ctx)
                                 % var2str(i.type())).str());
            }
            
            // Failed to detect the variant
            else
            {
                o.writer->write((boost::format(format) % i.name
                                                       % i.cID
                                                       % i.l.start
                                                       % "FN"
                                                       % "-"
                                                       % "-"
                                                       % "-"
                                                       % r.af(i.name)
                                                       % "-"
                                                       % "-"
                                                       % gt2str(sv.gt)
                                                       % ctx2Str(sv.ctx)
                                                       % var2str(i.type())).str());
            }
        }
    }
    
    o.writer->close();
}

static void writeDetected(const FileName &file,
                          const VGerm::SStats &ss,
                          const VGerm::Options &o)
{
    const auto &r = Standard::instance().r_var;
    const auto format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%\t%7%\t%8%\t%9%\t%10%\t%11%\t%12%";
    
    o.generate(file);
    o.writer->open(file);
    o.writer->write((boost::format(format) % "Name"
                                           % "Chrom"
                                           % "Position"
                                           % "Label"
                                           % "ReadR"
                                           % "ReadV"
                                           % "Depth"
                                           % "ExpFreq"
                                           % "ObsFreq"
                                           % "Qual"
                                           % "Context"
                                           % "Mutation").str());

    auto f = [&](const std::vector<VGerm::Match> &x, const std::string &label)
    {
        for (const auto &i : x)
        {
            auto sID = (i.var && i.alt && i.ref ? i.var->name : "-");
            const auto ctx = sID != "-" ?  ctx2Str(r.findSeqVar1(i.var->key()).ctx) : "-";

            o.writer->write((boost::format(format) % (i.rID.empty() ? "-" : i.rID)
                                                   % i.qry.cID
                                                   % i.qry.l.start
                                                   % label
                                                   % i.qry.readR
                                                   % i.qry.readV
                                                   % i.qry.depth
                                                   % (sID != "-" ? std::to_string(r.af(sID)) : "-")
                                                   % i.qry.allF
                                                   % toString(i.qry.qual)
                                                   % ctx
                                                   % var2str(i.qry.type())).str());
        }
    };
    
    f(ss.tps, "TP");
    f(ss.fps, "FP");

    o.writer->close();
}

static std::map<std::string, std::string> jsonD(const FileName &endo,
                                                const FileName &seqs,
                                                const EStats &es,
                                                const SStats &ss,
                                                const Options &o)
{
    const auto &r = Standard::instance().r_var;

    extern bool VCFFromUser();
    extern bool RBEDFromUser();
    extern bool UBEDFromUser();
    
    extern FileName VCFRef();
    extern FileName Bed1Ref();

    std::map<std::string, std::string> x;
    
    #define D(x) (isnan(x) ? "-" : std::to_string(x))
    #define E3() (endo.empty() ? "-" : std::to_string(es.v2c.at(Variation::SNP) + es.v2c.at(Variation::Insertion) + es.v2c.at(Variation::Deletion)))
    #define CSN(x) D(ss.c2c.at(x).sn())
    
    const auto &snp = ss.v2c.at(Variation::SNP);
    const auto &del = ss.v2c.at(Variation::Deletion);
    const auto &ins = ss.v2c.at(Variation::Insertion);
    const auto &hom = ss.g2c.at(Genotype::Homozygous);
    const auto &het = ss.g2c.at(Genotype::Heterzygous);
    
    auto ind = del;
    ind += ins;

    const auto c_nSNP = snp.nq();
    const auto c_nDel = del.nq();
    const auto c_nIns = ins.nq();

    x["mode"]      = "germline";
    x["nRegs"]     = D(r.nRegs());
    x["lRegs"]     = D(r.lRegs());
    x["rVCF"]      = (VCFFromUser() ? VCFRef()  : "-");
    x["rBED"]      = (RBEDFromUser() ? Bed1Ref() : "-");
    x["uSam"]      = (endo.empty() ? "-" : endo);
    x["uSeq"]      = seqs;
    x["uBed"]      = (UBEDFromUser() ? Bed1Ref() : "-");
    x["nSam"]      = E3(); // Number of sample variants
    x["nSeq"]      = D(c_nSNP + c_nDel + c_nIns);
    x["exclude"]   = o.filter == VCFFilter::Passed ? "True" : "False";    
    x["allN"]      = D(r.nType1(Variation::SNP)       +
                       r.nType1(Variation::Insertion) +
                       r.nType1(Variation::Deletion));
    x["allTP"]     = D(ss.oc.tp());
    x["allFP"]     = D(ss.oc.fp());
    x["allFN"]     = D(ss.oc.fn());
    x["allSN"]     = D(ss.oc.sn());
    x["allPC"]     = D(ss.oc.pc());
    x["allF1"]     = D(ss.oc.F1());
    x["allFDR"]    = D(1-ss.oc.pc());
    x["snpN"]      = D(r.nType1(Variation::SNP));
    x["snpTP"]     = D(snp.tp());
    x["snpFP"]     = D(snp.fp());
    x["snpFN"]     = D(snp.fn());
    x["snpSN"]     = D(snp.sn());
    x["snpPC"]     = D(snp.pc());
    x["snpF1"]     = D(snp.F1());
    x["snpFDR"]    = D(1-snp.pc());
    x["indN"]      = D(r.nType1(Variation::Insertion) + r.nType1(Variation::Deletion));
    x["indTP"]     = D(ind.tp());
    x["indFP"]     = D(ind.fp());
    x["indFN"]     = D(ind.fn());
    x["indSN"]     = D(ind.sn());
    x["indPC"]     = D(ind.pc());
    x["indF1"]     = D(ind.F1());
    x["indFDR"]    = D(1-ind.pc());
    x["GCN"]       = D(ss.gc2c.nr());
    x["GCTP"]      = D(ss.gc2c.tp());
    x["GCFN"]      = D(ss.gc2c.fn());
    x["GCSN"]      = D(ss.gc2c.sn());
    x["RepN"]      = D(ss.r2c.nr());
    x["RepTP"]     = D(ss.r2c.tp());
    x["RepFN"]     = D(ss.r2c.fn());
    x["RepSN"]     = D(ss.r2c.sn());
    x["homN"]      = D(r.nGeno1(Genotype::Homozygous));
    x["homTP"]     = D(hom.tp());
    x["homFP"]     = D(hom.fp());
    x["homFN"]     = D(hom.fn());
    x["homSN"]     = D(hom.sn());
    x["homPC"]     = D(hom.pc());
    x["homF1"]     = D(hom.F1());
    x["homFDR"]    = D(1-hom.pc());
    x["hetN"]      = D(r.nGeno1(Genotype::Heterzygous));
    x["hetTP"]     = D(het.tp());
    x["hetFP"]     = D(het.fp());
    x["hetFN"]     = D(het.fn());
    x["hetSN"]     = D(het.sn());
    x["hetPC"]     = D(het.pc());
    x["hetF1"]     = D(het.F1());
    x["hetFDR"]    = D(1-het.pc());
    x["common"]    = CSN(Context::Common);
    x["lowGC"]     = CSN(Context::LowGC);
    x["highGC"]    = CSN(Context::HighGC);
    x["longHom"]   = CSN(Context::LongHompo);
    x["vLowGC"]    = CSN(Context::VeryLowGC);
    x["vHighGC"]   = CSN(Context::VeryHighGC);
    x["shortDR"]   = CSN(Context::ShortDinRep);
    x["longDR"]    = CSN(Context::LongDinRep);
    x["shortHom"]  = CSN(Context::ShortHompo);
    x["longQRep"]  = CSN(Context::LongQuadRep);
    x["longTRep"]  = CSN(Context::LongTrinRep);
    x["shortQRep"] = CSN(Context::ShortQuadRep);
    x["shortTRep"] = CSN(Context::ShortTrinRep);
    
    /*
     * Endogenous variants
     */
    
    x["samGC"]  = D(es.gc);  // Number of GC Rich/Poor
    x["samRep"] = D(es.rep); // Number of simple repeats
    x["samN"]   = D(es.vs.size());
    x["samSNP"] = D(es.v2c.at(Variation::SNP));
    x["samInd"] = D(es.v2c.at(Variation::Insertion) + es.v2c.at(Variation::Deletion));
    x["samHom"] = D(es.g2c.at(Genotype::Homozygous));
    x["samHet"] = D(es.g2c.at(Genotype::Heterzygous));

    return x;
}

static void writeSummary(const FileName &file,
                         const FileName &endo,
                         const FileName &seqs,
                         const EStats &es,
                         const VGerm::SStats &ss,
                         const VGerm::Options &o)
{
    const auto summary = "-------VarMutation Summary Statistics\n\n"
                         "-------VarMutation Output Results\n\n"
                         "       Reference variant annotation:      %1%\n"
                         "       Reference sequin regions:          %2%\n\n"
                         "       Variants identified in sample:     %3%\n"
                         "       Variants identified in sequins:    %4%\n\n"
                         "       Number of sample variants (sequin regions): %5%\n"
                         "       Number of sequin variants (sequin regions): %6%\n\n"
                         "-------Diagnostic performance by variant\n\n"
                         "      *All variants\n"
                         "       Reference:             %7%\n"
                         "       True Positive:         %8%\n"
                         "       False Positive:        %9%\n"
                         "       False Negative:        %10%\n"
                         "       Sensitivity:           %11%\n"
                         "       Precision:             %12%\n"
                         "       F1 Score:              %13%\n"
                         "       FDR Rate:              %14%\n\n"
                         "      *Single Nucleotide Variants (SNVs)\n\n"
                         "       Reference:             %15%\n"
                         "       True Positive:         %16%\n"
                         "       False Positive:        %17%\n"
                         "       False Negative:        %18%\n"
                         "       Sensitivity:           %19%\n"
                         "       Precision:             %20%\n"
                         "       F1 Score:              %21%\n"
                         "       FDR Rate:              %22%\n\n"
                         "      *Small Insertions/Deletions (InDels)\n"
                         "       Reference:             %23%\n"
                         "       True Positive:         %24%\n"
                         "       False Positive:        %25%\n"
                         "       False Negative:        %26%\n"
                         "       Sensitivity:           %27%\n"
                         "       Precision:             %28%\n"
                         "       F1 Score:              %29%\n"
                         "       FDR Rate:              %30%\n\n"
                         "-------Diagnostic performance by genotype\n\n"
                         "      *Homozygous variants\n"
                         "       Reference:             %31%\n"
                         "       True Positive:         %32%\n"
                         "       False Positive:        %33%\n"
                         "       False Negative:        %34%\n"
                         "       Sensitivity:           %35%\n"
                         "       Precision:             %36%\n"
                         "       F1 Score:              %37%\n"
                         "       FDR Rate:              %38%\n\n"
                         "      *Heterozygous variants\n"
                         "       Reference:             %39%\n"
                         "       True Positive:         %40%\n"
                         "       False Positive:        %41%\n"
                         "       False Negative:        %42%\n"
                         "       Sensitivity:           %43%\n"
                         "       Precision:             %44%\n"
                         "       F1 Score:              %45%\n"
                         "       FDR Rate:              %46%\n\n"
                         "-------Diagnostic performance by context\n\n"
                         "       Context                      Sensitivity:\n"
                         "       Common                       %47%\n"
                         "       Low GC                       %48%\n"
                         "       High GC                      %49%\n"
                         "       Long Homopolymer             %50%\n"
                         "       Very Low GC                  %51%\n"
                         "       Very High GC                 %52%\n"
                         "       Short Dinucleotide Repeat    %53%\n"
                         "       Long Dinucleotide Repeat     %54%\n"
                         "       Short Homopolymer            %55%\n"
                         "       Long Quad Nucleotide Repeat  %56%\n"
                         "       Long Trinucleotide Repeat    %57%\n"
                         "       Short Quad Nucleotide Repeat %58%\n"
                         "       Short Trinucleotide Repeat   %59%\n";

        auto x = jsonD(endo, seqs, es, ss, o);

        o.generate(file);
        o.writer->open(file);
        o.writer->write((boost::format(summary) % x["vRef"]      // 1
                                                % x["bRef"]      // 2
                                                % x["inputE"]    // 3
                                                % x["inputS"]    // 4
                                                % x["nSam"]      // 5
                                                % x["nSeqs"]     // 6
                                                % x["allN"]      // 7
                                                % x["allTP"]     // 8
                                                % x["allFP"]     // 9
                                                % x["allFN"]     // 10
                                                % x["allSN"]     // 11
                                                % x["allPC"]     // 12
                                                % x["allF1"]     // 13
                                                % x["allFDR"]    // 14
                                                % x["snpN"]      // 15
                                                % x["snpTP"]     // 16
                                                % x["snpFP"]     // 17
                                                % x["snpFN"]     // 18
                                                % x["snpSN"]     // 19
                                                % x["snpPC"]     // 20
                                                % x["snpF1"]     // 21
                                                % x["snpFDR"]    // 22
                                                % x["indN"]      // 23
                                                % x["indTP"]     // 24
                                                % x["indFP"]     // 25
                                                % x["indFN"]     // 26
                                                % x["indSN"]     // 27
                                                % x["indPC"]     // 28
                                                % x["indF1"]     // 29
                                                % x["indFDR"]    // 30
                                                % x["homN"]      // 31
                                                % x["homTP"]     // 32
                                                % x["homFP"]     // 33
                                                % x["homFN"]     // 34
                                                % x["homSN"]     // 35
                                                % x["homPC"]     // 36
                                                % x["homF1"]     // 37
                                                % x["homFDR"]    // 38
                                                % x["hetN"]      // 39
                                                % x["hetTP"]     // 40
                                                % x["hetFP"]     // 41
                                                % x["hetFN"]     // 42
                                                % x["hetSN"]     // 43
                                                % x["hetPC"]     // 44
                                                % x["hetF1"]     // 45
                                                % x["hetFDR"]    // 46
                                                % x["common"]    // 47
                                                % x["lowGC"]     // 48
                                                % x["highGC"]    // 49
                                                % x["longHom"]   // 50
                                                % x["vLowGC"]    // 51
                                                % x["vHighGC"]   // 52
                                                % x["shortDR"]   // 53
                                                % x["longDR"]    // 54
                                                % x["shortHom"]  // 55
                                                % x["longQRep"]  // 56
                                                % x["longTRep"]  // 57
                                                % x["shortQRep"] // 58
                                                % x["shortTRep"] // 59
                         ).str());
    o.writer->close();
}

template <typename T, typename O> void writeFN(const FileName &file, const T &x, const O &o, bool useVar)
{
    extern FileName VCFRef();

    auto wFN = VCFWriter(); wFN.open(o.work + "/" + file);

    std::set<long> keys;
    for (const auto &i : x) { keys.insert(i.var->key()); }

    ParserVCF::parse(VCFRef(), [&](const Variant &x)
    {
        if (keys.count(x.key()))
        {
            wFN.write(x.hdr, x.line);
        }
    });
    
    wFN.close();
}

VGerm::Stats VGerm::report(const FileName &endo, const FileName &seqs, const Options &o)
{
    Stats stats;
    
    stats.es = analyzeE(endo, o);
    stats.ss = analyzeS(seqs, o);

    o.info("TP: " + std::to_string(stats.ss.oc.tp()));
    o.info("FP: " + std::to_string(stats.ss.oc.fp()));
    o.info("FN: " + std::to_string(stats.ss.oc.fn()));

    o.info("Generating statistics");

    /*
     * Generating VarMutation_sequins.tsv
     */
    
    writeQuins("VarMutation_sequins.tsv", stats.ss, o);

    /*
     * Generating VarMutation_summary.stats
     */
    
    writeSummary("VarMutation_summary.stats", endo, seqs, stats.es, stats.ss, o);
    
    /*
     * Generating VarMutation_detected.tsv
     */
    
    writeDetected("VarMutation_detected.tsv", stats.ss, o);
    
    /*
     * Generating VarMutation_ROC.R
     */
    
    o.generate("VarMutation_ROC.R");
    o.writer->open("VarMutation_ROC.R");
    o.writer->write(createROC("VarMutation_detected.tsv", "VarMutation_sequins.tsv"));
    o.writer->close();

    /*
     * Generating VarMutation_FN.vcf
     */

    writeFN("VarMutation_FN.vcf", stats.ss.fns, o, true);
    
    /*
     * Generating VarMutation_stats.json
     */
    
    o.generate("VarMutation_stats.json");
    JSONWriter w(o.work);
    w.open("VarMutation_stats.json");
    w.write(jsonD(endo, seqs, stats.es, stats.ss, o));
    w.close();

    return stats;
}

#ifndef T_ALIGN_HPP
#define T_ALIGN_HPP

#include "stats/analyzer.hpp"
#include "data/alignment.hpp"
#include "data/experiment.hpp"

namespace Anaquin
{
    typedef std::map<BinID, Counts> BinCounts;

    class TAlign : public Analyzer
    {
        public:

            typedef AnalyzerOptions Options;

            typedef std::map<GeneID, Base> FPStats;

            struct MergedConfusion
            {
                /*
                 * Alignment statistics
                 */
                
                Counts aTP = 0;
                Counts aFP = 0;
            
                /*
                 * Level statistics (eg: exons and introns)
                 */
                
                Counts lTP = 0;
                Counts lNR = 0;

                inline Counts aNQ() const { return aTP + aFP; }
                inline Counts lFN() const { return lNR - lTP; }

                inline double sn() const
                {
                    return (lTP + lFN()) ? static_cast<double>(lTP) / (lTP + lFN()) : NAN;
                }

                inline double pc() const
                {
                    return (aTP + aFP) ? static_cast<double>(aTP) / (aTP + aFP) : NAN;
                }
            };

            struct Stats : public AlignmentStats
            {
                enum class AlignMetrics
                {
                    AlignExon,
                    AlignIntron,
                    AlignBase
                };

                enum class MissingMetrics
                {
                    MissingExon,
                    MissingIntron,
                    MissingGene
                };

                struct Data
                {
                    std::map<ExonID,   GeneID> exonToGene;
                    std::map<IntronID, GeneID> intronToGene;
                    
                    BinCounts eContains, eOverlaps;
                    BinCounts iContains, iOverlaps;
                    
                    // Intervals for exons in TransQuin
                    Intervals<TransRef::ExonInterval> eInters;
                    
                    // Intervals for introns in TransQuin
                    Intervals<TransRef::IntronInterval> iInters;
                    
                    /*
                     * Overall statistics
                     */
                    
                    MergedConfusion overE, overI;
                    
                    // Overall performance at the base level
                    Performance overB;
                    
                    Hist histE, histI;
                    
                    /*
                     * Individual statistics for each gene (due to alternative splicing)
                     */
                    
                    std::map<GeneID, MergedConfusion> geneE, geneI;
                    
                    // Individual performance at the base level
                    std::map<GeneID, Confusion> geneB;
                    
                    // Alignments that have no mapping
                    std::vector<UnknownAlignment> unknowns;
                    
                    /*
                     * Missing statistics
                     */
                    
                    std::set<Missing> missE, missI, missG;

                    // False positives (left and right)
                    FPStats lFPS, rFPS;
                };

                std::map<ChromoID, Data> data;
                
                /*
                 * Accessor functions
                 */

                inline Limit limit(AlignMetrics m) const
                {
                    const auto h = (m == AlignMetrics::AlignExon)   ? &data.at(ChrT).histE :
                                   (m == AlignMetrics::AlignIntron) ? &data.at(ChrT).histI :
                                                                      &data.at(ChrT).overB.h;
                    return Standard::instance().r_trans.limitGene(*h);
                }
                
                // Number of non-split reads
                inline Counts countNonSplit(const ChromoID &cID) const { return data.at(cID).overE.aNQ(); }
                
                // Number of split reads
                inline Counts countSplit(const ChromoID &cID) const { return data.at(cID).overI.aNQ(); }

                // Number of query bases in the input file
                inline Counts countQBases(const ChromoID &cID) const { return data.at(cID).overB.m.nq(); }

                inline CountPercent missing(const ChromoID &cID, MissingMetrics m) const
                {
                    switch (m)
                    {
                        case MissingMetrics::MissingGene:
                        {
                            return CountPercent(data.at(cID).missG.size(), data.at(cID).histE.size());
                        }
                            
                        case MissingMetrics::MissingExon:
                        {
                            return CountPercent(data.at(cID).missE.size(), data.at(cID).eContains.size());
                        }

                        case MissingMetrics::MissingIntron:
                        {
                            return CountPercent(data.at(cID).missI.size(), data.at(cID).iContains.size());
                        }
                    }
                }

                inline double missProp(const ChromoID &cID, MissingMetrics m) const
                {
                    return missing(cID, m).percent();
                }
                
                // Overall sensitivity
                inline double sn(const ChromoID &cID, AlignMetrics m) const
                {
                    switch (m)
                    {
                        case AlignMetrics::AlignExon:   { return data.at(cID).overE.sn();   }
                        case AlignMetrics::AlignBase:   { return data.at(cID).overB.m.sn(); }
                        case AlignMetrics::AlignIntron: { return data.at(cID).overI.sn();   }
                    }
                }
                
                // Sensitivity at the gene level
                inline double sn(const ChromoID &cID, const GeneID &id) const
                {
                    return data.at(cID).geneE.at(id).sn();
                }

                // Overall precision
                inline double pc(const ChromoID &cID, AlignMetrics m) const
                {
                    switch (m)
                    {
                        case AlignMetrics::AlignExon:   { return data.at(cID).overE.pc();   }
                        case AlignMetrics::AlignBase:   { return data.at(cID).overB.m.pc(); }
                        case AlignMetrics::AlignIntron: { return data.at(cID).overI.pc();   }
                    }
                }
            };

            static Stats analyze(const FileName &, const Options &o = Options());
            static Stats analyze(const std::vector<Alignment> &, const Options &o = Options());

            static std::vector<Stats> analyze(const std::vector<FileName> &files, const Options &o = Options())
            {
                std::vector<TAlign::Stats> stats;
            
                for (const auto &file : files)
                {
                    stats.push_back(analyze(file, o));
                }
            
                return stats;
            }

            static std::vector<Stats> analyze(const std::vector<std::vector<Alignment>> &aligns, const Options &o = Options())
            {
                std::vector<TAlign::Stats> stats;
                
                for (const auto &align : aligns)
                {
                    stats.push_back(analyze(align, o));
                }
                
                return stats;            
            }

            static void report(const std::vector<FileName> &, const Options &o = Options());
    };
}

#endif
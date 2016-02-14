#ifndef V_DISCOVER_HPP
#define V_DISCOVER_HPP

#include <vector>
#include "stats/analyzer.hpp"
#include "VARQuin/VARQuin.hpp"

namespace Anaquin
{
    struct VDiscover
    {
        struct Options : public AnalyzerOptions
        {
            Options() {}
            
            Caller caller;
            
            // Significance level
            Probability sign = 0.1;
        };

        struct Stats : public MappingStats, public SequinStats, public VariantStats
        {
            typedef VariantMatch ChrTData;
            
            struct ChrTStats
            {
                inline Counts count(const std::vector<ChrTData> &data, Mutation type, Probability sign = 1.00) const
                {
                    return std::count_if(data.begin(), data.end(), [&](const ChrTData &d)
                    {
                        return (d.query.type() == type && d.query.pval <= sign) ? 1 : 0;
                    });
                }

                inline Counts tpTot() const { return tps.size(); }
                inline Counts tpSNP() const { return count(tps, Mutation::SNP); }
                inline Counts tpInd() const { return tpTot() - tpSNP(); }

                inline Counts fpTot() const { return fps.size(); }
                inline Counts fpSNP() const { return count(fps, Mutation::SNP); }
                inline Counts fpInd() const { return fpTot() - fpSNP(); }

                inline Counts tnTot() const { return tns.size(); }
                inline Counts tnSNP() const { return count(tns, Mutation::SNP); }
                inline Counts tnInd() const { return tnTot() - tnSNP(); }

                inline Counts fnTot() const { return fns.size(); }
                inline Counts fnSNP() const { return count(fns, Mutation::SNP); }
                inline Counts fnInd() const { return fnTot() - fnSNP(); }

                inline Counts detectTot() const { return tpTot() + fpTot() + tnTot() + fnTot(); }
                inline Counts detectSNP() const { return tpSNP() + fpSNP() + tnSNP() + fnSNP(); }
                inline Counts detectInd() const { return tpInd() + fpInd() + tnInd() + fnInd(); }
                
                inline Counts filterTot() const { return tpTot() + fpTot(); }
                inline Counts filterSNP() const { return tpSNP() + fpSNP(); }
                inline Counts filterInd() const { return tpInd() + fpInd(); }
                
                std::vector<ChrTData> fps, tps, tns, fns;

                // Performance metrics
                Confusion m, m_snp, m_ind;
            };

            typedef CalledVariant EndoData;
            typedef std::vector<EndoData> EndoStats;
            
            // Statistics for synthetic variants
            ChrTStats chrT;

            // Statistics for endogenous variants
            EndoStats endo;
        };

        static Stats analyze(const FileName &, const Options &o = Options());
        static void report(const FileName &, const Options &o = Options());
    };
}

#endif
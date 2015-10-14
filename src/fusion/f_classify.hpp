#ifndef F_CLASSIFY_HPP
#define F_CLASSIFY_HPP

#include "stats/analyzer.hpp"
#include "parsers/parser_top_fusion.hpp"
#include "parsers/parser_star_fusion.hpp"

namespace Anaquin
{
    struct FClassify
    {
        enum FusionClassify
        {
            Genome     = -2,
            GenomeChrT = -1,
            Negative   = 0,
            Positive   = 1,
        };

        template <typename Options, typename T> static FusionClassify
                classifyFusion(const T &f, Confusion &m, SequinID &id, Options &o)
        {
            const auto &r = Standard::instance().r_fus;

            if (f.chr_1 != Standard::instance().id || f.chr_2 != Standard::instance().id)
            {
                if (f.chr_1 != Standard::instance().id && f.chr_2 != Standard::instance().id)
                {
                    return Genome;
                }
                else
                {
                    return GenomeChrT;
                }
            }

            if (classify(m, f, [&](const T &)
            {
                const auto min = std::min(f.l1, f.l2);
                const auto max = std::max(f.l1, f.l2);

                const auto m = r.find(min, max, f.s1, f.s2, o.fuzzy);
                
                if (m)
                {
                    id = m->id;
                }

                return m ? Positive : Negative;
            }))
            {
                return Positive;
            }

            return Negative;
        }

        template <typename Options, typename Stats> static Stats analyze
                (const FileName &file, bool shouldMix, const Options &o = Options())
        {
            Stats stats;
            const auto &r = Standard::instance().r_fus;

            o.info("Fuzzy level: " + std::to_string(o.fuzzy));
            o.info("Parsing alignment file");

            auto positive = [&](const SequinID &id, Reads reads)
            {
                assert(!id.empty() && r.match(id));
                
                stats.n_chrT++;
                stats.h.at(id)++;

                if (shouldMix)
                {
                    // Known abundance for the fusion
                    const auto known = r.match(id)->abund(Mix_1);
                    
                    // Measured abundance for the fusion
                    const auto measured = reads;
                    
                    Point p;
                    
                    p.x = known;
                    p.y = measured;

                    stats[id] = p;
                }
            };

            SequinID id;

            if (o.soft == FDiscover::Software::Star)
            {
                ParserStarFusion::parse(Reader(file), [&](const ParserStarFusion::Fusion &f, const ParserProgress &)
                {
                    const auto r = classifyFusion(f, stats.m, id, o);
                    
                    switch (r)
                    {
                        case FusionClassify::Negative:   { break;                        }
                        case FusionClassify::Genome:     { stats.n_expT++;        break; }
                        case FusionClassify::GenomeChrT: { stats.hg38_chrT++;     break; }
                        case FusionClassify::Positive:   { positive(id, f.reads); break; }
                    }
                });
            }
            else
            {
                ParserTopFusion::parse(Reader(file), [&](const ParserTopFusion::Fusion &f, const ParserProgress &)
                {
                    const auto r = classifyFusion(f, stats.m, id, o);
                    
                    switch (r)
                    {
                        case FusionClassify::Negative:   { break;                        }
                        case FusionClassify::Genome:     { stats.n_expT++;        break; }
                        case FusionClassify::GenomeChrT: { stats.hg38_chrT++;     break; }
                        case FusionClassify::Positive:   { positive(id, f.reads); break; }
                    }
                });
            }

            /*
             * Find out all the sequins undetected in the experiment
             */

            o.info("Detected " + std::to_string(stats.h.size()) + " sequins in the reference");
            o.info("Checking for missing sequins");
            
//            for (const auto &i : s.seqIDs)
//            {
//                const auto &seqID = i;
//
//                // If the histogram has an entry of zero
//                if (!stats.h.at(seqID))
//                {
//                    if (!s.seqs_1.count(seqID))
//                    {
//                        o.warn(seqID + " defined in the referene but not in the mixture and it is undetected.");
//                        continue;
//                    }
//
//                    o.warn(seqID + " defined in the referene but not detected");
//
//                    const auto seq = s.seqs_1.at(seqID);
//
//                    // Known abundance for the fusion
//                    const auto known = seq.abund() / seq.length;
//
//                    //stats.y.push_back(0); // TODO: We shouldn't even need to add those missing sequins!
//                    //stats.z.push_back(seqID);
//                    //stats.x.push_back(log2f(known));
//
//                    stats.miss.push_back(MissingSequin(seqID, known));
//                }
//            }

            // The references are simply the known fusion points
            stats.m.nr = r.countFusions();

            o.info("Calculating limit of sensitivity");

            if (shouldMix)
            {
                stats.s = r.limit(stats.h);
            }

            return stats;
        }
    };
}

#endif
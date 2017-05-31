#ifndef PARSER_BAMBED_HPP
#define PARSER_BAMBED_HPP

#include "data/intervals.hpp"
#include "parsers/parser_bam.hpp"

namespace Anaquin
{
    /*
     * Builds support for BED regions on top of SAM/BAM alignments
     */

    struct ParserBAMBED
    {
        struct Stats
        {
            ID2Intervals inters;
            
            Counts nNA  = 0;
            Counts nMap = 0;
        };

        enum class Response
        {
            OK,
            SKIP_MATCH,
            SKIP_EVERYTHING
        };
        
        template <typename F> static ParserBAMBED::Stats parse(const FileName &file,
                                                               const C2Intervals &c2l,
                                                               F f)
        {
            ParserBAMBED::Stats stats;

            // For each chromosome...
            for (const auto &i : c2l)
            {
                Intervals<> x;
                
                for (const auto &inter : i.second.data())
                {
                    const auto &l = inter.second.l();
                    x.add(Interval(l.key(), l));
                }

                stats.inters[i.first] = x;
                stats.inters[i.first].build();
            }

            ParserBAM::parse(file, [&](ParserBAM::Data &x, const ParserBAM::Info &info)
            {
                Interval *matched = nullptr;
                
                if (x.mapped && stats.inters.count(x.cID))
                {
                    matched = stats.inters[x.cID].overlap(x.l);
                }
                
                const auto r = f(x, info, matched);
                
                if (r != Response::SKIP_EVERYTHING)
                {
                    if (r != Response::SKIP_MATCH && matched)
                    {
                        matched->map(x.l);
                        
                        if (x.cID != "*")
                        {
                            stats.nMap++;
                        }
                        else
                        {
                            stats.nNA++;
                        }
                    }
                    else
                    {
                        stats.nNA++;
                    }
                }
            });
            
            return stats;
        }
    };
}

#endif

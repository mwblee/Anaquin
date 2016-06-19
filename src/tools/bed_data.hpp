#ifndef BED_DATA_HPP
#define BED_DATA_HPP

#include "data/hist.hpp"
#include "data/standard.hpp"
#include "data/intervals.hpp"
#include "stats/analyzer.hpp"
#include "parsers/parser_bed.hpp"

namespace Anaquin
{
    typedef ParserBed::Data Data;

    struct BedChrData
    {
        std::map<std::string, Data> g2d;
    };
    
    struct BedData : public std::map<ChrID, BedChrData>
    {
        inline Counts countGene(const ChrID &cID) const
        {
            return at(cID).g2d.size();
        }
        
        inline Counts countGene() const
        {
            return ::Anaquin::count(*this, [&](const ChrID &cID, const BedChrData &x)
            {
                return countGene(cID);
            });
        }
        
        inline Counts countGeneSyn() const
        {
            return ::Anaquin::count(*this, [&](const ChrID &cID, const BedChrData &x)
            {
                return Standard::isSynthetic(cID) ? countGene(cID) : 0;
            });
        }

        inline Counts countGeneGen() const
        {
            return countGene() - countGeneSyn();
        }

        inline std::map<ChrID, Hist> hist() const
        {
            std::map<ChrID, Hist> r;
            
            for (const auto &i : *this)
            {
                for (const auto &j :i.second.g2d)
                {
                    r[i.first][j.first];
                }
            }

            return r;
        }
        
        inline Intervals<> gIntervals(const ChrID &cID) const
        {
            Intervals<> r;
            
            for (const auto &i : at(cID).g2d)
            {
                r.add(Interval(i.first, i.second.l));
            }
            
            r.build();
            return r;
        }

        // Intervals for the genes
        inline std::map<ChrID, Intervals<>> gInters() const
        {
            std::map<ChrID, Intervals<>> r;
            
            for (const auto &i : *this)
            {
                r[i.first] = gIntervals(i.first);
            }
            
            return r;
        }
        
        // Genomic intervals for the genes
        inline std::map<ChrID, Intervals<>> gIntersGen() const
        {
            std::map<ChrID, Intervals<>> r;
            
            for (const auto &i : *this)
            {
                if (!Standard::isSynthetic(i.first))
                {
                    r[i.first] = gIntervals(i.first);
                }
            }
            
            return r;
        }
    };
    
    inline BedData bedData(const Reader &r)
    {
        BedData c2d;
        
        ParserBed::parse(r, [&](const ParserBed::Data &x, const ParserProgress &)
        {
            c2d[x.cID].g2d[x.name] = x;
        });

        return c2d;
    }    
}

#endif

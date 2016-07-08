#include "tools/sample.hpp"
#include "parsers/parser_sam.hpp"
#include "writers/writer_sam.hpp"

using namespace Anaquin;

Sampler::Stats Sampler::subsample(const FileName &file, Proportion p, const AnalyzerOptions &o, User *user)
{
    Sampler::Stats stats;
    
    assert(p > 0.0 && p < 1.0);    
    Random r(1.0 - p);

    WriterSAM w;
    w.openTerm();
    
    ParserSAM::parse(file, [&](ParserSAM::Data &x, const ParserSAM::Info &info)
    {
        if (info.p.i && !(info.p.i % 1000000))
        {
            o.logInfo(std::to_string(info.p.i));
        }
        
        const auto shouldWrite = !x.mapped || !Standard::isSynthetic(x.cID);

        if (shouldWrite)
        {
            stats.before.gen++;
        }
        else
        {
            stats.before.syn++;
        }
        
        // This is the key, randomly write the reads with certain probability
        if (shouldWrite || r.select(x.name))
        {
            const auto isSyn = Standard::isSynthetic(x.cID);
            
            if (x.mapped && isSyn)
            {
                assert(Standard::isSynthetic(x.cID));
                
                if (user)
                {
                    user->syncReadSampled(x);
                }
            }
            
            if (!shouldWrite)
            {
                stats.after.syn++;
                o.logInfo("Sampled " + x.name);
            }

            // Print the SAM line
            w.write(x);
        }
    }, true);
    
    assert(stats.before.syn >= stats.after.syn);    
    stats.after.gen = stats.before.gen;
    
    w.close();
    
    return stats;
}
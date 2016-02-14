#include "FusQuin/f_express.hpp"
#include "parsers/parser_stab.hpp"

using namespace Anaquin;

// Defined resources.cpp
extern Scripts PlotExpress_F();

FExpress::Stats FExpress::analyze(const FileName &splice, const Options &o)
{
    const auto &r = Standard::instance().r_fus;
    
    FExpress::Stats stats;
    
    //stats.hist = r.fusionHist();

    switch (o.caller)
    {
        case FusionCaller::StarFusion:
        {
            ParserSTab::parse(Reader(splice), [&](const ParserSTab::Chimeric &c, const ParserProgress &)
            {
                if (c.id == ChrT)
                {
                    stats.n_chrT++;
                }
                else
                {
                    stats.n_endo++;
                }

                const SequinData *match;
                
                if ((match = r.findSplice(c.l)))
                {
                    const auto expected = match->mixes.at(Mix_1);
                    const auto measured = c.unique;

                    stats.add(match->id, expected, measured);
                    //stats.hist.at(match->id)++;
                }
            });

            break;
        }

        case FusionCaller::TopHatFusion:
        {
            throw "TopHatFusion not supported in FusionExpress";
        }
    }

    stats.limit = r.limit(stats.hist);

    return stats;
}

static void writeCSV(const FileName &file, const FExpress::Stats &stats, const FExpress::Options &o)
{
    o.writer->open(file);
    
    const auto format = "%1%\t%2%\t%3%";
    
    o.writer->write((boost::format(format) % "Sequin"
                                           % "EAbund"
                                           % "MAbund").str());
    
    const auto data = stats.data(false);

    for (auto i = 0; i < data.ids.size(); i++)
    {
        o.writer->write((boost::format(format) % data.ids[i]
                                               % data.x[i]
                                               % data.y[i]).str());
    }
    
    o.writer->close();
}

void FExpress::report(const FileName &file, const Options &o)
{
    const auto stats = FExpress::analyze(file, o);

    o.info("Generating statistics");
    
    /*
     * Generating summary statistics
     */
    
    o.writer->open("FusionExpress_summary.stats");
    o.writer->write(StatsWriter::inflectSummary(o.rChrT, o.rEndo, file, stats.hist, stats, stats, ""));
    o.writer->close();

    /*
     * Generating CSV for all fusions
     */
    
    writeCSV("FusionExpress_quins.csv", stats, o);

    /*
     * Generating scatter plot
     */
    
    o.writer->open("FusionExpress_scatter.R");
    o.writer->write(RWriter::createScript("FusionExpress_quins.csv", PlotExpress_F()));
    o.writer->close();
}
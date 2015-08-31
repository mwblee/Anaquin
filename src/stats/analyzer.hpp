#ifndef GI_ANALYZER_HPP
#define GI_ANALYZER_HPP

#include <map>
#include <memory>
#include "classify.hpp"
#include "data/types.hpp"
#include <boost/format.hpp>
#include <ss/regression/lm.hpp>
#include "writers/r_writer.hpp"
#include "stats/sensitivity.hpp"
#include "writers/mock_writer.hpp"

namespace Anaquin
{
    typedef std::map<Locus, Counts>    LocusHist;
    typedef std::map<BaseID, Counts>   BaseHist;
    typedef std::map<SequinID, Counts> SequinHist;

    /*
     * List of softwares supported by Anaquin
     */

    enum Software
    {
        NA,
        Star,
        TopHat,
        Cufflink,
    };

    template <typename Iter, typename T> static std::map<T, Counts> counter(const Iter &iter)
    {
        std::map<T, Counts> c;
        
        for (const auto &i : iter)
        {
            c[static_cast<T>(i)] = 0;
        }
        
        return c;
    }

    template <typename T> static void sums(const std::map<T, Counts> &m, Counts &c)
    {
        for (const auto &i : m)
        {
            if (i.second == 0)
            {
                c++;
            }
            else
            {
                c += i.second;
            }
        }

        assert(c);
    }

    struct Analyzer
    {
        template <typename Iter> static std::map<typename Iter::value_type, Counts> hist(const Iter &iter)
        {
            std::map<typename Iter::value_type, Counts> h;

            for (const auto &i : iter)
            {
                h[i] = 0;
            }

            assert(!h.empty());
            return h;
        }
        
        // Create a histogram for each of the base
        static BaseHist baseHist()
        {
            return hist(Standard::instance().baseIDs);
        }

        // Create a histogram for each of the sequin
        static SequinHist seqHist()
        {
            return hist(Standard::instance().seqIDs);
        }
    };

    /*
     * Represents a sequin that is not detected in the experiment
     */
    
    struct MissingSequin
    {
        MissingSequin(const SequinID &id, Concentration abund) : id(id), abund(abund) {}

        SequinID id;

        // The expect abundance
        Concentration abund;
    };
    
    typedef std::vector<MissingSequin> MissingSequins;

    /*
     * Represents a simple linear regression fitted by maximum-likehihood estimation.
     *
     *   Model: y ~ c + m*x
     */

    struct LinearModel
    {
        // Constant coefficient
        double c;

        // Least-squared slope coefficient
        double m;

        // Adjusted R2
        double r2;

        // Pearson correlation
        double r;
    };

    // Classify at the base-level by counting for non-overlapping regions
    template <typename I1, typename I2> void countBase(const I1 &r, const I2 &q, Confusion &m, SequinHist &c)
    {
        typedef typename I2::value_type Type;
        
        assert(!Locus::overlap(r));
        
        const auto merged = Locus::merge<Type, Locus>(q);
        
        for (const auto &l : merged)
        {
            m.nq   += l.length();
            m.tp() += countOverlaps(r, l, c);
            m.fp()  = m.nq - m.tp();
            
            // Make sure we don't run into negative
            assert(m.nq >= m.tp());
        }

        assert(!Locus::overlap(merged));
    }

    struct ModelStats
    {
        // This is needed to make the compiler happy ...
        ModelStats() {}

        ModelStats(const ModelStats &stats) : s(stats.s), z(stats.z), x(stats.x), y(stats.y) {}
        
        Sensitivity s;

        // Sequin IDs for each x and y
        std::vector<SequinID> z;

        std::vector<double> x, y;

        inline LinearModel linear() const
        {
            std::vector<double> y_;
            std::vector<double> x_;
            
            assert(x.size() == y.size());
            
            /*
             * Ignore any invalid value...
             */
            
            for (auto i = 0; i < x.size(); i++)
            {
                if (!isnan(x[i]) && !isnan(y[i]))
                {
                    x_.push_back(x[i]);
                    y_.push_back(y[i]);
                }
            }

            const auto m = SS::lm("y~x", SS::data.frame(SS::c(y_), SS::c(x_)));
            
            LinearModel lm;
            
            // Pearson correlation
            lm.r = SS::cor(x_, y_);
            
            // Adjusted R2
            lm.r2 = m.ar2;
            
            // Constant coefficient
            lm.c = m.coeffs[0].value;

            // Regression slope
            lm.m = m.coeffs[1].value;

            return lm;
        }
    };

    struct AnalyzerOptions
    {
        std::set<SequinID> filters;

        std::shared_ptr<Writer> writer = std::shared_ptr<Writer>(new MockWriter());
        std::shared_ptr<Writer> logger = std::shared_ptr<Writer>(new MockWriter());
        std::shared_ptr<Writer> output = std::shared_ptr<Writer>(new MockWriter());

        enum LogLevel
        {
            Info,
            Warn,
            Error,
        };

        inline void warn(const std::string &s) const
        {
            logger->write("[WARN]: " + s);
            output->write("[WARN]: " + s);
        }

        inline void wait(const std::string &s) const
        {
            logger->write("[WAIT]: " + s);
            output->write("[WAIT]: " + s);
        }
        
        inline void info(const std::string &s) const
        {
            logInfo(s);
            output->write("[INFO]: " + s);
        }

        inline void logInfo(const std::string &s) const
        {
            logger->write("[INFO]: " + s);
        }

        inline void logWarn(const std::string &s) const
        {
            logger->write("[WARN]: " + s);
        }
        
        inline void error(const std::string &s) const
        {
            logger->write("[ERROR]: " + s);
            output->write("[ERROR]: " + s);
        }

        // Write to the standard terminal
        inline void out(const std::string &s) const { output->write(s); }
    };

    struct SingleMixtureOptions : public AnalyzerOptions
    {
        Mixture mix = MixA;
    };

    struct DoubleMixtureOptions : public AnalyzerOptions
    {
        const Mixture rMix = MixA;
        const Mixture qMix = MixB;
    };

    struct ViewerOptions : public AnalyzerOptions
    {
        std::string path;
    };
    
    struct Expression
    {
        template <typename Map> static void print(const Map &m)
        {
            for (auto iter = m.begin(); iter != m.end(); iter++)
            {
                std::cout << iter->first << "  " << iter->second << std::endl;
            }
        }

        template <typename T, typename ID, typename S> static Sensitivity
                analyze(const std::map<T, Counts> &c, const std::map<ID, S> &m)
        {
            Sensitivity s;
            
            // The lowest count must be zero because it can't be negative
            s.counts = std::numeric_limits<unsigned>::max();

            for (auto iter = c.begin(); iter != c.end(); iter++)
            {
                const auto counts = iter->second;
                
                /*
                 * Is this sequin detectable? If it's detectable, what about the concentration?
                 * By definition, the detection limit is defined as the smallest abundance while
                 * still being detected.
                 */
                
                if (counts)
                {
                    const auto &id = iter->first;
                    
                    if (!m.count(id))
                    {
                        //std::cout << "Warning: " << id << " not found in LOS" << std::endl;
                        continue;
                    }
                    
                    if (counts < s.counts || (counts == s.counts && m.at(id).abund() < s.abund))
                    {
                        s.id     = id;
                        s.counts = counts;
                        s.abund  = m.at(s.id).abund();
                    }
                }
            }
            
            if (s.counts == std::numeric_limits<unsigned>::max())
            {
                s.counts = 0;
            }
            
            return s;
        }
    };

    struct AnalyzeReporter
    {
        template <typename Stats, typename Writer> static void missing(const std::string &file,
                                                                       const Stats &stats,
                                                                       Writer writer)
        {
            const auto format = "%1%\t%2%";

            writer->open(file);
            writer->write((boost::format(format) % "id" % "abund").str());

            for (const auto &i : stats.miss)
            {
                writer->write((boost::format(format) % i.id % i.abund).str());
            }

            writer->close();
        }

        template <typename Stats, typename Writer>
            static void writeCSV(const Stats &stats, const std::string file, Writer writer)
        {
            writer->open(file);
            writer->write("ID,expect,measure");

            /*
             * Prefer to write results in sorted order
             */

            std::set<std::string> sorted(stats.z.begin(), stats.z.end());

            for (const auto &s : sorted)
            {
                const auto it = std::find(stats.z.begin(), stats.z.end(), s);
                const auto i  = std::distance(stats.z.begin(), it);
                
                writer->write((boost::format("%1%,%2%,%3%") % stats.z[i] % stats.x[i] % stats.y[i]).str());
            }
            
            writer->close();
        }
        
        template <typename Stats, typename Writer> static void linear(const Stats &stats,
                                                                      const std::string prefix,
                                                                      const std::string unit,
                                                                      Writer writer,
                                                                      bool r       = true,
                                                                      bool summary = true,
                                                                      bool sequin  = true)
        {
            assert(stats.x.size() == stats.y.size() && stats.y.size() == stats.z.size());

            std::vector<double> x, y;
            std::vector<std::string> z;

            /*
             * Ignore any invalid value...
             */
            
            for (auto i = 0; i < stats.x.size(); i++)
            {
                if (!isnan(stats.x[i]) && !isnan(stats.y[i]))
                {
                    x.push_back(stats.x[i]);
                    y.push_back(stats.y[i]);
                    z.push_back(stats.z[i]);
                }
            }

            /*
             * Generate a script for data visualization
             */

            if (r)
            {
                writer->open(prefix + ".R");
                writer->write(RWriter::write(x, y, z, unit, stats.s.abund));
                writer->close();
            }
            
            const std::string format = "%1%\t%2%\t%3%\t%4%";
            const auto lm = stats.linear();

            /*
             * Generate summary statistics
             */

            if (summary)
            {
                writer->open(prefix + "_summary.stats");
                writer->write((boost::format(format) % "r" % "slope" % "r2" % "ss").str());
                writer->write((boost::format(format) % lm.r % lm.m % lm.r2 % stats.s.abund).str());
                writer->close();
            }

            /*
             * Generate CSV for each sequin
             */

            if (sequin)
            {
                writeCSV(stats, prefix + "_quins.csv", writer);
            }
        }

        template <typename Writer, typename Histogram> static void stats(const FileName &name,
                                                                         const Performance &p,
                                                                         const Histogram   &h,
                                                                         Writer writer)
        {
            const std::string format = "%1%\t%2%\t%3%\t%4%\t%5%";

            const auto sn = p.m.sn();
            const auto sp = p.m.sp();
            const auto ss = p.s.abund;

            assert(ss >= 0);
            assert(isnan(sn) || (sn >= 0 && sn <= 1.0));
            assert(isnan(sp) || (sp >= 0 && sp <= 1.0));

            writer->open(name);
            writer->write((boost::format(format) % "sn" % "sp" % "los"  % "ss" % "counts").str());
            writer->write((boost::format(format) %  sn  %  sp  % p.s.id %  ss  % p.s.counts).str());
            writer->write("\n");

            writer->close();
        };
    };
}

#endif

#include <ctime>
#include <iostream>
#include <unistd.h>
#include <getopt.h>

#include "data/tokens.hpp"
#include "data/reader.hpp"
#include "data/resources.hpp"

#include "rna/r_diffs.hpp"
#include "rna/r_align.hpp"
#include "rna/r_assembly.hpp"
#include "rna/r_abundance.hpp"

#include "dna/d_align.hpp"
#include "dna/d_variant.hpp"
#include "dna/d_sequence.hpp"

#include "meta/m_blast.hpp"
#include "meta/m_diffs.hpp"
#include "meta/m_assembly.hpp"

#include "parsers/parser_csv.hpp"
#include "writers/path_writer.hpp"
#include "parsers/parser_sequins.hpp"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#define CMD_VER    'v'
#define CMD_TEST   't'
#define CMD_RNA   265
#define CMD_DNA   266
#define CMD_META  267

#define MODE_BLAST        281
#define MODE_SEQUENCE     282
#define MODE_ALIGN        283
#define MODE_ASSEMBLY     284
#define MODE_ABUNDANCE    285
#define MODE_DIFFERENTIAL 286
#define MODE_VARIATION    287
#define MODE_SEQUINS      288

#define OPT_MIN     321
#define OPT_MAX     322
#define OPT_LOS     323
#define OPT_OUTPUT  324
#define OPT_PSL     325
#define OPT_MODEL   326
#define OPT_MIXTURE 327
#define OPT_FILTER  328

using namespace Spike;

struct InvalidUsageError : public std::runtime_error
{
    InvalidUsageError() : std::runtime_error("") { }
};

struct InvalidFilterError : public std::runtime_error
{
    InvalidFilterError(const std::string &msg) : std::runtime_error(msg) { }
};

/*
 * Variables used in argument parsing
 */

// The path that output files are written
static std::string _output;

// Name of a PSL alignment for the first sample generated by BLAST
static std::string _psl_A;

// Name of a PSL alignment for the second sample generated by BLAST
static std::string _psl_B;

// Name of a custom model file (eg: BED, GTF)
static std::string _model;

// Name of a custom CSV file
static std::string _mixture;

// The sequins that have been filtered
static std::set<SequinID> _filters;

static int _cmd  = 0;
static int _mode = 0;

// The operands for the command
std::vector<std::string> _opts;

static void reset()
{
    _opts.clear();
    _psl_A.clear();
    _psl_B.clear();
    _model.clear();
    _output.clear();
    _mixture.clear();
    _filters.clear();
    _cmd = _mode = 0;
}

/*
 * Argument options
 */

static const char *short_options = "";

static const struct option long_options[] =
{
    { "v", no_argument, 0, CMD_VER  },
    { "t", no_argument, 0, CMD_TEST },

    { "rna",  required_argument, 0, CMD_RNA  },
    { "dna",  required_argument, 0, CMD_DNA  },
    { "meta", required_argument, 0, CMD_META },

    { "psl",     required_argument, 0, OPT_PSL     },
    { "min",     required_argument, 0, OPT_MIN     },
    { "max",     required_argument, 0, OPT_MAX     },
    { "los",     required_argument, 0, OPT_LOS     },

    { "mod",     required_argument, 0, OPT_MODEL   },
    { "model",   required_argument, 0, OPT_MODEL   },
    
    { "mix",     required_argument, 0, OPT_MIXTURE },
    { "mixture", required_argument, 0, OPT_MIXTURE },

    { "o",       required_argument, 0, OPT_OUTPUT  },
    { "output",  required_argument, 0, OPT_OUTPUT  },
    
    { "f",        required_argument, 0, OPT_FILTER },
    { "filter",   required_argument, 0, OPT_FILTER },

    { "l",        no_argument, 0, MODE_SEQUINS },
    { "sequins",  no_argument, 0, MODE_SEQUINS },

    { "as",       required_argument, 0, MODE_ASSEMBLY },
    { "assembly", required_argument, 0, MODE_ASSEMBLY },

    { "al",    required_argument, 0, MODE_ALIGN },
    { "align", required_argument, 0, MODE_ALIGN },

    { "df",    required_argument, 0, MODE_DIFFERENTIAL },
    { "diffs", required_argument, 0, MODE_DIFFERENTIAL },

    { "ab",        required_argument, 0, MODE_ABUNDANCE },
    { "abundance", required_argument, 0, MODE_ABUNDANCE },

    { "var",      required_argument, 0, MODE_VARIATION },
    { "variant",  required_argument,  0, MODE_VARIATION },

    { "blast", required_argument, 0, MODE_BLAST },

    {0, 0, 0, 0 }
};

static void printUsage()
{
    extern std::string manual();
    std::cout << manual() << std::endl;
}

static void printVersion()
{
    const auto c = Resources::chromo();
    const auto m = Resources::mixture();

    std::cout << "Version 1.0. Garvan Institute of Medical Research, 2015." << std::endl;
    std::cout << std::endl;
    std::cout << "Chromosome: " << c.id << " version " << c.v << std::endl;
    std::cout << "Mixture A: version " << m.va << std::endl;
    std::cout << "Mixture B: version " << m.vb << std::endl;
}

// Print a file of mixture A and B
void print(Reader &r)
{
    /*
     * Format: <ID, Mix A, Mix B>
     */

    std::string l;
    
    // Skip the first line
    r.nextLine(l);

    std::cout << "ID\tMix A\tMix B" << std::endl;
    
    while (r.nextLine(l))
    {
        if (l == "\r" || l == "\n" || l == "\r\n")
        {
            continue;
        }

        std::vector<std::string> tokens;
        Tokens::split(l, ",", tokens);

        if (tokens.size() != 3)
        {
            std::cerr << "Malformed file: [" << l << "]" << std::endl;
        }

        std::cout << tokens[0] << "\t" << tokens[1] << "\t" << tokens[2] << std::endl;
    }
}

void print_meta()
{
    extern std::string m_mix_f();
    Reader r(m_mix_f(), String);
    print(r);
}

static void print_rna()
{
    // Empty Implementation
}

void print_dna()
{
    // Empty Implementation
}

template <typename Mixture, typename Model> void applyCustom(Mixture mix, Model mod)
{
    if (_mixture.empty() && _model.empty())
    {
        return;
    }
    else if ((!_mixture.empty() && _model.empty()) || (_mixture.empty() && !_model.empty()))
    {
        throw InvalidUsageError();
    }
    
    std::cout << "Mixture file: " << _mixture << std::endl;
    std::cout << "Model file: "   << _model   << std::endl;

    mix(Reader(_mixture));
    mod(Reader(_model));
}

// Read sequins from a file, one per line. The identifiers must match.
static void readFilters(const std::string &file)
{
    Reader r(file);
    std::string line;
    
    // We'll use it to compare the sequins
    const auto &s = Standard::instance();

    while (r.nextLine(line))
    {
        switch (_cmd)
        {
            case CMD_RNA:
            {
                break;
            }

            case CMD_DNA:
            {
                break;
            }

            case CMD_META:
            {
                // The sequins must match other than the specific mixture
                assert(s.m_seq_A.size() == s.m_seq_B.size());

                if (!s.m_seq_A.count(line))
                {
                    throw InvalidFilterError("Unknown sequin for metagenomics: " + line);
                }

                _filters.insert(line);
                break;
            }

            default: { throw InvalidUsageError(); }
        }
    }

    if (_filters.empty())
    {
        throw InvalidFilterError("No sequin found in: " + file);
    }
}

template <typename Analyzer, typename F> void analyzeF(F f, typename Analyzer::Options o)
{
    const auto path = _output.empty() ? "spike_out" : _output;
    
    std::cout << "Path: " << path << std::endl;
    o.writer = std::shared_ptr<PathWriter>(new PathWriter(path));

    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "------------- Sequin Analysis -----------" << std::endl;
    std::cout << "-----------------------------------------" << std::endl << std::endl;
    
    for (const auto &filter : (o.filters = _filters))
    {
        std::cout << "Filter: " << filter << std::endl;
    }
    
    std::clock_t begin = std::clock();
    
    f(o);
    
    std::clock_t end = std::clock();
    const double elapsed = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Completed. Elpased: " << elapsed << " seconds" << std::endl;
}

// Analyze for a single-sample input
template <typename Analyzer> void analyze(const std::string &file, typename Analyzer::Options o = typename Analyzer::Options())
{
    return analyzeF<Analyzer>([&](const typename Analyzer::Options &o) { Analyzer::analyze(file, o); }, o);
}

// Analyze for two-samples input
template <typename Analyzer> void analyze(const std::string &f1, const std::string &f2, typename Analyzer::Options o = typename Analyzer::Options())
{
    return analyzeF<Analyzer>([&](const typename Analyzer::Options &o) { Analyzer::analyze(f1, f2, o); }, o);
}

template <typename Options> static Options detect(const std::string &file)
{
    const bool found_gene = file.find("gene") != std::string::npos;
    const bool found_isoform = file.find("isoform") != std::string::npos;

    Options o;
    
    if (found_gene && !found_isoform)
    {
        std::cout << "Calculating for the genes" << std::endl;
        o.level = RNALevel::Gene;
    }
    else if (!found_gene && found_isoform)
    {
        std::cout << "Calcualting for the isoforms" << std::endl;
        o.level = RNALevel::Isoform;
    }
    else
    {
        throw std::runtime_error("Unknown type. Have you specified the level?");
    }

    return o;
}

void parse(int argc, char ** argv)
{
    reset();

    if (argc <= 1)
    {
        printUsage();
    }
    else
    {
        const auto tmp = std::string(argv[1]);
        
        if (tmp == "rna")  { _cmd = CMD_RNA;  }
        if (tmp == "dna")  { _cmd = CMD_DNA;  }
        if (tmp == "meta") { _cmd = CMD_META; }
    }
    
    int next, index;

#ifdef UNIT_TESTING
    optind = optreset = 1;
#endif

    while ((next = getopt_long_only(argc, argv, short_options, long_options, &index)) != -1)
    {
        switch (next)
        {
            case OPT_OUTPUT:  { _output = optarg;    break; }
            case OPT_MODEL:   { _model = optarg;     break; }
            case OPT_MIXTURE: { _mixture = optarg;   break; }
            case OPT_FILTER:  { readFilters(optarg); break; }
                
            case OPT_MAX:
            case OPT_MIN:
            case OPT_LOS:
            {
                break;
            }

            case OPT_PSL:
            {
                _psl_A = optarg;

                if (optind < argc && *argv[optind] != '-')
                {
                    _psl_B = argv[optind++];
                }
                
                break;
            }
                
            case CMD_VER:
            case CMD_DNA:
            case CMD_RNA:
            case CMD_META:
            case CMD_TEST:
            {
                if (_cmd != 0)
                {
                    throw InvalidUsageError();
                }
                
                _cmd = next;
                break;
            }
                
            default:
            {
                if (_mode != 0)
                {
                    throw InvalidUsageError();
                }

                _mode = next;
                
                /*
                 * If this is not the last argument, we might try to extract multiple arguments.
                 */
                
                if (optind < argc)
                {
                    optind--;
                    
                    for (; optind < argc && *argv[optind] != '-'; optind++)
                    {
                        _opts.push_back(argv[optind]);
                    }                    
                }
                else if (optarg)
                {
                    _opts.push_back(optarg);
                }
                
                break;
            }
        }
    }
    
    if (_cmd == 0)
    {
        throw InvalidUsageError();
    }
    else if ((_cmd == CMD_TEST || _cmd == CMD_VER) && (!_output.empty() || _mode != 0 || !_opts.empty()))
    {
        throw InvalidUsageError();
    }
    else
    {
        auto &s = Standard::instance();

        switch (_cmd)
        {
            case CMD_VER:  { printVersion();                break; }
            case CMD_TEST: { Catch::Session().run(1, argv); break; }

            case CMD_RNA:
            {
                std::cout << "RNA analysis - Garvan Institute" << std::endl;

                if (_mode != MODE_SEQUINS   &&
                    _mode != MODE_SEQUENCE  &&
                    _mode != MODE_ALIGN     &&
                    _mode != MODE_ASSEMBLY  &&
                    _mode != MODE_ABUNDANCE &&
                    _mode != MODE_DIFFERENTIAL)
                {
                    throw InvalidUsageError();
                }
                else
                {
                    switch (_mode)
                    {
                        case MODE_SEQUENCE: { break; }
                        case MODE_SEQUINS:  { print_rna();                  break; }
                        case MODE_ALIGN:    { analyze<RAlign>(_opts[0]);    break; }
                        case MODE_ASSEMBLY: { analyze<RAssembly>(_opts[0]); break; }
                            
                        case MODE_ABUNDANCE:
                        {
                            analyze<RAbundance>(_opts[0], detect<RAbundance::Options>(_opts[0]));
                            break;
                        }
                            
                        case MODE_DIFFERENTIAL:
                        {
                            analyze<RDiffs>(_opts[0], detect<RDiffs::Options>(_opts[0]));
                            break;
                        }
                    }
                }
                
                break;
            }
                
            case CMD_DNA:
            {
                std::cout << "DNA analysis - Garvan Institute" << std::endl;
                
                if (_mode != MODE_SEQUINS  &&
                    _mode != MODE_SEQUENCE &&
                    _mode != MODE_ALIGN    &&
                    _mode != MODE_VARIATION)
                {
                    throw InvalidUsageError();
                }
                else
                {
                    switch (_mode)
                    {
                        case MODE_SEQUENCE:  { break; }
                        case MODE_SEQUINS:   { print_dna();                 break; }
                        case MODE_ALIGN:     { analyze<DAlign>(_opts[0]);   break; }
                        case MODE_VARIATION: { analyze<DVariant>(_opts[0]); break; }
                    }
                }
                
                break;
            }
                
            case CMD_META:
            {
                std::cout << "Metagenomics analysis - Garvan Institute" << std::endl;
                
                applyCustom(std::bind(&Standard::meta_mix, &s, std::placeholders::_1),
                            std::bind(&Standard::meta_mod, &s, std::placeholders::_1));

                if (_mode != MODE_BLAST    &&
                    _mode != MODE_SEQUINS  &&
                    _mode != MODE_SEQUENCE &&
                    _mode != MODE_ASSEMBLY &&
                    _mode != MODE_DIFFERENTIAL)
                {
                    throw InvalidUsageError();
                }
                else
                {
                    switch (_mode)
                    {
                        case MODE_SEQUENCE: { break; }
                        case MODE_SEQUINS:  { print_meta(); break; }
                            
                        case MODE_BLAST:
                        {
                            /*
                             * Prints the relevant statistics about the alignment file
                             */
                            
                            const std::string format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%\t%7%";
                            
                            // Analyse the BLAST alignment
                            const auto r = MBlast::analyze(_opts[0]);
                            
                            std::cout << (boost::format(format) % "ID"
                                          % "MixA"
                                          % "MixB"
                                          % "N"
                                          % "Cov"
                                          % "Mis"
                                          % "Gaps").str() << std::endl;
                            
                            for (const auto &align : r.metas)
                            {
                                std::cout << (boost::format(format)
                                                % align.second.id
                                                % align.second.seqA.abund()
                                                % align.second.seqB.abund()
                                                % align.second.contigs.size()
                                                % align.second.covered
                                                % align.second.mismatch
                                                % align.second.gaps).str() << std::endl;
                            }

                            break;
                        }
                            
                        case MODE_DIFFERENTIAL:
                        {
                            if (_opts.size() != 2 || (!_psl_A.empty() != !_psl_B.empty()))
                            {
                                throw InvalidUsageError();
                            }
                            
                            MDiffs::Options o;
                            
                            o.psl_1 = _psl_A;
                            o.psl_2 = _psl_B;

                            analyze<MDiffs>(_opts[0], _opts[1], o);
                            break;
                        }

                        case MODE_ASSEMBLY:
                        {
                            MAssembly::Options o;
                            
                            // We'd also take an alignment PSL file from a user
                            o.psl = _psl_A;

                            analyze<MAssembly>(_opts[0], o);
                            break;
                        }
                    }
                }
                
                break;
            }
                
            default:
            {
                assert(false);
            }
        }
    }
}

int parse_options(int argc, char ** argv)
{
    auto printError = [&](const std::string &file)
    {
        std::cerr << std::endl;
        std::cerr << "*********************************************" << std::endl;
        std::cerr << file << std::endl;
        std::cerr << "*********************************************" << std::endl;
    };
    
    try
    {
        parse(argc, argv);
        return 0;
    }
    catch (const EmptyFileError &ex)
    {
        printError((boost::format("%1%%2%") % "Empty file: " % ex.what()).str());
    }
    catch (const InvalidFileError &ex)
    {
        printError((boost::format("%1%%2%") % "Invalid file: " % ex.what()).str());
    }
    catch (const InvalidUsageError &)
    {
        printUsage();
    }
    catch (const InvalidFilterError &ex)
    {
        printError((boost::format("%1%%2%") % "Invalid filter: " % ex.what()).str());
    }

    return 1;
}

int main(int argc, char ** argv)
{
    return parse_options(argc, argv);
}
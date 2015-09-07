#include "data/tokens.hpp"
#include "data/reference.hpp"

using namespace Anaquin;

template <typename Iter> Base countLocus(const Iter &iter)
{
    Base n = 0;
    
    for (const auto &i : iter)
    {
        n += static_cast<Locus>(i).length();
    }
    
    return n;
}

/*
 * ------------------------- Transcriptome Analysis -------------------------
 */

struct TransRef::TransRefImpl
{
    // Number of bases for all the reference exons
    Base exonBase;

    std::map<GeneID, GeneData> genes;
    std::vector<ExonData>      mergedExons;
    std::vector<ExonData>      sortedExons;
    std::vector<IntronData>    sortedIntrons;

    /*
     * Raw data - structure before validated
     */
    
    std::set<GeneID>                           rawGIDs;
    std::set<IsoformID>                        rawIIDs;
    std::map<SequinID, GeneID>                 rawMapper;
    std::map<GeneID, std::vector<ExonData>>    exonsByGenes;
    std::map<IsoformID, std::vector<ExonData>> exonsByTrans;
};

TransRef::TransRef() : _impl(new TransRefImpl()) {}

Base TransRef::exonBase() const { return _impl->exonBase; }

// Limit of detection for the gene level
Sensitivity TransRef::limitGene(const GeneHist &h) const
{
    return Reference<TransData, SequinStats>::limit(h, [&](const GeneID &id)
    {
        return findGene(id);
    });
}

void TransRef::addRef(const IsoformID &iID, const GeneID &gID, const Locus &l)
{
    TransRef::ExonData e;
    
    e.l   = l;
    e.iID = iID;
    e.gID = gID;
    
    _impl->exonsByGenes[gID].push_back(e);
    _impl->exonsByTrans[iID].push_back(e);
    
    _impl->rawIIDs.insert(iID);
    _impl->rawGIDs.insert(gID);
    _impl->rawMapper[iID] = gID;
}

const std::vector<TransRef::ExonData>   & TransRef::mergedExons()   const { return _impl->mergedExons;   }
const std::vector<TransRef::ExonData>   & TransRef::sortedExons()   const { return _impl->sortedExons;   }
const std::vector<TransRef::IntronData> & TransRef::sortedIntrons() const { return _impl->sortedIntrons; }

const TransRef::GeneData * TransRef::findGene(const GeneID &id) const
{
    return _impl->genes.count(id) ? &(_impl->genes.at(id)) : nullptr;
}

template <typename Iter> const typename Iter::mapped_type *findMap(const Iter &x, const Locus &l, TransRef::Matching m)
{
    for (const auto &i : x)
    {
        if ((m == TransRef::Exact && i.second.l() == l) || (m == TransRef::Contains && i.second.l().contains(l)))
        {
            return &i.second;
        }
    }
    
    return nullptr;
}

template <typename Iter> const typename Iter::value_type *findList(const Iter &x, const Locus &l, TransRef::Matching m)
{
    for (const auto &i : x)
    {
        if ((m == TransRef::Exact && i.l == l) || (m == TransRef::Contains && i.l.contains(l)))
        {
            return &i;
        }
    }
    
    return nullptr;
}

const TransRef::GeneData * TransRef::findGene(const Locus &l, TransRef::Matching m) const
{
    return findMap(_impl->genes, l, m);
}

const TransRef::ExonData * TransRef::findExon(const Locus &l, TransRef::Matching m) const
{
    return findList(_impl->sortedExons, l, m);
}

const TransRef::IntronData * TransRef::findIntron(const Locus &l, TransRef::Matching m) const
{
    return findList(_impl->sortedIntrons, l, m);
}

TransRef::GeneHist TransRef::histGene() const
{
    GeneHist h;
    
    for (const auto &i : _impl->genes)
    {
        h[i.first] = 0;
    }
    
    return h;
}

void TransRef::merge(const std::set<SequinID> &mIDs, const std::set<SequinID> &aIDs)
{
    assert(!mIDs.empty() && !aIDs.empty());
    
    std::vector<SequinID> diffs, inters;
    
    /*
     * Check for any sequin defined in mixture but not in annotation
     */
    
    std::set_difference(mIDs.begin(),
                        mIDs.end(),
                        aIDs.begin(),
                        aIDs.end(),
                        std::back_inserter(diffs));
    
    /*
     * Check for any sequin defined in both mixture and annotation
     */
    
    std::set_intersection(mIDs.begin(),
                          mIDs.end(),
                          aIDs.begin(),
                          aIDs.end(),
                          std::back_inserter(inters));
    
    /*
     * Construct a set of validated sequins. A valid sequin is one in which it's
     * defined in both mixture and annoation.
     */
    
    std::for_each(inters.begin(), inters.end(), [&](const SequinID &id)
                  {
                      auto d = TransData();
                      
                      d.id  = id;
                      d.gID = _impl->rawMapper.at(d.id);
                      
                      // Add a new entry for the validated sequin
                      _data[id] = d;
                      
                      assert(!d.id.empty() && !d.gID.empty());
                  });
    
    /*
     * Now, we have a list of validated sequins. Use those sequins to combine information
     * from mixtures and annotations.
     */
    
    for (const auto i : _mixes)
    {
        // Eg: MixA, MixB etc
        const auto mix = i.first;
        
        // For each of the mixture defined
        for (const auto j : i.second)
        {
            // Only if it's a validated sequin
            if (_data.count(j.id))
            {
                _data.at(j.id).length = j.length;
                _data.at(j.id).mixes[mix] = j.abund;
            }
        }
    }
    
    assert(!_data.empty());
    
    /*
     * Compute the locus for each sequin
     */
    
    for (const auto &i : _impl->exonsByTrans)
    {
        if (_data.count(i.first))
        {
            _data[i.first].l = Locus::expand(i.second, [&](const ExonData &f)
                                             {
                                                 return true;
                                             });
            
            _impl->genes[_data[i.first].gID].seqs.push_back(&_data[i.first]);
        }
    }
}

void TransRef::validate()
{
    if (_impl->exonsByGenes.empty())
    {
        throw std::runtime_error("There is no synthetic chromosome in the annotation file. Anaquin is unable to proceed unless a valid annotation is given. Please check your file and try again.");
    }
    
    merge(_rawMIDs, _impl->rawIIDs);
    
    /*
     * Filter out only those validated exons
     */
    
    for (const auto &i : _impl->exonsByTrans)
    {
        if (_data.count(i.first))
        {
            for (const auto &j : i.second)
            {
                //_gIDs.insert(j.gID);
                _impl->sortedExons.push_back(j);
            }
        }
    }
    
    /*
     * Generate a list of sorted exons
     */
    
    assert(!_impl->sortedExons.empty());
    std::sort(_impl->sortedExons.begin(), _impl->sortedExons.end(), [](const ExonData &x, const ExonData &y)
              {
                  return (x.l.start < y.l.start) || (x.l.start == y.l.start && x.l.end < y.l.end);
              });
    
    /*
     * Generate a list of sorted introns, only possible once the exons are sorted.
     */
    
    for (auto i = 0; i < _impl->sortedExons.size(); i++)
    {
        if (i && _impl->sortedExons[i].gID == _impl->sortedExons[i-1].gID)
        {
            IntronData d;
            
            d.gID = _impl->sortedExons[i].gID;
            d.iID = _impl->sortedExons[i].iID;
            d.l   = Locus(_impl->sortedExons[i-1].l.end + 1, _impl->sortedExons[i].l.start - 1);
            
            _impl->sortedIntrons.push_back(d);
        }
    }
    
    assert(!_impl->sortedIntrons.empty());
    std::sort(_impl->sortedIntrons.begin(), _impl->sortedIntrons.end(),
                [](const IntronData &x, const IntronData &y)
                {
                    return (x.l.start < y.l.start) || (x.l.start == y.l.start && x.l.end < y.l.end);
                });

    // Count number of non-overlapping bases for all exons
    _impl->exonBase = countLocus(_impl->mergedExons = Locus::merge<ExonData, ExonData>(_impl->sortedExons));
}

/*
 * ------------------------- Variant Analysis -------------------------
 */

struct VarRef::VariantPair
{
    const MixtureData *r, *v;
};

struct VarRef::VarRefImpl
{
    /*
     * Validated variables
     */

    std::vector<Variation> vars;

    std::map<Mixture, std::map<BaseID, VariantPair>> pairs;
    
    /*
     * Raw variables
     */
    
    std::vector<Variation> rawVars;
};

VarRef::VarRef() : _impl(new VarRefImpl()) {}

std::size_t VarRef::countVars() const
{
    return _impl->vars.size();
}

double VarRef::alleleFreq(Mixture m, const BaseID &bID) const
{
    const auto &p = _impl->pairs.at(m).at(bID);
    const auto &r = p.r;
    const auto &v = p.v;

    // Abundance ratio of reference to variant DNA standard
    return v->abund / (r->abund + v->abund);
}

void VarRef::addVar(const Variation &v)
{
    assert(!v.bID.empty());
    _impl->rawVars.push_back(v);
}

void VarRef::validate()
{
    _impl->vars = _impl->rawVars;

    // Validate sequins defined in the mixture
    merge(_rawMIDs, _rawMIDs);
    
   /*
    * Construct data structure for homozygous/heterozygous
    */

    std::vector<std::string> toks;
    
    for (const auto &i : _mixes)
    {
        for (const auto &j : _mixes.at(i.first))
        {
            Tokens::split(j.id, "_", toks);

            // It has be the reference or variant...
            assert(toks[3] == "R" || toks[3] == "V");
            
            // Eg: D_1_10
            const auto baseID = toks[0] + "_" + toks[1] + "_" + toks[2];

            if (toks[3] == "R")
            {
                _impl->pairs[i.first][baseID].r = &j;
            }
            else
            {
                _impl->pairs[i.first][baseID].v = &j;
            }
        }
    }

    assert(!_impl->pairs.empty());
}

const Variation * VarRef::findVar(const Locus &l, double fuzzy, Matching match) const
{
    for (const auto &i : _impl->vars)
    {
        assert(i.l.length());

        if (match == StartOnly && i.l.start == l.start)
        {
            return &i;
        }
    }

    return nullptr;
}
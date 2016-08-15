#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <string>
#include <assert.h>
#include <klib/khash.h>

namespace Anaquin
{
    class RandomSelection
    {
        public:

            RandomSelection(double prob) : _prob(prob)
            {
                assert(prob >= 0.0);
                _seed = rand();
            }

            inline bool select(const std::string &hash) const
            {
                const uint32_t k = __ac_Wang_hash(__ac_X31_hash_string(hash.c_str()) ^ _seed);
                return ((double)(k&0xffffff) / 0x1000000 >= _prob);
            }
        
        private:

            // Random seed
            int _seed;

            // The probability of selection
            const double _prob;
    };
}

#endif
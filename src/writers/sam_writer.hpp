#ifndef SAM_WRITER_HPP
#define SAM_WRITER_HPP

#include <htslib/sam.h>
#include "tools/samtools.hpp"
#include "writers/writer.hpp"

namespace Anaquin
{
    class SAMWriter : public Writer<ParserBAM::Data>
    {
        public:

            inline void close() override
            {
                sam_close(_fp);
            }

            inline void open(const FileName &file) override
            {
                _fp = sam_open("/dev/null", "w");
            }

            inline void write(const ParserBAM::Data &x) override
            {
                const auto *b = reinterpret_cast<bam1_t *>(x.b());
                const auto *h = reinterpret_cast<bam_hdr_t *>(x.h());

                if (!_header)
                {
                    std::cout << std::string(h->text);
                }
                
                if (sam_write1(_fp, h, b) == -1)
                {
                    throw std::runtime_error("Failed to SAM record");
                }
                
#ifndef UNIT_TEST
                std::cout << std::string(_fp->line.s);
#endif
                _header = true;
            }

        private:

            // Whether the header has written
            bool _header = false;

            // File pointer
            samFile *_fp;
    };
}

#endif

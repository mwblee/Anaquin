#ifndef READER_HPP
#define READER_HPP

#include <vector>
#include <string>
#include <ostream>
#include "parsers/parser.hpp"

namespace Anaquin
{
    struct ReaderInternal;

    enum DataMode
    {
        File,
        String,
    };
    
    /*
     * Reader encapsulates the underlying data source. For example, we could source from a memory string
     * or a physical file.
     */

    class Reader
    {
        public:
            Reader(const Reader &r);
            Reader(const std::string &, DataMode mode = File);

            ~Reader();

            void reset();
        
            // Returns description for the source
            std::string src() const;
        
            // Returns the next line in the file
            bool nextLine(std::string &) const;

            // Returns the next line and parse it into tokens
            bool nextTokens(std::vector<std::string> &, const std::string &c) const;

            Line lastLine() const;

        private:
            ReaderInternal *_imp;
    };
}

#endif
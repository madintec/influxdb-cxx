// MIT License
//
// Copyright (c) 2021 Nicola Foissac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "parser/CSV.h"
#include <sstream>

namespace influxdb::parser {

    CSV::CSV()
    :mDelimiter_(',')
    ,values_()
    {
    }

    CSV::CSV(char delimiter)
    :mDelimiter_(delimiter)
    {
    }


    void CSV::readCSV(const std::string & raw){
        std::stringstream input_;
        input_ << raw ;
        for (std::string line; std::getline(input_, line); ) {
            std::vector<std::string> row;
            std::stringstream row_line_;
            row_line_ << line;
            for (std::string tok; std::getline(row_line_, tok, mDelimiter_);)
            {
                row.push_back(tok);
            }
            this->values_.push_back(row);
        }
    }

    const std::vector<std::vector<std::string>> & CSV::getDocument() const {
        return this->values_;
    }

}
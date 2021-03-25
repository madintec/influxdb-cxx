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


#ifndef INFLUXDB_CXX_CSV_H
#define INFLUXDB_CXX_CSV_H

#include <vector>
#include <chrono>
#include <string>

namespace influxdb::parser {

    class CSV {
        char mDelimiter_;
        //char mQuoteChar_;
        //char mCommentPrefix_;
        std::vector<std::vector<std::string>> values_;

    public:
        CSV();
        explicit CSV(char delimiter);
        void readCSV(const std::string & raw);
        [[nodiscard]] const std::vector<std::vector<std::string>>& getDocument() const;
    };
}

#endif //INFLUXDB_CXX_CSV_H

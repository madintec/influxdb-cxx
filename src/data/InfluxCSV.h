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


#ifndef INFLUXDB_CXX_INFLUXCSV_H
#define INFLUXDB_CXX_INFLUXCSV_H

#include "parser/CSV.h"
#include <map>

namespace influxdb::data {

    struct Record
    {
        /*
        uint32_t mTable_;
        std::chrono::nanoseconds mStart_;
        std::chrono::nanoseconds mStop_;
        std::chrono::nanoseconds mTime_;
        */
        std::string mMeasurement_;
        std::string mFields_;
        double mValue_;
    };

    struct Column {
        Column()
            :mIndex_(0)
            ,mName_()
        {
        }
        //uint8_t mDataType_;
        uint32_t mIndex_ ;
        std::string mName_;
    };

    struct Table
    {
        std::vector<Column> mColumns_;
        std::map<std::chrono::time_point<std::chrono::system_clock>,std::vector<Record>> mRows_;
    };

    class InfluxCSV
    {
        std::vector<Table> mTables_;
        std::map<std::string, std::pair<uint8_t, int>> decode_ = {{"_field", {0, 0}}, {"_measurement", {1, 0}}, {"_value", {2, 0}}, {"_time", {3, 0}}};
    public:
        void parse(const influxdb::parser::CSV & csv);
        const std::vector<Table> & getTables();

    };

}



#endif //INFLUXDB_CXX_INFLUXCSV_H

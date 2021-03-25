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


#include "data/InfluxCSV.h"
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <iomanip>

namespace influxdb::data
{

    const std::vector<Table> & InfluxCSV::getTables()
    {
        return this->mTables_;
    }

    void InfluxCSV::parse(const influxdb::parser::CSV& csv)
    {

        bool is_new_ = false;
        Table current_table_ = {};

        for (auto& row : csv.getDocument())
        {
            if (row.empty())
            {
                //std::cout << " parsing errors";
                continue;
            }

            if (row[0][0] == '#')
            {
                if (row[0].find("#datatype") != std::string::npos)
                {
                    //std::cout << "parse data type \n";
                    if (!is_new_)
                    {
                        for (auto c : row)
                        {
                            current_table_.mColumns_.emplace_back();
                        }
                        is_new_ = true;
                    }
                }
            }
            else
            {
                if (is_new_)
                {
                    int32_t colunm = 0;
                    for (const auto& c : row)
                    {
                        auto header = decode_.find(c);
                        if (header != decode_.end())
                        {

                            header->second.second = colunm;
                        }
                        current_table_.mColumns_[colunm].mName_ = c;
                        current_table_.mColumns_[colunm].mIndex_ = colunm;
                        colunm++;
                    }
                    is_new_ = false;
                }
                else
                {
                    Record record = {};
                    std::chrono::time_point<std::chrono::system_clock> time = {};
                    for (auto& d : decode_)
                    {
                        switch (d.second.first)
                        {
                            case 0:
                                record.mFields_ = row[d.second.second];
                                break;
                            case 1:
                                record.mMeasurement_ = row[d.second.second];
                                break;
                            case 2:
                                record.mValue_ = boost::lexical_cast<double>(row[d.second.second]);
                                break;
                            case 3:
                            {
                                std::stringstream timeString;
                                std::tm tm = {};
                                timeString << row[d.second.second];
                                timeString >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                                time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                            }
                            break;
                            default:
                                break;
                        }
                    }
                    auto find = current_table_.mRows_.find(time);
                    if (find == current_table_.mRows_.end())
                    {
                        find = current_table_.mRows_.emplace(time, std::vector<Record>()).first;
                    }
                    find->second.push_back(record);
                }
            }
        }

        this->mTables_.push_back(current_table_);
    }
}
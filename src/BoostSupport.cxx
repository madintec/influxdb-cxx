// MIT License
//
// Copyright (c) 2021 Nicola Foissac
// Copyright (c) 2020-2021 offa
// Copyright (c) 2019 Adam Wegrzynek
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

#include "BoostSupport.h"
#include "UDP.h"
#include "UnixSocket.h"
#include <chrono>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "parser/CSV.h"
#include "data/InfluxCSV.h"

namespace influxdb::internal
{
    std::vector<Point> queryImpl(Transport* transport, const std::string& query)
    {
        auto response = transport->query(query);
        response.erase(std::remove_if(response.begin(), response.end(),[]( auto const & c ) -> bool { return c == '\r'; }), response.end());
        std::stringstream responseString;
        responseString << response;
        std::vector<Point> points;

        parser::CSV csv;
        csv.readCSV(response);
        data::InfluxCSV influxCsv;
        influxCsv.parse(csv);

        for(const auto& table : influxCsv.getTables()){
            for(const auto& rows : table.mRows_){
                Point p;
                p.setTimestamp(rows.first);
                for(const auto& row : rows.second){
                    p.addField(row.mFields_,row.mValue_);
                }
                points.push_back(p);
            }
        }

        /*
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(responseString, pt);
        std::cout << responseString.str() << "\n";
        for (const auto& result : pt.get_child("results"))
        {
            const auto isResultEmpty = result.second.find("series");
            if (isResultEmpty == result.second.not_found())
            {
                const auto isError = result.second.find("error");
                if(isError != result.second.not_found()){
                    throw BadRequest(__func__, "Bad request: " + isError->second.get_value<std::string>());
                }
                return {};
            }
            for (const auto& series : result.second.get_child("series"))
            {
                const auto columns = series.second.get_child("columns");

                for (const auto& values : series.second.get_child("values"))
                {
                    Point point{series.second.get<std::string>("name", "")};
                    auto iColumns = columns.begin();
                    auto iValues = values.second.begin();
                    for (; iColumns != columns.end() && iValues != values.second.end(); ++iColumns, ++iValues)
                    {
                        const auto value = iValues->second.get_value<std::string>();
                        const auto column = iColumns->second.get_value<std::string>();
                        if (!column.compare("time"))
                        {
                            std::tm tm = {};
                            std::stringstream timeString;
                            timeString << value;
                            timeString >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                            point.setTimestamp(std::chrono::system_clock::from_time_t(std::mktime(&tm)));
                            continue;
                        }
                        // cast all values to double, if strings add to tags
                        try
                        {
                            point.addField(column, boost::lexical_cast<double>(value));
                        }
                        catch (...)
                        {
                            point.addTag(column, value);
                        }
                    }

                    points.push_back(std::move(point));
                }
            }

        }*/
        return points;
    }

    std::unique_ptr<Transport> withUdpTransport(const http::url& uri)
    {
        return std::make_unique<transports::UDP>(uri.host, uri.port);
    }

    std::unique_ptr<Transport> withUnixSocketTransport(const http::url& uri)
    {
        return std::make_unique<transports::UnixSocket>(uri.path);
    }
}

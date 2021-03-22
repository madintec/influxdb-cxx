// MIT License
//
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

namespace influxdb::internal
{
    std::vector<Point> queryImpl(Transport* transport, const std::string& query)
    {
        const auto response = transport->query(query);
        std::stringstream responseString;
        responseString << response;
        std::vector<Point> points;
        size_t _time = 0L;
        size_t  _value = 0L;
        size_t _field = 0L;
        size_t _measurement = 0L;
        auto count = 0L;
        size_t count_row = 0L;
        auto table_count = -1L;
        bool next_row = false;

        for (std::string line; std::getline(responseString, line); ) {

            if(line.at(0) == '#'){
                if(line.find("#datatype") != std::string::npos){
                    _field = 0L;
                    _time = 0L;
                    _value = 0L;
                    _measurement = 0L;
                    count_row = 0L;
                }
                continue;
            }

            if(line.find(",result,") != std::string::npos){
                table_count++;
                std::stringstream header;
                header << line;

                for ( std::string tok; std::getline(header, tok,',');){
                    if(tok.find("_time") != std::string::npos){
                        _time = count_row;
                    }

                    if(tok.find("_value") != std::string::npos){
                        _value = count_row;
                    }

                    if(tok.find("_field") != std::string::npos){
                        _field = count_row;
                    }

                    if(tok.find("_measurement") != std::string::npos){
                        _measurement = count_row;
                    }
                    count_row++;
                }
                continue;
            }

            count_row = 0;
            std::stringstream header;
            header << line;

            std::string fields;
            std::string name;
            double value = {};
            std::tm tm = {};

            for (std::string tok; std::getline(header, tok, ',');)
            {
                if (_measurement == count_row)
                {
                    name = tok;
                }
                if (_field == count_row)
                {
                    fields = tok;
                }

                if (_value == count_row)
                {
                    try
                    {
                        value = boost::lexical_cast<double>(tok);
                    }
                    catch (...)
                    {
                    }
                }

                if (_time == count_row)
                {
                    std::stringstream timeString;
                    timeString << tok;
                    timeString >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                }

                if(count_row == 2){
                    if(table_count != std::stol(tok)){
                        next_row = true;
                        count = 0;
                        table_count++;
                    }
                }
                count_row++;
            }

            if(line.size() == 1){
                continue;
            }

            if(next_row){
                points.at(count).addField(fields,value);
            }
            else{
                Point p;
                p.setName(name);
                p.addField(fields, value);
                p.setTimestamp(std::chrono::system_clock::from_time_t(std::mktime(&tm)));
                points.push_back(p);
            }

            count++;
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

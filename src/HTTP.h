// MIT License
//
// Copyright (c) 2019 Adam Wegrzynek
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

///
/// \author Adam Wegrzynek
///

#ifndef INFLUXDATA_TRANSPORTS_HTTP_H
#define INFLUXDATA_TRANSPORTS_HTTP_H

#include "Transport.h"
#include <curl/curl.h>
#include <memory>
#include <string>
#include <vector>

namespace influxdb::transports
{

/// \brief HTTP transport
class HTTP : public Transport
{
public:
  /// Constructor
  explicit HTTP(const std::string &url);

  /// Default destructor
  ~HTTP() override;

  /// Sends point via HTTP POST
  ///  \throw InfluxDBException	when CURL fails on POSTing or response code != 200
  void send(std::string &&lineprotocol) override;

  /// Queries database
  /// \throw InfluxDBException	when CURL GET fails
  std::string query(const std::string &query) override;

  /// Creates database used at url if it does not exists
  /// \throw InfluxDBException	when CURL POST fails
  void createDatabase() override;

  /// Enable Basic Auth
  /// \param auth <username>:<password>
  void enableBasicAuth(const std::string &auth);

  void setToken(const std::string & token) override;

  void setOrg(const std::string & org) override;

  /// Get the database name managed by this transport
  [[nodiscard]] std::string databaseName() const;

  /// Get the influx service url which transport connects to
  [[nodiscard]] std::string influxDbServiceUrl() const;

private:

  /// Obtain InfluxDB service url from the url passed
  void obtainInfluxServiceUrl(const std::string &url);

  /// Obtain database name from the url passed
  void obtainDatabaseName(const std::string &url);

  void initHttpContext(const std::string & url);


  /// \throw InfluxDBException	if database (?db=) not specified
  void initCurl();


  /// treats responses of CURL requests
  void treatCurlResponse(const CURLcode &response, long responseCode) const;


  std::tuple<std::string, const CURLcode, long> post(const std::string & args);
  std::tuple<std::string, const CURLcode, long> get(const std::string & args);


  CURL * postHandle;
  CURL * getHandle;

  /// InfluxDB base url
  std::string mUrl;

  /// InfluxDB service URL
  std::string mInfluxDbServiceUrl;

  /// Database name used
  std::string mDatabaseName;

  /// database token
  std::string mToken;

  /// database organisation
  std::string mOrg;
};

} // namespace influxdb

#endif // INFLUXDATA_TRANSPORTS_HTTP_H

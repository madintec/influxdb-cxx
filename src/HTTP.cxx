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
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///


#include <iostream>
#include "HTTP.h"
#include "InfluxDBException.h"


namespace influxdb::transports
{
    namespace
    {
        size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
        {
            static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
            return size * nmemb;
        }

        /*
        size_t noopWriteCallBack([[maybe_unused]] char* ptr, size_t size,
                                 size_t nmemb, [[maybe_unused]] void* userdata)
        {
            return size * nmemb;
        }
         */

        void setConnectionOptions(CURL* handle)
        {
            curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10);
            curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10);
            curl_easy_setopt(handle, CURLOPT_TCP_KEEPIDLE, 120L);
            curl_easy_setopt(handle, CURLOPT_TCP_KEEPINTVL, 60L);
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        }

        CURL* createWriteHandle(const std::string& url)
        {
            if (CURL* writeHandle = curl_easy_init(); writeHandle != nullptr)
            {
                setConnectionOptions(writeHandle);
                curl_easy_setopt(writeHandle, CURLOPT_URL, url.c_str());
                curl_easy_setopt(writeHandle, CURLOPT_POST, 1);
                return writeHandle;
            }

            throw InfluxDBException{__func__, "Failed to initialize write handle"};
        }

        CURL * createPost() noexcept(false) {
            if( CURL * post = curl_easy_init(); post != nullptr){
                setConnectionOptions(post);
                curl_easy_setopt(post,CURLOPT_POST, 1);
                return post;
            }
            throw InfluxDBException{__func__,"Failed to initialize curl post context."};
        }

        CURL * createGet() noexcept(false) {
            if( CURL * get = curl_easy_init(); get != nullptr){
                setConnectionOptions(get);
                return get;
            }
            throw InfluxDBException{__func__,"Failed to initialize curl get context."};
        }


        struct curl_slist * createHeader(const std::vector<std::string> & options){
            struct curl_slist * headers = nullptr;
            for(auto & option: options){
                headers = curl_slist_append(headers,option.c_str());
            }
            return headers;
        }

    }

HTTP::HTTP(const std::string &url)
{
  initCurl();
  initHttpContext(url);

  obtainInfluxServiceUrl(url);
  obtainDatabaseName(url);

}

HTTP::~HTTP()
{
  curl_easy_cleanup(postHandle);
  curl_easy_cleanup(getHandle);
  curl_global_cleanup();
}

void HTTP::initHttpContext(const std::string & url)
{

   auto pos = url.find("?db=");

   if(pos == std::string::npos){
       throw InfluxDBException(__func__, "Database not specified");
   }

   mDatabaseName = url.substr(pos + 4);

   mUrl = url.substr(0,pos);

   if(mUrl.at(pos-1)!='/'){
       mUrl.push_back('/');
   }
}

void HTTP::initCurl()
{
  if (const CURLcode globalInitResult = curl_global_init(CURL_GLOBAL_ALL); globalInitResult != CURLE_OK)
  {
    throw InfluxDBException(__func__, curl_easy_strerror(globalInitResult));
  }
    postHandle = createPost();
    getHandle = createGet();
}


std::string HTTP::query(const std::string &query)
{
    auto * header = createHeader({"Authorization: Token "+mToken,"Accept: application/csv","Content-type: application/vnd.flux"});
    curl_easy_setopt(postHandle,CURLOPT_HTTPHEADER,header);
    char* encodedArgs = curl_easy_escape(postHandle, mOrg.c_str(), static_cast<int>(mOrg.size()));
    auto fullUrl = mUrl+"api/v2/query?org="+std::string(encodedArgs);
    curl_easy_setopt(postHandle,CURLOPT_URL,fullUrl.c_str());
    auto[buffer,response, code] = this->post(query);
    curl_free(encodedArgs);
    curl_slist_free_all(header);
    treatCurlResponse(response, code);
    return buffer;
}

void HTTP::enableBasicAuth(const std::string &auth)
{
  curl_easy_setopt(getHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  curl_easy_setopt(getHandle, CURLOPT_USERPWD, auth.c_str());
  /*
  curl_easy_setopt(postHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  curl_easy_setopt(postHandle, CURLOPT_USERPWD, auth.c_str());
   */
}

void HTTP::send(std::string &&lineprotocol)
{
  auto fullUrl = mUrl+"write?db="+mDatabaseName;
  curl_easy_setopt(postHandle,CURLOPT_URL,fullUrl.c_str());
  auto[buffer, response, code] = this->post(lineprotocol);
  treatCurlResponse(response, code);
  buffer.clear();
}

void HTTP::treatCurlResponse(const CURLcode &response, long responseCode) const
{
  if (response != CURLE_OK)
  {
    throw ConnectionError(__func__, curl_easy_strerror(response));
  }
  //
  // Influx API response codes:
  // https://docs.influxdata.com/influxdb/v1.7/tools/api/#status-codes-and-responses-2
  //
  if (responseCode == 404)
  {
    throw NonExistentDatabase(__func__, "Nonexistent database: " + std::to_string(responseCode));
  }
  else if ((responseCode >= 400) && (responseCode < 500))
  {
    throw BadRequest(__func__, "Bad request: " + std::to_string(responseCode));
  }
  else if (responseCode >= 500)
  {
    throw ServerError(__func__, "Influx server error: " + std::to_string(responseCode));
  }
}

void HTTP::obtainInfluxServiceUrl(const std::string &url)
{
  const auto questionMarkPosition = url.find('?');

  if (url.at(questionMarkPosition - 1) == '/')
  {
    mInfluxDbServiceUrl = url.substr(0, questionMarkPosition-1);
  }
  else
  {
    mInfluxDbServiceUrl = url.substr(0, questionMarkPosition);
  }
}

void HTTP::obtainDatabaseName(const std::string &url)
{
  const auto dbParameterPosition = url.find("db=");
  mDatabaseName = url.substr(dbParameterPosition + 3);
}

std::string HTTP::databaseName() const
{
  return mDatabaseName;
}

std::string HTTP::influxDbServiceUrl() const
{
  return mInfluxDbServiceUrl;
}

std::tuple<std::string, const CURLcode, long> HTTP::get(const std::string& args)
{
  long responseCode;
  std::string buffer;
  char* encodedArgs = curl_easy_escape(getHandle, args.c_str(), static_cast<int>(args.size()));
  auto fullUrl = mUrl+"query?db="+mDatabaseName+"&q="+std::string(encodedArgs);
  curl_free(encodedArgs);
  curl_easy_setopt(getHandle, CURLOPT_URL, fullUrl.c_str());
  curl_easy_setopt(getHandle, CURLOPT_WRITEDATA, &buffer);
  const CURLcode response = curl_easy_perform(getHandle);
  curl_easy_getinfo(getHandle, CURLINFO_RESPONSE_CODE, &responseCode);
  return {buffer,response,responseCode};
}

std::tuple<std::string, const CURLcode, long> HTTP::post(const std::string& args)
{
  long responseCode;
  std::string buffer;
  curl_easy_setopt(postHandle, CURLOPT_POSTFIELDS, args.c_str());
  curl_easy_setopt(postHandle, CURLOPT_POSTFIELDSIZE, static_cast<long>(args.length()));
  curl_easy_setopt(postHandle, CURLOPT_WRITEDATA, &buffer);
  const CURLcode response = curl_easy_perform(postHandle);
  curl_easy_getinfo(postHandle, CURLINFO_RESPONSE_CODE, &responseCode);
  return {buffer,response,responseCode};
}



void HTTP::createDatabase()
{
  const std::string createUrl = mInfluxDbServiceUrl + "/query";
  const std::string postFields = "q=CREATE DATABASE " + mDatabaseName;

  CURL* createHandle = createWriteHandle(createUrl);

  curl_easy_setopt(createHandle, CURLOPT_POSTFIELDS, postFields.c_str());
  curl_easy_setopt(createHandle, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));

  const CURLcode response = curl_easy_perform(createHandle);
  long responseCode;
  curl_easy_getinfo(createHandle, CURLINFO_RESPONSE_CODE, &responseCode);
  treatCurlResponse(response,responseCode);
  curl_easy_cleanup(createHandle);
}

void HTTP::setToken(const std::string& token)
{
    this->mToken = token;
}

void HTTP::setOrg(const std::string& org)
{
    this->mOrg = org;
}

} // namespace influxdb

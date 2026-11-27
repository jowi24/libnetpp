/*
 * libnet++
 *
 * Copyright (C) 2007-2013 Joachim Wilke <libnet@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "HttpClient.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <mutex>

#include <liblog++/Log.h>
#include <libconv++/CharsetConverter.h>

namespace network {

namespace {
std::once_flag curlInitFlag;

void ensureCurlInitialized() {
    std::call_once(curlInitFlag, []() {
        CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (code != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(code));
    });
}

size_t appendCurlData(char *data, size_t size, size_t count, void *userData) {
    const size_t bytes = size * count;
    static_cast<std::string *>(userData)->append(data, bytes);
    return bytes;
}
}


HttpClient::HttpClient(const std::string &host, int port)
: TcpClient{host, port} {
}

HttpClient::~HttpClient() {
}

HttpClient::response_t HttpClient::parseResponse() {
	header_t header;
	body_t body;

    std::string http_version;
    *stream >> http_version;
    unsigned int status_code;
    *stream >> status_code;
    std::string status_message;
    std::getline(*stream, status_message);
    if (!(*stream) || http_version.substr(0, 5) != "HTTP/")
      throw std::runtime_error("Invalid response");
    DBG("HTTP status code " << status_code);

    // Process the response headers, which are terminated by a blank line
    std::string headerline;
    while (std::getline(*stream, headerline) && headerline != "\r") {
    	size_t separator = headerline.find(':');
    	if (separator == std::string::npos)
    		throw std::runtime_error("Invalid header format detected in HTTP response.");
    	std::string key = headerline.substr(0, separator);
    	std::string value = headerline.substr(separator+2);
    	header.insert(std::pair<std::string, std::string>(key, value));
    	DBG("Found header: " << key << ": " << value);
    }

    // The remaining data is the body
    std::stringstream bodystream;
    bodystream << stream->rdbuf();
    body = bodystream.str();
	DBG("Body size " << body.length() << " Bytes.");

	// check encoding and convert
	const std::string charsetToken = "charset=";
	const std::string& contentType = header["Content-Type"];
	if (contentType.length()) {
		size_t start = contentType.find(charsetToken);
		if (start != std::string::npos) {
			start += charsetToken.length();
			size_t stop = contentType.find('\r', start);
			if (stop == std::string::npos)
				stop = contentType.find('\n', start);
			std::string charset = std::string(contentType, start, stop-start);
			DBG("Converting response from charset " << charset << " to local encoding.");
			body = convert::CharsetConverter::ConvertToLocalEncoding(body, charset);
		}
	}

	return response_t(header, body);
}

std::string HttpClient::sendRequest(const std::string &request, const std::ostream &postdata, const header_t &header) {
	if (!connected)
		connectStream();
	std::stringstream post;
	post << postdata.rdbuf();
	int postContentLength = post.str().length();
	std::string method = postContentLength ? "POST" : "GET";

	DBG("Requesting HTTP " << method << " on " << request);
	*stream << method << " " << request << " HTTP/1.0\r\n";
	for (auto entry : defaultHeader) {
		*stream << entry.first << ": " << entry.second << "\r\n";
	}
	for (auto entry : header) {
		*stream << entry.first << ": " << entry.second << "\r\n";
	}
	if (postContentLength)
		*stream << "Content-Length: " << postContentLength << "\r\n"
		       << "\r\n" << post.str() << "\r\n" << std::flush;
	else
		*stream << "\r\n" << std::flush;

	response_t response = parseResponse();
	disconnectStream();

	// check for redirection
	header_t responseHeader = response.first;
	if (responseHeader["Location"].length() > 0) {
		DBG("Redirect requested to " << responseHeader["Location"]);
		return getURL(responseHeader["Location"]);
	}
	return response.second;
}

std::string HttpClient::get(const std::string& url, const param_t &params, const header_t &header) {
	std::stringstream ss;
	if (url.find('?') == std::string::npos)
		ss << url << "?";
	else
		ss << url << "&";
	for (auto parameter: params)
		ss << parameter.first << "=" << parameter.second << "&";
	return sendRequest(ss.str(), std::ostringstream(), header);
}

std::string HttpClient::post(const std::string &request, const param_t &postdata, const header_t &header) {
	header_t fullheader = {
			{ "Content-Type", "application/x-www-form-urlencoded" }
	};
	fullheader.insert(begin(header), end(header));

	std::stringstream ss;
	for (auto parameter : postdata)
			ss << parameter.first << "=" << parameter.second << "&";

	return sendRequest(request, ss, fullheader);
}

std::string HttpClient::getURL(const std::string &url, const header_t &header) {
    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("Unable to initialize libcurl");

    std::string body;
    struct curl_slist *headers = nullptr;
    for (const auto &entry : header) {
        const std::string line = entry.first + ": " + entry.second;
        struct curl_slist *next = curl_slist_append(headers, line.c_str());
        if (!next) {
            if (headers)
                curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("Unable to allocate HTTP headers");
        }
        headers = next;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendCurlData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; vdr-fritz)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // TLS certificate chain and hostname validation are deliberately enabled.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Request compressed content; libcurl transparently decompresses it.
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode code = curl_easy_perform(curl);
    long status = 0;
    char *contentType = nullptr;
    if (code == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
    }

    std::string charset;
    if (contentType) {
        std::string type(contentType);
        std::string::size_type pos = type.find("charset=");
        if (pos != std::string::npos) {
            pos += 8;
            std::string::size_type stop = type.find_first_of("; \t\r\n", pos);
            charset = type.substr(pos, stop == std::string::npos ? stop : stop - pos);
            if (charset.size() >= 2 &&
                ((charset.front() == '"' && charset.back() == '"') ||
                 (charset.front() == '\'' && charset.back() == '\'')))
                charset = charset.substr(1, charset.size() - 2);
        }
    }

    if (headers)
        curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
        throw std::runtime_error(std::string("HTTP request failed: ") + curl_easy_strerror(code));
    if (status < 200 || status >= 300)
        throw std::runtime_error("HTTP request returned status " + std::to_string(status));

    if (!charset.empty())
        body = convert::CharsetConverter::ConvertToLocalEncoding(body, charset);

    return body;
}

std::string HttpClient::postMIME(const std::string &request, const param_t &postdata, const header_t &header) {

	const std::string boundary = "----FormBoundaryZMsGfL5JxTz5LuAW";
	header_t fullheader = {
			{ "Content-Type", "multipart/form-data; boundary=" + boundary }
	};
	fullheader.insert(begin(header), end(header));

	std::stringstream ss;
	for (auto parameter : postdata) {
		ss << "--" << boundary << "\r\n"
		   << "Content-Disposition: form-data; name=\""+parameter.first+"\"\r\n"
		   << "\r\n"
		   << parameter.second
		   << "\r\n";
	}
	ss << "--" << boundary << "--\r\n";
	return sendRequest(request, ss, fullheader);
}

}

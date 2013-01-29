/*
 * HttpClient.cpp
 *
 *  Created on: 18.01.2013
 *      Author: jo
 */


#include "gtest/gtest.h"

#include <libnet++/HttpClient.h>

namespace test {

class HttpClient : public ::testing::Test  {
public:

	HttpClient()
	{};

	network::HttpClient *client;
	const network::HttpClient::header_t defaultHeader {
		{ "Header1", "headervalue1" }
	};

	void SetUp() {
		std::string host("httpbin.org");
		client = new network::HttpClient(host);
	}
};

TEST_F(HttpClient, HttpGetRequest) {
	std::string responseBody1 = client->get("/get?param1=value1&param2=value2");
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestSeparatedParams) {
	std::string responseBody1 = client->get("/get", {{"param1", "value1"},{"param2","value2"}});
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestCombinedParams) {
	std::string responseBody1 = client->get("/get?param1=value1", {{"param2","value2"}});
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestTwice) {
	std::string responseBody1 = client->get("/get?param1=value1&param2=value2");
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
	std::string responseBody2 = client->get("/get?param3=value3&param4=value4");
	ASSERT_TRUE(responseBody2.find("\"param3\": \"value3\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"param4\": \"value4\"") != std::string::npos);
}

TEST_F(HttpClient, HttpPostRequest) {
	network::HttpClient::param_t params =
	{
	  { "param1", "value1" },
	  { "param2", "value2" }
	};
	std::string responseBody1 = client->post("/post", params);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
}

TEST_F(HttpClient, HttpPostRequestTwice) {
	network::HttpClient::param_t params1 =
	{
	  { "param1", "value1" },
	  { "param2", "value2" }
	};
	std::string responseBody1 = client->post("/post", params1);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
	network::HttpClient::param_t params2 =
	{
	  { "param3", "value3" },
	  { "param4", "value4" }
	};
	std::string responseBody2 = client->post("/post", params2);
	ASSERT_TRUE(responseBody2.find("\"param3\": \"value3\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"param4\": \"value4\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestWithHeader) {
	std::string responseBody = client->get("/get?param1=value1&param2=value2", network::HttpClient::param_t(), defaultHeader);
	ASSERT_TRUE(responseBody.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody.find("\"param2\": \"value2\"") != std::string::npos);
	ASSERT_TRUE(responseBody.find("\"Header1\": \"headervalue1\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestTwiceWithHeader) {
	std::string responseBody1 = client->get("/get?param1=value1&param2=value2", network::HttpClient::param_t(), defaultHeader);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"Header1\": \"headervalue1\"") != std::string::npos);
	std::string responseBody2 = client->get("/get?param3=value3&param4=value4", network::HttpClient::param_t(), defaultHeader);
	ASSERT_TRUE(responseBody2.find("\"param3\": \"value3\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"param4\": \"value4\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"Header1\": \"headervalue1\"") != std::string::npos);
}

TEST_F(HttpClient, HttpPostRequestWithHeader) {
	network::HttpClient::param_t params =
	{
	  { "param1", "value1" },
	  { "param2", "value2" }
	};
	std::string responseBody1 = client->post("/post", params, defaultHeader);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"Header1\": \"headervalue1\"") != std::string::npos);
}

TEST_F(HttpClient, HttpPostRequestTwiceWithHeader) {
	network::HttpClient::param_t params1 =
	{
	  { "param1", "value1" },
	  { "param2", "value2" }
	};
	std::string responseBody1 = client->post("/post", params1, defaultHeader);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"Header1\": \"headervalue1\"") != std::string::npos);

	network::HttpClient::param_t params2 =
	{
	  { "param3", "value3" },
	  { "param4", "value4" }
	};
	std::string responseBody2 = client->post("/post", params2, defaultHeader);
	ASSERT_TRUE(responseBody2.find("\"param3\": \"value3\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"param4\": \"value4\"") != std::string::npos);
	ASSERT_TRUE(responseBody2.find("\"Header1\": \"headervalue1\"") != std::string::npos);
}

TEST_F(HttpClient, HttpGetRequestLargeResponse) {
	std::string responseBody1 = client->get("/stream/100");
	ASSERT_TRUE(responseBody1.find("\"id\": 99") != std::string::npos);
}


TEST_F(HttpClient, HttpPostMimeRequest) {
	network::HttpClient::param_t params =
	{
	  { "param1", "value1" },
	  { "param2", "value2" }
	};
	std::string responseBody1 = client->postMIME("/post", params);
	ASSERT_TRUE(responseBody1.find("\"param1\": \"value1\"") != std::string::npos);
	ASSERT_TRUE(responseBody1.find("\"param2\": \"value2\"") != std::string::npos);
}



}



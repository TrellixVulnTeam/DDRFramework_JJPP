#include <functional>
#include "../../Shared/src/Utility/Logger.h"
#include "../../Shared/src/Utility/DDRMacro.h"
#include "HttpClient.h"
#include <curl/curl.h>
#include "../../Shared/thirdparty/curl/include/curl/easy.h"
#include <sstream>
#include <iostream>
namespace DDRFramework
{

	HttpSession::HttpSession():m_spWork(std::make_shared<asio::io_context::work>(m_ContextIO))
	{
	}

	HttpSession::~HttpSession()
	{
		DebugLog("HttpSession Destroy")
	}



	curl_asio::data_action::type HttpSession::on_transfer_data_read(std::ofstream &out, const asio::const_buffer& buffer)
	{
		try
		{
			out.write(asio::buffer_cast<const char*>(buffer), asio::buffer_size(buffer));
			return curl_asio::data_action::success;
		}
		catch (std::exception& e)
		{
			DebugLog("%s", e.what());
		}
		return curl_asio::data_action::abort;
	}

	void HttpSession::on_transfer_done(curl_asio::transfer::ptr transfer, std::ofstream &out, const std::string &file, CURLcode result)
	{
		if (result == CURLE_OK)
		{
			out.close();

			DebugLog("Transfer of %s  completed successfully ( %d  seconds)! Content saved to file: %s ", transfer->info().effective_url().c_str(), transfer->info().total_time(), file.c_str());
		}
		else
		{
			DebugLog("Transfer of %s  failed with error %i", transfer->info().effective_url().c_str(), result);
		}
		transfer->on_data_read = nullptr;
		transfer->on_done = nullptr;
		transfer.reset();
		m_spWork.reset();
	}

	size_t write_file(void *ptr, size_t size, size_t nmemb, void *pData) {

		string data((const char*)ptr, (size_t)size * nmemb);

		auto pSession = ((HttpSession*)pData);
		pSession->WriteFileStream(ptr, size * nmemb);

		return size * nmemb;
	}
	void HttpSession::ThreadEntry(std::string url, std::string outfile)
	{
		m_pCurl = curl_easy_init();
	
		curl_easy_setopt(m_pCurl, CURLOPT_URL, url.c_str());
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1); //Prevent "longjmp causes uninitialized stack frame" bug
		curl_easy_setopt(m_pCurl, CURLOPT_ACCEPT_ENCODING, "deflate");

		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, write_file);

		m_OutFileStream.open(outfile, std::ios::binary);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);
		/* Perform the request, res will get the return code */
		CURLcode res = curl_easy_perform(m_pCurl);
		/* Check for errors */
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		}
		m_OutFileStream.close();

		curl_easy_cleanup(m_pCurl);
		DebugLog("HttpSession Entry Finish")

		if (m_OnDoneFunc)
		{
			m_OnDoneFunc(0);
		}
	}

	void HttpSession::DoGet(std::string& url, std::string outfile)
	{
		auto func = std::bind(&HttpSession::ThreadEntry, shared_from_this(), url, outfile);
		std::thread t(func);
		t.detach();
	}
	void HttpSession::DoPost()
	{
		DebugLog("Post");
	}

}
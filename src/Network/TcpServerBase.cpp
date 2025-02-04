#include "TcpServerBase.h"
#include "../Utility/Logger.h"
#include "../Network/BaseMessageDispatcher.h"
#include "../Network/MessageSerializer.h"
namespace DDRFramework
{

	TcpSessionBase::TcpSessionBase(asio::io_context& context):TcpSocketContainer::TcpSocketContainer(context)
	{

		std::thread::id tid = std::this_thread::get_id();
		uint64_t* ptr = (uint64_t*)&tid;
		LevelLog(DDRFramework::Log::Level::INFO,"TcpSessionBase Create On Thread: %d", (*ptr))
		m_bConnected = true;
	}
	TcpSessionBase::~TcpSessionBase()
	{

		LevelLog(DDRFramework::Log::Level::INFO,"TcpSessionBase Destroy");
	}

	void TcpSessionBase::OnStart()
	{	

		std::thread::id tid = std::this_thread::get_id();
		uint64_t* ptr = (uint64_t*)&tid;

		LevelLog(DDRFramework::Log::Level::INFO, "Connection Established! %s : On Thread %d", m_Socket.remote_endpoint().address().to_string().c_str(), (*ptr));
		if (m_bConnected)
		{

			if (m_fOnSessionConnected)
			{
				m_fOnSessionConnected(shared_from_this());
			}

			m_IOContext.post(std::bind(&TcpSessionBase::StartRead, shared_from_base()));
		}
	}


	void TcpSessionBase::StartRead()
	{
		asio::async_read(m_Socket, m_ReadStreamBuf,asio::transfer_at_least(1),std::bind(&TcpSessionBase::HandleRead, shared_from_base(),std::placeholders::_1));

	}
	void TcpSessionBase::HandleRead(const asio::error_code& ec)
	{
		if (!ec)
		{
			if (m_bConnected)
			{
				PushData(m_ReadStreamBuf);
				m_ReadWriteStrand.post(std::bind(&TcpSessionBase::StartRead, shared_from_base()));
			}
		}
		else
		{
			Stop();

			LevelLog(DDRFramework::Log::Level::ERR,"TcpSessionBase Error on receive: %s", ec.message().c_str());
		}

	}
	void TcpSessionBase::StartWrite(std::shared_ptr<asio::streambuf> spbuf)
	{
		if (m_Socket.is_open())
		{
			if (spbuf && spbuf->size() > 0)
			{
				m_CurrentWritingBuf = spbuf;
				asio::async_write(m_Socket, *m_CurrentWritingBuf.get(), std::bind(&TcpSessionBase::HandleWrite, shared_from_base(), std::placeholders::_1, std::placeholders::_2));
			}
			else
			{
				LevelLog(DDRFramework::Log::Level::WARNING,"TcpSessionBase StartWrite Size 0")
			}


		}

	}

	TcpServerBase::TcpServerBase(int port) :m_Acceptor(m_IOContext, tcp::endpoint(tcp::v4(), port))
	{
		LevelLog(DDRFramework::Log::Level::INFO,"TcpServerBase  Create");
	}

	TcpServerBase::~TcpServerBase()
	{
		LevelLog(DDRFramework::Log::Level::INFO,"TcpServerBase  Destroy");
	}


	void TcpServerBase::Start(int threadNum)
	{
		m_WorkerThreads.create_threads(std::bind(&TcpServerBase::ThreadEntry, shared_from_this()), threadNum);
	}
	void TcpServerBase::Stop()
	{
		m_Acceptor.close();
		std::vector<std::shared_ptr<TcpSessionBase>> vec;

		for (auto spSessionPair : m_SessionMap)
		{
			auto spSession = spSessionPair.second;
			vec.push_back(spSession);
		}//donot clear map heae , session stop will call ondisconncet callback ,and session ptr will be remove in ondisconnect function

		for (auto spSession : vec)
		{
			spSession->Stop();
		}


		std::vector<std::shared_ptr<TcpSessionBase>> vecWaitingAccept;
		for (auto spSession : m_WaitingAcceptSessionSet)
		{
			vecWaitingAccept.push_back(spSession);
		}
		for (auto spSession : vecWaitingAccept)
		{
			spSession->Stop();
		}

	}

	void TcpServerBase::ThreadEntry()
	{
		StartAccept();
		m_IOContext.run(); 
	}


	std::shared_ptr<TcpSessionBase> TcpServerBase::BindSerializerDispatcher()
	{
		BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, TcpSessionBase, MessageSerializer, BaseMessageDispatcher,BaseHeadRuleRouter)
			return spTcpSessionBase;
	}
	std::shared_ptr<TcpSessionBase> TcpServerBase::StartAccept()
	{
		auto spSession = BindSerializerDispatcher();
		spSession->BindOnDisconnect(std::bind(&TcpServerBase::OnSessionDisconnect, shared_from_this(), std::placeholders::_1));
		m_Acceptor.async_accept(spSession->GetSocket(),
			std::bind(&TcpServerBase::HandleAccept, this, spSession, std::placeholders::_1));

		m_WaitingAcceptSessionSet.insert(spSession);

		return spSession;

	}
	void TcpServerBase::HandleAccept(std::shared_ptr<TcpSessionBase> spSession, const asio::error_code& error)
	{
		if (!error)
		{ 
			if (m_WaitingAcceptSessionSet.find(spSession) != m_WaitingAcceptSessionSet.end())
			{
				m_WaitingAcceptSessionSet.erase(spSession);
			}

			if (m_SessionMap.find(&spSession->GetSocket()) == m_SessionMap.end())
			{
				m_SessionMap[&spSession->GetSocket()] = spSession;
				spSession->Start();

			}
			else
			{
				/*m_SessionMap[spSession->GetSocket()]->Stop();
				std::thread t(bind(&TcpServerBase::WaitUntilPreSessionDestroy, shared_from_this(), ip, spSession));
				t.detach();*/


			}

			//only not error ,to start next accept
			StartAccept();
		}
		else
		{
			spSession->Stop();
		}

	}
	//void TcpServerBase::WaitUntilPreSessionDestroy(tcp::socket& socket, std::shared_ptr<TcpSessionBase> spSession)
	//{
	//	while (m_SessionMap.find(spSession->GetSocket()) != m_SessionMap.end())
	//	{
	//		std::this_thread::sleep_for(std::chrono::seconds(1));
	//	}

	//	m_SessionMap[socket] = spSession;
	//	spSession->Start();
	//}


	void TcpServerBase::OnSessionDisconnect(std::shared_ptr<TcpSocketContainer> spContainer)
	{
		std::lock_guard<std::mutex> lock(m_mapMutex);
		try
		{
			asio::ip::tcp::socket& sock = spContainer->GetSocket();
			asio::ip::tcp::socket* psock = &sock;
			if (m_SessionMap.find(psock) != m_SessionMap.end())
			{
				auto sp = m_SessionMap[psock];
				//erase first then release and reset  , cause somewhere may call map to get session,it will get a empty sp if reset first
				m_SessionMap.erase(psock);
				sp->Release();
				sp.reset();
			}

			auto spServerSession = dynamic_pointer_cast<TcpSessionBase>(spContainer);
			if (spServerSession)
			{
				if (m_WaitingAcceptSessionSet.find(spServerSession) != m_WaitingAcceptSessionSet.end())
				{
					m_WaitingAcceptSessionSet.erase(spServerSession);
					spServerSession->Release();
					spServerSession.reset();
				}

			}

		}
		catch (asio::error_code& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"Disconnect Error %s", e.message().c_str())
		}
		catch (asio::system_error& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"Disconnect Error %s", e.what())
		}
		catch (std::exception& e)
		{

			LevelLog(DDRFramework::Log::Level::ERR,"Disconnect Error %s", e.what())
		}
	}

	std::shared_ptr<TcpSessionBase> TcpServerBase::GetTcpSessionBySocket(tcp::socket* pSocket)
	{
		if (m_SessionMap.find(pSocket) != m_SessionMap.end())
		{
			return m_SessionMap[pSocket];
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<DDRFramework::TcpSessionBase>> TcpServerBase::GetConnectedSessions()
	{
		std::lock_guard<std::mutex> lock(m_mapMutex);
		std::vector<std::shared_ptr<TcpSessionBase>> vec;
		for (auto spPair : m_SessionMap)
		{
			vec.push_back(spPair.second);
		}
		return vec;
	}

	int TcpServerBase::SendToAll(std::shared_ptr<asio::streambuf> spbuf)
	{
		int i = 0;
		std::lock_guard<std::mutex> lock(m_mapMutex);
		for (auto spPair : m_SessionMap)
		{
			auto spSession = spPair.second;
			if (spSession)
			{
				spSession->Send(spbuf);
				i++;
			}
		}
		return i;
	}

	std::map<tcp::socket*, std::shared_ptr<TcpSessionBase>>& TcpServerBase::GetTcpSocketContainerMap()
	{

		return m_SessionMap;
	}












	//-------------------------------------------------------------------------------------------------------------------------------------------------






	HookTcpSession::HookTcpSession(asio::io_context& context) : DDRFramework::TcpSessionBase(context)
	{
		SetRealtime(true);
	}
	HookTcpSession::~HookTcpSession()
	{
		LevelLog(DDRFramework::Log::Level::INFO,"HookTcpSession Destroy")
	}
	void HookTcpSession::StartRead()
	{
		asio::async_read(m_Socket, m_ReadStreamBuf, asio::transfer_at_least(1), std::bind(&HookTcpSession::HandleRead, shared_from_base(), std::placeholders::_1));

	}
	void HookTcpSession::HandleRead(const asio::error_code& ec)
	{
		if (!ec)
		{
			if (m_bConnected)
			{
				OnHookReceive(m_ReadStreamBuf);
				m_ReadStreamBuf.consume(m_ReadStreamBuf.size());
				m_ReadWriteStrand.post(std::bind(&HookTcpSession::StartRead, shared_from_base()));
			}
		}
		else
		{
			Stop();
			LevelLog(DDRFramework::Log::Level::ERR,"HookTcpSession Error on receive:  %s",  ec.message().c_str());
		}
	}




	HookTcpServer::HookTcpServer(int port) :TcpServerBase(port)
	{
	}


	HookTcpServer::~HookTcpServer()
	{
		LevelLog(DDRFramework::Log::Level::INFO,"HookTcpServer Destroy")
	}


	std::shared_ptr<TcpSessionBase> HookTcpServer::BindSerializerDispatcher()
	{
		BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, HookTcpSession, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
			return spHookTcpSession;
	}

}
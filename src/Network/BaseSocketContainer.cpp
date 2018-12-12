#include "BaseSocketContainer.h"
#include "../Utility/DDRMacro.h"

namespace DDRFramework
{

	BaseSocketContainer::BaseSocketContainer(std::shared_ptr<TcpSocketContainer> sp)
	{
		SetTcp(sp);
	}
	BaseSocketContainer::BaseSocketContainer(std::shared_ptr<UdpSocketBase> sp)
	{
		SetUdp(sp);
	}
	BaseSocketContainer::~BaseSocketContainer()
	{
		if (m_spTcpSocketContainer)
		{
			m_spTcpSocketContainer.reset();
		}

		if (m_spUdpSocketBase)
		{
			m_spUdpSocketBase.reset();
		}
		DebugLog("\nBaseSocketContainer Destroy");
	}

	void BaseSocketContainer::SetTcp(std::shared_ptr<TcpSocketContainer> sp)
	{
		m_spTcpSocketContainer = sp;
		m_SocketType = ESOCKETTYPE::EST_TCP;
	}
	void BaseSocketContainer::SetUdp(std::shared_ptr<UdpSocketBase> sp)
	{
		m_spUdpSocketBase = sp;
		m_SocketType = ESOCKETTYPE::EST_UDP;
	}
	asio::io_context& BaseSocketContainer::GetIOContext()
	{
		if (m_SocketType == EST_TCP)
		{
			return m_spTcpSocketContainer->GetIOContext();
		}
		else if (m_SocketType == EST_UDP)
		{
			return m_spUdpSocketBase->GetIOContext();
		}
	}

	void BaseSocketContainer::Send(std::shared_ptr<google::protobuf::Message> spmsg)
	{
		if (m_SocketType == EST_TCP)
		{
			m_spTcpSocketContainer->Send(spmsg);
		}
		else if (m_SocketType == EST_UDP)
		{
			m_spUdpSocketBase->Send(spmsg);
		}

	}
}
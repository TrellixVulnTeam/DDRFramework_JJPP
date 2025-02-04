/*!
 * File: BaseMessageDispatcher.h
 * Date: 2019/04/29 15:59
 *
 * Author: michael
 * Contact: michael2345@live.cn
 *
 * Description:Dispatch msg by it type
 *
*/
#ifndef BaseMessageDispatcher_h__
#define BaseMessageDispatcher_h__


#include <map>
#include <memory.h>

#include "BaseProcessor.h"
#include "src/Utility/LoggerDef.h"
#include "../../proto/RemoteCmd.pb.h"
#include "BaseSocketContainer.h"



#define RegisterProcessor(action,name) action##name action##name;\
m_ProcessorMap[action##name.GetTypeName()] = std::make_shared<name##Processor>(*this);


namespace DDRFramework
{
	class TcpSocketContainer;
	class BaseProcessor;
	class  BaseMessageDispatcher : public std::enable_shared_from_this<BaseMessageDispatcher>
	{
	public:
		BaseMessageDispatcher();
		~BaseMessageDispatcher();

		virtual void Dispatch(std::shared_ptr<BaseSocketContainer> spParentSocketContainer,std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg);

		virtual void Hook(std::shared_ptr<BaseSocketContainer> spParentSocketContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr <google::protobuf::Message> spMsg);

		bool RegisterExternalProcessor(google::protobuf::Message& msg, std::shared_ptr<BaseProcessor> sp);
		bool UnregisterExternalProcessor(google::protobuf::Message& msg);

		
		void AsyncThreadEntry(std::shared_ptr<BaseProcessor> spProcessor, std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg);
		void EraseThread(std::shared_ptr<std::thread> spThread);
	protected:


		std::map<std::string, std::shared_ptr<BaseProcessor>> m_ProcessorMap;
		//std::set<std::shared_ptr<std::thread>> m_AsyncThreadSet;
	};

	class BaseUdpMessageDispatcher : public DDRFramework::BaseMessageDispatcher
	{
	public:
		BaseUdpMessageDispatcher();
		~BaseUdpMessageDispatcher();

		template<class TMsg,class TProcessor>
		void AddProcessor()
		{
			TMsg tmsg;
			m_ProcessorMap[tmsg.GetTypeName()] = std::make_shared<TProcessor>(*this);
		}
	};
}
#endif // BaseMessageDispatcher_h__
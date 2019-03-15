#ifndef ___NICE_TCPSERVER____
#define ___NICE_TCPSERVER____
#include <string>
#include "Type.h"
#include <memory>
#include "Server.h"
#include <unordered_map>
#include <array>
#include <atomic>
#include "Message.h"
#include "NoCopy.h"
#include <functional>

namespace nicehero
{
	class TcpSession;
	typedef std::string tcpuid ;
	typedef bool(*tcpcommand)(TcpSession&, Message&);
	//typedef std::function<bool(TcpSession&, Message&)> tcpcommand;
	class TcpServer;
	class TcpSessionImpl;
	class TcpServerImpl;
	class TcpMessageParser
	{
	public:
		tcpcommand m_commands[65536] = {nullptr};
	};
	class TcpSessionCommand
	{
	public:
		TcpSessionCommand(const std::type_info & info,	//!< 类型信息
			ui16 command,					//!< 网络消息ID
			tcpcommand fucnc			//!< 处理函数)
			);
	};
	class TcpSession
		:public NoCopy,public std::enable_shared_from_this<TcpSession>
	{
		friend class TcpSessionImpl;
		friend class TcpServer;
		friend class TcpServerImpl;
	public:
		TcpSession();
		virtual void init();
		virtual void init(TcpServer& server);
		virtual void init2(TcpServer& server);
		virtual void close();
		virtual void setMessageParser(TcpMessageParser* messageParser);
		TcpMessageParser* m_MessageParser = nullptr;
		std::string& getUid();
	protected:
		std::shared_ptr<TcpSessionImpl> m_impl;
		std::string m_hash;
		tcpuid m_uid;
		ui64 m_serialID;
		Message m_PreMsg;
		bool parseMsg(unsigned char* data, ui32 len);
		virtual void removeSelf();
		virtual void removeSelfImpl();
		void doRead();
		virtual void handleMessage(std::shared_ptr<Message> msg);
	};
	class TcpSessionS
		:public TcpSession
	{
		friend class TcpServer;
		friend class TcpSessionImpl;
		friend class TcpServerImpl;
	public:
		TcpSessionS();
		virtual ~TcpSessionS();
		void init(TcpServer& server);
		void init2(TcpServer& server);
	protected:
		TcpServer* m_TcpServer = nullptr;
		void removeSelf();
		void removeSelfImpl();
	};
	class TcpSessionC
		:public TcpSession,public Server
	{
	public:
		TcpSessionC();
		virtual ~TcpSessionC();
		
		bool connect(const std::string& ip, ui16 port);
		void init(bool isSync = false);

		std::atomic<bool> m_isInit = false;
	private:
		int checkServerSign(ui8* data_);//return 0 ok 1 error 2 warnning
	};
	class TcpServer
		:public Server
	{
		friend class TcpSession;
		friend class TcpSessionS;
	public:
		TcpServer(const std::string& ip,ui16 port );
		virtual ~TcpServer();

		std::shared_ptr<TcpServerImpl> m_impl;

		bool m_Started = false;

		virtual TcpSessionS* createSession();

		virtual void addSession(const tcpuid& uid, std::shared_ptr<TcpSession> session);
		virtual void removeSession(const tcpuid& uid,ui64 serialID);

		std::unordered_map<tcpuid, std::shared_ptr<TcpSession> > m_sessions;
	};
	TcpMessageParser& getMessagerParse(const std::type_info& typeInfo);
	inline TcpSessionCommand::TcpSessionCommand(const std::type_info & info, /*!< 类型信息 */ ui16 command, /*!< 网络消息ID */ tcpcommand func /*!< 处理函数) */)
	{
		getMessagerParse(info).m_commands[command] = func;
	}

}

#define TCP_SESSION_COMMAND(CLASS,COMMAND) \
static bool _##CLASS##_##COMMAND##FUNC(nicehero::TcpSession& session, Message& msg);\
static nicehero::TcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
static bool _##CLASS##_##COMMAND##FUNC(nicehero::TcpSession& session, Message& msg)

#ifndef SESSION_COMMAND
#define SESSION_COMMAND TCP_SESSION_COMMAND
#endif

#endif

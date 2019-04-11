#ifndef ___NICE_TCPSERVER____
#define ___NICE_TCPSERVER____
#include <string>
#include "Type.h"
#include <memory>
#include "Server.h"
#include <unordered_map>
#include <array>
#include <list>
#include <atomic>
#include "Message.h"
#include "NoCopy.h"
#include <functional>
namespace nicehero
{
	class TcpSession;
	using tcpuid = std::string;
	using tcpcommand = bool(*)(TcpSession&, Message&);
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
		TcpSessionCommand(const std::type_info & info,	
			ui16 command,					
			tcpcommand fucnc			
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
		virtual void sendMessage(Message& msg);
		virtual void sendMessage(Serializable& msg);
		TcpMessageParser* m_MessageParser = nullptr;
		tcpuid& getUid();
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
		void doSend(Message& msg);
		void doSend();
		std::atomic_bool m_IsSending;
		std::list<Message> m_SendList;
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
		void startRead();
		std::atomic<bool> m_isInit;
	protected:
		void removeSelf();
	private:
		int checkServerSign(ui8* data_);//return 0 ok 1 error 2 warning
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
		virtual void accept();
		std::unordered_map<tcpuid, std::shared_ptr<TcpSession> > m_sessions;
	};
	TcpMessageParser& getTcpMessagerParse(const std::type_info& typeInfo);
	inline TcpSessionCommand::TcpSessionCommand(const std::type_info & info,  ui16 command, tcpcommand func )
	{
		TcpMessageParser& msgParser = getTcpMessagerParse(info);
		msgParser.m_commands[command] = func;
	}

}

#define TCP_SESSION_COMMAND(CLASS,COMMAND) \
static bool _##CLASS##_##COMMAND##FUNC(nicehero::TcpSession& session, nicehero::Message& msg);\
static nicehero::TcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
static bool _##CLASS##_##COMMAND##FUNC(nicehero::TcpSession& session, nicehero::Message& msg)


#ifndef SESSION_COMMAND
#define SESSION_COMMAND TCP_SESSION_COMMAND
#endif

#define TCP_SESSION_YIELDCMD(CLASS,COMMAND) \
namespace{\
class _##CLASS##_##COMMAND##TASK:asio::coroutine\
{\
	bool exe(nicehero::TcpSession& session, nicehero::Message& msg);\
};\
static nicehero::TcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
bool _##CLASS##_##COMMAND##TASK::exe(nicehero::TcpSession& session, nicehero::Message& msg){\
	reenter(this){
#endif

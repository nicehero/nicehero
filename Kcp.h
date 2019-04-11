#ifndef ___NICE_KCPSERVER____
#define ___NICE_KCPSERVER____
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
	class KcpSession;
	using kcpuid = ui32;
	using kcpcommand = bool(*)(KcpSession&, Message&);

	class KcpServer;
	class KcpSessionImpl;
	class KcpServerImpl;
	class KcpMessageParser
	{
	public:
		kcpcommand m_commands[65536] = {nullptr};
	};
	class KcpSessionCommand
	{
	public:
		KcpSessionCommand(const std::type_info & info,
			ui16 command,					
			kcpcommand fucnc			
			);
	};
	class KcpSession
		:public NoCopy,public std::enable_shared_from_this<KcpSession>
	{
		friend class KcpSessionImpl;
		friend class KcpServer;
		friend class KcpServerImpl;
	public:
		KcpSession();
		virtual void init();
		virtual void init(KcpServer& server);
		virtual void init2(KcpServer& server);
		virtual void close();
		virtual void setMessageParser(KcpMessageParser* messageParser);
		virtual void sendMessage(Message& msg);
		virtual void sendMessage(Serializable& msg);
		KcpMessageParser* m_MessageParser = nullptr;
		kcpuid& getUid();
	protected:
		std::shared_ptr<KcpSessionImpl> m_impl;
		std::string m_hash;
		kcpuid m_uid;
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
	class KcpSessionS
		:public KcpSession
	{
		friend class KcpServer;
		friend class KcpSessionImpl;
		friend class KcpServerImpl;
	public:
		KcpSessionS();
		virtual ~KcpSessionS();
		void init(KcpServer& server);
		void init2(KcpServer& server);
	protected:
		KcpServer* m_KcpServer = nullptr;
		void removeSelf();
		void removeSelfImpl();
	};
	class KcpSessionC
		:public KcpSession,public Server
	{
	public:
		KcpSessionC();
		virtual ~KcpSessionC();
		
		bool connect(const std::string& ip, ui16 port);
		void init(bool isSync = false);
		void startRead();
		std::atomic<bool> m_isInit;
	protected:
		void removeSelf();
	private:
		int checkServerSign(ui8* data_);//return 0 ok 1 error 2 warnning
	};
	class KcpServer
		:public Server
	{
		friend class KcpSession;
		friend class KcpSessionS;
	public:
		KcpServer(const std::string& ip,ui16 port );
		virtual ~KcpServer();

		std::shared_ptr<KcpServerImpl> m_impl;

		bool m_Started = false;

		virtual KcpSessionS* createSession();

		virtual void addSession(const kcpuid& uid, std::shared_ptr<KcpSession> session);
		virtual void removeSession(const kcpuid& uid,ui64 serialID);
		virtual void accept();
		std::unordered_map<kcpuid, std::shared_ptr<KcpSession> > m_sessions;
	};
	KcpMessageParser& getKcpMessagerParse(const std::type_info& typeInfo);
	inline KcpSessionCommand::KcpSessionCommand(const std::type_info & info,  ui16 command, kcpcommand func )
	{
		KcpMessageParser& msgParser = getKcpMessagerParse(info);
		msgParser.m_commands[command] = func;
	}

}

#define KCP_SESSION_COMMAND(CLASS,COMMAND) \
static bool _##CLASS##_##COMMAND##FUNC(nicehero::KcpSession& session, nicehero::Message& msg);\
static nicehero::KcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
static bool _##CLASS##_##COMMAND##FUNC(nicehero::KcpSession& session, nicehero::Message& msg)


#ifndef SESSION_COMMAND
#define SESSION_COMMAND TCP_SESSION_COMMAND
#endif

#define KCP_SESSION_YIELDCMD(CLASS,COMMAND) \
namespace{\
class _##CLASS##_##COMMAND##TASK:asio::coroutine\
{\
	bool exe(nicehero::KcpSession& session, nicehero::Message& msg);\
};\
static nicehero::KcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
bool _##CLASS##_##COMMAND##TASK::exe(nicehero::KcpSession& session, nicehero::Message& msg){\
	reenter(this){
#endif

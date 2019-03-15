#ifndef ___TCPSERVER____
#define ___TCPSERVER____
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
	typedef std::string tcpuid ;
	class TcpServer;
	class TcpSessionImpl;
	class TcpServerImpl;
	class TcpSession;
	class MessageParser
	{
	public:
		std::function<bool(TcpSession& session, Message&)> m_commands[65536];
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
		virtual void setMessageParser(MessageParser* messageParser);
		MessageParser* m_MessageParser = nullptr;
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

}
#ifndef ON_TCPMESSAGE
#define ON_TCPMESSAGE template <const ui16 cmd> static bool onMessage(nicehero::TcpSession& session,nicehero::Message& msg);
#endif

#endif

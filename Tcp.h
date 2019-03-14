#ifndef ___TCPSERVER____
#define ___TCPSERVER____
#include <string>
#include "Type.h"
#include <memory>
#include "Server.h"
#include <unordered_map>
#include <array>
#include <atomic>

namespace nicehero
{
	typedef std::string tcpuid ;
	class TcpServer;
	class TcpSessionImpl;
	class TcpServerImpl;
	class TcpSession
		:public std::enable_shared_from_this<TcpSession>
	{
	public:
		TcpSession();
		virtual void init();
		virtual void init(TcpServer& server);
		virtual void init2(TcpServer& server);
		std::shared_ptr<TcpSessionImpl> m_impl;
		std::string m_hash;
		tcpuid m_uid;
		ui64 m_serialID;

		void doRead();
	};
	class TcpSessionS
		:public TcpSession
	{
	public:
		TcpSessionS();
		virtual ~TcpSessionS();
		void init(TcpServer& server);
		void init2(TcpServer& server);
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

#endif

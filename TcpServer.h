#ifndef ___TCPSERVER____
#define ___TCPSERVER____
#include <string>
#include "Type.h"
#include <memory>
#include "Server.h"

namespace nicehero
{
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
		std::string m_sign;
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
	class TcpServer
		:public Server
	{
	public:
		TcpServer(const std::string& ip,ui16 port );
		virtual ~TcpServer();

		std::shared_ptr<TcpServerImpl> m_impl;

		bool m_Started = false;

		virtual TcpSessionS* createSession();
	};
}

#endif

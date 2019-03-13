#ifndef ___TCPSERVER____
#define ___TCPSERVER____
#include <string>
#include "Type.h"
#include <memory>

namespace nicehero
{
	class TcpSessionImpl;
	class TcpSession
	{
	public:
		TcpSession();
		std::shared_ptr<TcpSessionImpl> m_impl;
	};
	class TcpSessionS
		:public TcpSession
	{
	public:
		TcpSessionS();
	};
	class TcpServerImpl;
	class TcpServer
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

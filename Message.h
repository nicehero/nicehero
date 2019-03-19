#ifndef __NICEMESSAGE____
#define __NICEMESSAGE____
#include <memory>
#include <memory.h>
#include "Type.h"
#define MSG_SIZE 32 * 1024
namespace nicehero
{
	class Message
	{
	public:
		Message()
		{
			m_buff = nullptr;
		}

		Message(void* buff, size_t size_)
		{
			m_buff = nullptr;
			write_data(buff, size_);
		}

		void write_data(void* buff, size_t size_)
		{
			if (m_buff == (unsigned char*)&m_writePoint)
			{
				m_buff = nullptr;
			}
			if (m_buff)
			{
				delete m_buff;
				m_buff = nullptr;
			}
			m_buff = new unsigned char[size_];
			m_writePoint = 6;
			m_readPoint = 6;
			memcpy(m_buff, buff, size_);
		}

		bool push_data()
		{
			return true;
		}

		~Message()
		{
			if (m_buff == (unsigned char*)&m_writePoint)
			{
				m_buff = nullptr;
			}
			if (m_buff)
			{
				delete m_buff;
				m_buff = nullptr;
			}
		}

		ui16 getMsgID()
		{
			if (!m_buff)
			{
				return 0;
			}
			return *(unsigned short*)(m_buff + 4);
		}

		ui32 getSize()
		{
			if (!m_buff)
			{
				return 0;
			}
			return *(unsigned long*)(m_buff);
		}

		void swap(Message& msg)
		{
			unsigned char* buff = msg.m_buff;
			msg.m_buff = m_buff;
			m_buff = buff;
			m_writePoint = 6;
			m_readPoint = 6;
			msg.m_writePoint = 6;
			msg.m_readPoint = 6;
		}

		ui8* m_buff;
		ui32 m_writePoint = 6;
		ui32 m_readPoint = 6;
	};
}

#endif

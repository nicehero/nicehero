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

		Message(const void* buff, size_t size_)
		{
			m_buff = nullptr;
			if (size_ < 6)
			{
				return;
			}
			buildBuff(size_);
			memcpy(m_buff, buff, size_);
		}
		Message& ID(ui16 msgID);
		void write_data(const void* buff, size_t size_)
		{
			if (getSize() < 1)
			{
				return;
			}
			memcpy(m_buff + m_writePoint, buff, size_);
			m_writePoint += (ui32)size_;
		}
		const void* read_data(size_t size_)
		{
			if (m_readPoint + size_ > getSize())
			{
				//todo throw
				return nullptr;
			}
			ui8* ret = m_buff + m_readPoint;
			m_readPoint += (ui32)size_;
			return ret;
		}
		~Message()
		{
			clear();
		}

		ui16 getMsgID()
		{
			if (!m_buff || m_buff == (unsigned char*)&m_writePoint)
			{
				return 0;
			}
			return *(unsigned short*)(m_buff + 4);
		}

		ui32 getSize()
		{
			if (!m_buff || m_buff == (unsigned char*)&m_writePoint)
			{
				return 0;
			}
			return *(unsigned long*)(m_buff);
		}

		virtual void serialize()
		{

		}

		void swap(Message& msg)
		{
			serialize();
			unsigned char* buff = msg.m_buff;
			msg.m_buff = m_buff;
			m_buff = buff;
			m_writePoint = 6;
			m_readPoint = 6;
			msg.m_writePoint = 6;
			msg.m_readPoint = 6;
		}

		void clear()
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
			m_writePoint = 6;
			m_readPoint = 6;
		}

		void buildBuff(size_t size_)
		{
			clear();
			m_buff = new unsigned char[size_];
			*(ui32*)m_buff = ui32(size_);
		}

		ui8* m_buff;
		ui32 m_writePoint = 6;//write offset
		ui32 m_readPoint = 6;//read offset

	};
	inline Message & Message::ID(ui16 msgID)
	{
		if (!m_buff)
		{
			buildBuff(6);
		}
		*((ui16*)(m_buff + 4)) = msgID;
		return *this;
	}

	struct Serializable
	{
	public:
		virtual ~Serializable() {

		}
		ui32 getSize(const std::string& s) const
		{
			return (ui32)s.size() + 4;
		}
		ui32 getSize(const ui64& s) const
		{
			return sizeof(s);
		}
		ui32 getSize(const Serializable& s) const
		{
			return s.getSize();
		}
		virtual ui32 getSize() const = 0;
		void toMsg(Message& msg) const;
		virtual ui16 getID() const 
		{
			return 0;
		}
		virtual void serializeTo(nicehero::Message& msg) const  = 0;
		virtual void unserializeFrom(nicehero::Message& msg) = 0;
	};

	inline void Serializable::toMsg(Message& msg) const
	{
		ui32 s = 6;
		s += getSize();
		msg.buildBuff(s);
		msg.ID(getID());
		serializeTo(msg);
	}

	inline Message & operator << (Message &m, const ui64 & p)
	{
		m.write_data(&p, sizeof(p));
		return m;
	}

	inline Message & operator << (Message &m, const std::string& p)
	{
		ui32 s = ui32(p.size());
		m.write_data(&s, 4);
		m.write_data(p.data(), p.size());
		return m;
	}

	inline Message & operator >> (Message &m, ui64 & p)
	{
		p = *(ui64*)m.read_data(sizeof(p));
		return m;
	}

	inline Message & operator >> (Message &m, std::string& p)
	{
		ui32 s = 0;
		s = *(ui32*)m.read_data(4);
		p.assign((const char*)m.read_data(s), s);
		return m;
	}

}

#endif

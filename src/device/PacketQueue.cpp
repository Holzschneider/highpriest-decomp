#include "common.h"
#include "PacketQueue.h"

#include <string.h>


CPacketQueue::CPacketQueue()
{
	m_buf = NULL;
}

CPacketQueue::CPacketQueue(int capacity)
{
	m_buf = NULL;
	Init(capacity);
}

CPacketQueue::~CPacketQueue()
{
}

int		CPacketQueue::GetSize() const
{
	return m_rearPos - m_frontPos;
}

void	CPacketQueue::Init(int capacity)
{
	m_frontPos = 0;
	m_rearPos = 0;
	m_buffer.resize( capacity ) ;
	m_buf = m_buffer.begin();
}

void	CPacketQueue::InsertData(int len, const char *src)
{
	int capacity = m_buffer.size();

	if(m_rearPos+len > capacity) {

		if((m_rearPos-m_frontPos)+len > capacity) {
			m_buffer.resize(m_buffer.size() * 2);
			m_buf = m_buffer.begin() ;
		}

		memmove(m_buf, m_buf+m_frontPos, m_rearPos - m_frontPos);
		m_rearPos = m_rearPos - m_frontPos;
		m_frontPos = 0;
	}

	memcpy(m_buf+m_rearPos, src, len);
	m_rearPos += len;
}

bool	CPacketQueue::GetData(int len, char *dst)
{
	if(m_rearPos-m_frontPos < len) {
		return false;
	} else {
		memcpy(dst, m_buf+m_frontPos, len);
		m_frontPos += len;
		return true;
	}
}

bool	CPacketQueue::PeekData(int len, char *dst) const
{
	if(m_rearPos-m_frontPos < len) {
		return false;
	} else {
		memcpy(dst, m_buf+m_frontPos, len);
		return true;
	}
}

bool	CPacketQueue::RemoveData(int len)
{
	if(m_rearPos-m_frontPos < len) {
		return false;
	} else {
		m_frontPos += len;
		return true;
	}
}

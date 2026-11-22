#ifndef HIGHPRIEST_DEVICE_PACKETQUEUE_H
#define HIGHPRIEST_DEVICE_PACKETQUEUE_H


class CPacketQueue;

class CPacketQueue {
public:
	CPacketQueue(int capacity);
	CPacketQueue();
	~CPacketQueue();
	void Init(int capacity);
	int GetSize() const;
	void InsertData(int len, const char* src);
	bool GetData(int len, char* dst);
	bool PeekData(int len, char* dst) const;
	bool RemoveData(int len);
	int GetSafeDataLen() const { return m_rearPos - m_frontPos; }
	char* GetDataPtr() const { return m_buf + m_frontPos; }
private:
	char* m_buf;
	int m_frontPos;
	int m_rearPos;
	std::vector<char> m_buffer;
};

#endif

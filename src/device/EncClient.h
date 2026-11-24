#ifndef HIGHPRIEST_DEVICE_ENCCLIENT_H
#define HIGHPRIEST_DEVICE_ENCCLIENT_H


struct coder_packet {
public:
	short len;
	short random;
};

struct coder_data {
public:
	unsigned int temp[65536];
	unsigned char func[4096];
};

class EncClient;

class EncClient {
public:
	EncClient();
	void Reset();
	int GetKeySize(const coder_packet&);
	void AddEncKey(const coder_packet& coderPacket, unsigned char* encKey);
	void AddDecKey(const coder_packet& coderPacket, unsigned char* encKey);
	void EncData(unsigned char* buffer, int len);
	void DecData(unsigned char* buffer, int len);
	unsigned char IsEnable();
private:
	unsigned char* FindToken(unsigned char* buf);
	void SET_JMP(unsigned char* func, unsigned char* target);
	void SET_CALL(unsigned char* func, unsigned char* target);
	void SET_DEST_ADDR(unsigned char* func, unsigned char* target);
	void InitCoder(unsigned char* func);
public:
	typedef std::list<coder_data> coder_list;
	typedef coder_list::iterator coder_list_iter;
private:
	std::list<coder_data> m_decoderData;
	std::list<coder_data> m_encoderData;
	unsigned char* m_coderDestAddr;
};

#endif

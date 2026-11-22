#include "common.h"














#include "EncClient.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>



extern unsigned char TOKEN[];
unsigned char TOKEN[6] = { 0x33, 0xc0, 0x33, 0xdb, 0x33, 0xc9 };




void Coder(unsigned char* buf, unsigned int len, unsigned long* temp, bool client)
{
	char buffer[0x400];
	__asm {
		xor eax, eax
		xor ebx, ebx
		xor ecx, ecx
	}
	memcpy(buffer, buf, len);
	memcpy(buf, buffer, len);
	*temp = len;
	__asm {
		xor eax, eax
		xor ebx, ebx
		xor ecx, ecx
	}
}

void EncClient::Reset()
{
	m_decoderData.clear();
	m_encoderData.clear();
	InitCoder(reinterpret_cast<unsigned char*>(&Coder));
}

void EncClient::AddEncKey(const coder_packet& coderPacket, unsigned char* encKey)
{
	coder_data encoderData;
	unsigned int i = 0;
	int random = coderPacket.random;
	encoderData.temp[0] = random;
	unsigned char* keyBuf = encoderData.func;
	for (; static_cast<int>(i) < (coderPacket.random ^ coderPacket.len); ++i)
		keyBuf[i] = encKey[i] ^ static_cast<unsigned char>(coderPacket.random);
	for (; i < 0x1000; ++i)
		keyBuf[i] = static_cast<unsigned char>(rand());

	m_encoderData.push_back(encoderData);
}

void EncClient::AddDecKey(const coder_packet& coderPacket, unsigned char* decKey)
{
	coder_data decoderData;
	unsigned int i = 0;
	int random = coderPacket.random;
	decoderData.temp[0] = random;
	unsigned char* keyBuf = decoderData.func;
	for (; static_cast<int>(i) < (coderPacket.random ^ coderPacket.len); ++i)
		keyBuf[i] = decKey[i] ^ static_cast<unsigned char>(coderPacket.random);
	for (; i < 0x1000; ++i)
		keyBuf[i] = static_cast<unsigned char>(rand());

	m_decoderData.push_back(decoderData);
}

void EncClient::EncData(unsigned char* buffer, int len)
{
	for (std::list<coder_data>::iterator it = m_encoderData.begin(); it != m_encoderData.end(); ++it)
	{
		SET_DEST_ADDR(m_coderDestAddr, it->func);
		Coder(buffer, len, it->temp, true);
	}
}

void EncClient::DecData(unsigned char* buffer, int len)
{
	for (std::list<coder_data>::iterator it = m_decoderData.begin(); it != m_decoderData.end(); ++it)
	{
		SET_DEST_ADDR(m_coderDestAddr, it->func);
		Coder(buffer, len, it->temp, true);
	}
}

unsigned char* EncClient::FindToken(unsigned char* buf)
{
	unsigned char* p = buf;
	while (memcmp(p, &TOKEN, 6) != 0)
		++p;
	return p;
}

void EncClient::SET_JMP(unsigned char* func, unsigned char* target)
{
	*func++ = 0xE9;
	*reinterpret_cast<int*>(func) = static_cast<int>(target - func - 4);
}

void EncClient::SET_CALL(unsigned char* func, unsigned char* target)
{
	*func++ = 0xE8;
	*reinterpret_cast<int*>(func) = static_cast<int>(target - func - 4);
}

void EncClient::SET_DEST_ADDR(unsigned char* func, unsigned char* target)
{
	*reinterpret_cast<int*>(func) = static_cast<int>(target - func - 4);
}

void EncClient::InitCoder(unsigned char* func)
{
	while (*func == 0xE9)
		func = func + 5 + *reinterpret_cast<int*>(func + 1);

	unsigned char* token1 = FindToken(func) + 6;
	DWORD oldProtectFlag;
	VirtualProtect(token1, 0xa, 4, &oldProtectFlag);
	token1[0] = 0xE8;
	m_coderDestAddr = token1 + 1;

	unsigned char* token2 = FindToken(token1);
	token1[5] = 0xE9;
	SET_DEST_ADDR(token1 + 6, token2 + 6);
}

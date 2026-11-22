#ifndef HIGHPRIEST_GAMEACTORMSGHANDLER_H
#define HIGHPRIEST_GAMEACTORMSGHANDLER_H


class RandGenerator;

class RandGenerator {
public:
	RandGenerator(int callNumber, int randSeed);
	int RandNum();
private:
	int m_curCallNumber;
	int m_maxCallNumber;
	int m_randSeed;
	int m_totalRandNum;
};

#endif

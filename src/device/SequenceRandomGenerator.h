#ifndef HIGHPRIEST_DEVICE_SEQUENCERANDOMGENERATOR_H
#define HIGHPRIEST_DEVICE_SEQUENCERANDOMGENERATOR_H


#include <windows.h>
#include <stdio.h>

class CSequenceRandomGenerator;

class CSequenceRandomGenerator {
public:
	struct tagGenerator {
	public:
		unsigned char bExist;
		int holdrand;
		int addfactor;
		int mulfactor;
		unsigned int deformed_seed_factor;
		unsigned int deformed_add_factor;
		unsigned int issue_time;
	};

	struct tagSeedFactor {
		union {
			struct tagBit {
				unsigned int seed1 : 4;
				unsigned int mask2 : 4;
				unsigned int mask4 : 4;
				unsigned int seed2 : 4;
				unsigned int mask3 : 4;
				unsigned int seed3 : 4;
				unsigned int seed4 : 4;
				unsigned int mask1 : 4;
			} bit;
			unsigned int seedfactor;
		};
	};

public:
	void Init(const unsigned int in_seedfactor, const unsigned int in_addfactor);
	void Reset();
	void GetFactor(unsigned short*);
	CSequenceRandomGenerator();
	virtual ~CSequenceRandomGenerator();
private:
	int m_holdrand;
	int m_mulfactor;
	int m_addfactor;
	tagGenerator m_Generator;
};

#endif

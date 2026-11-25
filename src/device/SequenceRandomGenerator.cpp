#include "bf_types.h"
#include "SequenceRandomGenerator.h"
#include <stdlib.h>

CSequenceRandomGenerator::CSequenceRandomGenerator(void) :
	m_holdrand(0),
	m_mulfactor(1),
	m_addfactor(0)
{
	m_Generator.bExist = false;
}


CSequenceRandomGenerator::~CSequenceRandomGenerator(void)
{
}

void CSequenceRandomGenerator::Reset(void)
{
	m_holdrand = 0;
	m_mulfactor = 1;
	m_addfactor = 0;
}

void CSequenceRandomGenerator::Init(const unsigned int in_seedfactor, const unsigned int in_addfactor)
{
	struct tagSeedFactor SeedFactor;

	SeedFactor.seedfactor = in_seedfactor;

	m_holdrand = SeedFactor.bit.seed1 +
				 (SeedFactor.bit.seed2 << 4) +
				 (SeedFactor.bit.seed3 << 8) +
				 (SeedFactor.bit.seed4 << 12);

	m_mulfactor = 214013L;
	unsigned int mask = SeedFactor.bit.mask1 +
						(SeedFactor.bit.mask2 << 4) +
						(SeedFactor.bit.mask3 << 8) +
						(SeedFactor.bit.mask4 << 12);
	unsigned int add_mask = (mask + (m_holdrand^0x49df)) | ((mask + (m_holdrand^0xf3ac)) << 16);
	m_addfactor = in_addfactor ^ add_mask;
}

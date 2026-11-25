#include "common.h"
#include "Insult.h"
#include "ItemInfo.h"

#include <ctype.h>
#include <string.h>
#include "bf_types.h"
#include "bf_text.h"

extern DWORD g_codePage;




static char sentenceBuffer[260];
CInsultFilter g_insultFilter;
static CommonObject s_obj;


const char * MakeLower(char * text)
{
	char * textIter = text;
	while ( textIter < text + strlen(text) ) {
		if ( *textIter > 0 )
		{
			if ( isupper(*textIter) )
				*textIter += ('a' - 'A');
		}
		textIter += bf_text_char_length(g_codePage, textIter, -1);
	}
	return text;
}

Insult::Insult(char* word)
{
	this->word = MakeLower(word);
}

bool CInsultFilter::IsBadSentence(const char* str)
{
	return !IsGoodSentence(str);
}

bool CInsultFilter::IsGoodSentence(const char* str)
{
	strcpy(sentenceBuffer, str);
	const char* nStr = MakeLower(sentenceBuffer);
	const char* nStrEnd = nStr + strlen(nStr);
	if ( nStr < nStrEnd ) {
		do {
			for ( std::vector<Insult>::iterator i = m_insult.begin(); i != m_insult.end(); ++i )
			{
				if ( strncmp(i->word, nStr, strlen(i->word)) == 0 )
					return false;
			}
			nStr += bf_text_char_length(g_codePage, nStr, -1);
		} while ( nStr < nStrEnd );
	}
	return true;
}

void CInsultFilter::Insert(char* str)
{
	char* pc = str;
	while ( *pc == ' ' || *pc == '\t' )
		pc++;
	char* word = pc;

	while ( *pc ) pc++;

	while ( *pc == ' ' ) pc--;
	*pc = 0;

	if ( !*word )
		return;

	Insult insult(str);
	m_insult.push_back(insult);
}

bool CInsultFilter::Load(const char* fName)
{
	PCharList pCharList;
	bool b = s_obj.TokenFileToPCharList(pCharList, fName, 1);
	for ( PCharListIter it = pCharList.begin(); it != pCharList.end(); ++it )
	{
		Insert((char*)*it);
	}
	return b;
}

CInsultFilter::CInsultFilter()
{
}

CInsultFilter::~CInsultFilter()
{
}

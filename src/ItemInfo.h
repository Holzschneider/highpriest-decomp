#ifndef HIGHPRIEST_ITEMINFO_H
#define HIGHPRIEST_ITEMINFO_H

#include "common.h"




typedef	std::vector<LPCSTR>					PCharList;
typedef	std::vector<LPCSTR>::iterator		PCharListIter;
typedef std::vector<int>						IntegerList;
typedef	std::map<int, struct ITEM_DESC>		itemdescmap;
typedef	std::map<int, struct CARD_DESC>		carddescmap;
typedef	std::map<int, PCharList>				maketargetdesc;
typedef	std::map<int, PCharList>				refineleveldesc;
typedef	std::map<int, int>				itemleveldesc;
typedef	std::map<int, PCharList>				bookDescMap ;
typedef	std::map<int, struct QUEST_DESC>		questDescMap ;
typedef std::pair<struct ITEM_DESC*, bool>	itemdescfind;
typedef std::pair<struct CARD_DESC*, bool>	carddescfind;

struct DRAG_INFO;
struct QUEST_DESC;
struct SHOP_ITEM;
struct CARD_DESC;
struct CommonObject;
struct ITEM_DESC;
struct ITEM_INFO;

struct CommonObject {
public:
	CommonObject() { Clear(); }
	~CommonObject() { Clear(); }
	void Clear();
	bool TokenFileToPCharList(std::vector<char const *>& pCharList, const char* fName, int type = 0);
	std::vector<char *> m_deleteList;
};

struct QUEST_DESC {
	QUEST_DESC() : questName(NULL), iconName(NULL) {}
	LPCSTR questName;
	LPCSTR iconName;
	LPCSTR detailBmp;
	LPCSTR summaryTXT;
	LPCSTR objectTXT;
};

struct ITEM_DESC {
public:
	ITEM_DESC()
		: identifiedDisplayName(0), identifiedResourceName(0),
		  unidentifiedDisplayName(0), unidentifiedResourceName(0),
		  slot(0), slotCount(0)
	{
	}
	const char* identifiedDisplayName;
	const char* identifiedResourceName;
	std::vector<char const *> identifiedDescriptionName;
	const char* unidentifiedDisplayName;
	const char* unidentifiedResourceName;
	std::vector<char const *> unidentifiedDescriptionName;
	unsigned int slot;
	int slotCount;
	std::vector<int> parameter;
};

struct CARD_DESC {
public:
	CARD_DESC() : illustName(0), prefix(0), postfix(0) {}
	const char* illustName;
	const char* prefix;
	const char* postfix;
};

struct SHOP_ITEM {
	int index;
	int price;
	int realPrice;
	int type;
	unsigned char isIdentified;
	char itemName[32+1];
};

struct ITEM_INFO {
public:
	ITEM_INFO(int) { Reset(); }
	ITEM_INFO() { Reset(); }
	void SetItemId(int __formal);
	int GetItemId() const { return atoi(m_itemName.c_str()); }
	int GetRefining(char* refining) const;
	int GetNamed(char* refining) const;
	int GetPrefix(char* refining) const;
	int GetPostfix(char* refining) const;
	int GetManufacturedItemPrefix(char* refining) const;
	int GetManufacturedItemProducer(char* refining) const;
	int GetManufacturedItemPostfix(char* refining) const;
	int GetIdDisplayName(char* refining) const;
	int GetUnidDisplayName(char* refining) const;
	int GetSocketNumber(char* refining) const;
	char* GetItemName(char* itemName);
	bool IsValid() { return m_itemIndex ? true : false; }
	void Reset() { memset(this, 0, sizeof(*this)); }
	const char* GetResName();
	const std::vector<char const *>& GetDescription();
	int GetSlotCount();
	void CopyItemInfo(const DRAG_INFO* dragInfo);
	unsigned char IsIdentified() const;
	unsigned char IsCardItem() const;
	static bool __cdecl IsCardItem(int itemName);
	static bool IsCardItem(const char* itemName) { return IsCardItem(atoi(itemName)); }
	unsigned char HasCardItem() const { return IsCardItem(m_slot[0]); }
	bool IsManufacturedItem() const;
	bool IsRecoveryItem() const;
	bool IsAttachableItem() const;
	bool IsPropertyItem() const;
	bool IsMaterialItem() const;
	int GetRefiningName(char*);
	int GetPrefixName(char*);
	int GetManufacturerName(char*);
	int GetDisplayName(char*);
	int GetPostfixName(char*);
	ITEM_INFO& operator=(const SHOP_ITEM& iteminfo);
	ITEM_INFO& operator=(const ITEM_INFO& iteminfo);
	int SetType();
	static void __cdecl InitItemInfoTables();
	static const ITEM_DESC& __cdecl GetItemDesc(const char* itemName) { return GetItemDesc(atoi(itemName)); }
	static const ITEM_DESC& __cdecl GetItemDesc(int itemName);
	static const CARD_DESC& __cdecl GetCardDesc(const char* itemName) { return GetCardDesc(atoi(itemName)); }
	static const CARD_DESC& __cdecl GetCardDesc(int itemName);
	static const std::vector<char const *>& __cdecl GetMakeTargetDesc(const char* itemName) { return GetMakeTargetDesc(atoi(itemName)); }
	static const std::vector<char const *>& __cdecl GetMakeTargetDesc(int itemName);
	unsigned char IsBookItem() const;
	static bool __cdecl IsBookItem(int itemName);
	static bool IsBookItem(const char* itemName) { return IsBookItem(atoi(itemName)); }
	static const QUEST_DESC& __cdecl GetQuestDesc(int itemName);
protected:
	static std::map<int, ITEM_DESC> s_itemDescMap;
	static std::map<int, CARD_DESC> s_cardDescMap;
	static std::map<int, std::vector<char const *> > s_maketargetdesc;
	static CommonObject s_obj;
	static std::map<int, std::vector<char const *> > s_bookDescMap;
	static std::map<int, QUEST_DESC> s_questDescMap;
public:
	int m_itemType;
	int m_location;
	unsigned int m_itemIndex;
	int m_wearLocation;
	int m_num;
	int m_price;
	int m_realPrice;
	int m_slot[4];
	std::string m_itemName;
	unsigned char m_isIdentified;
	unsigned char m_isDamaged;
	int m_refiningLevel;
	unsigned short m_isYours;
	int m_deleteTime;
};



template<class T>
typename T::referent_type& GetDesc(typename T::key_type itemName, T& table)
{
	static typename T::referent_type nullDesc;

	typename T::iterator iter = table.find(itemName);

	if (iter == table.end())
		return nullDesc;
	else
		return iter->second;
}

#endif

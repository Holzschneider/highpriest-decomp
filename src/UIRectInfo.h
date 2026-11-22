#ifndef HIGHPRIEST_UIRECTINFO_H
#define HIGHPRIEST_UIRECTINFO_H


struct BasicInfoWndInfo;
struct ChatRoomWndInfo;
struct ChooseWndInfo;
struct CombinedCardItemCollectionWndInfo;
struct DetailLevelWndInfo;
struct EmotionListWndInfo;
struct EmotionWndInfo;
struct EquipWndInfo;
struct ExchangeAcceptWndInfo;
struct FriendOptionWndInfo;
struct GuildWndInfo;
struct HomunInfoWndInfo;
struct ItemCollectionWndInfo;
struct ItemCompositionWndInfo;
struct ItemIdentifyWndInfo;
struct ItemParamChangeDisplayWndInfo;
struct ItemPurchaseWndInfo;
struct ItemSellWndInfo;
struct ItemShopWndInfo;
struct ItemStoreWndInfo;
struct ItemWndInfo;
struct MerInfoWndInfo;
struct MerchantItemMyShopWndInfo;
struct MerchantItemPurchaseWndInfo;
struct MerchantItemShopWndInfo;
struct MerchantItemWndInfo;
struct MerchantMirrorItemWndInfo;
struct MerchantShopMakeWndInfo;
struct MessengerGroupWndInfo;
struct OptionWndInfo;
struct PartySettingWndInfo;
struct PetInfoWndInfo;
struct QuestWndInfo;
struct SayDialogWndInfo;
struct ShortCutWndInfo;
struct ShortenItemWndInfo;
struct ShortenSkillWndInfo;
struct SkillListWndInfo;
struct StatusWndInfo;
struct SubChatWndInfo;
struct WhisperWndInfo;

struct BasicInfoWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int orgHeight;
	int curTab;
	int curRadio1;
	int curRadio2;
	int curRadio3;
	int show;
};

struct QuestWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int orgHeight;
	int curTab;
	int show;
};

struct StatusWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int orgHeight;
	int show;
};

struct EquipWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int orgHeight;
	int show;
};

struct OptionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int orgHeight;
	int show;
};

struct ShortenItemWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
};

struct ShortenSkillWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct SayDialogWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ChooseWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ChatRoomWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemStoreWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemShopWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemSellWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemPurchaseWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MessengerGroupWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
	int radio;
};

struct ShortCutWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
};

struct ExchangeAcceptWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct SkillListWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
};

struct MerchantItemWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
};

struct MerchantMirrorItemWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MerchantShopMakeWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MerchantItemShopWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MerchantItemMyShopWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MerchantItemPurchaseWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemCollectionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct CombinedCardItemCollectionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemParamChangeDisplayWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct PartySettingWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct DetailLevelWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct GuildWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int curTab;
};

struct WhisperWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct FriendOptionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemCompositionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct ItemIdentifyWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct EmotionWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct EmotionListWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct PetInfoWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct HomunInfoWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct MerInfoWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

struct SubChatWndInfo {
public:
	void MakeDefault(int mode);
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
};

#endif

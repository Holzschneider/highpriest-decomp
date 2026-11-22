#include "common.h"
#include "UIRectInfo.h"
#include "window/UISys.h"

extern CRenderer* g_renderer;




extern int guildWndWidth;
extern int guildWndHeight;

void	SayDialogWndInfo :: MakeDefault()
{
	x = 200;
	y = 100;
	w = 280;
	h = 180;
}

BOOL	SayDialogWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h!= 180)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	ChooseWndInfo :: MakeDefault()
{
	x = 200;
	y = 300;
	w = 280;
	h = 120;
}

BOOL	ChooseWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h!= 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	BasicInfoWndInfo :: MakeDefault()
{
	x = 740;
	y = 645;
	w = 280;
	h = 120;
}

BOOL	BasicInfoWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 34 && h!= 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	ItemWndInfo :: MakeDefault()
{
	x = 0;
	y = 120;
	w = 280;
	h = 120;
	orgHeight = 120;
	curTab = 0;
	curRadio1 = 3;
	curRadio2 = 3;
	curRadio3 = 4;
	show = 0;
}

BOOL	ItemWndInfo :: IsValidInfo()
{
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	if (curTab<0 || curTab>2)
		return false;
	if (curRadio1<0 || curRadio1>3)
		return false;
	if (curRadio2<0 || curRadio2>3)
		return false;
	if (curRadio3<0 || curRadio3>4)
		return false;
	return TRUE;
}

void	QuestWndInfo :: MakeDefault()
{
	x = 0;
	y = 120;
	w = 280;
	h = 200;
	orgHeight = 200;
	curTab = 0;

	show = 0;
}

BOOL	QuestWndInfo :: IsValidInfo()
{








	return TRUE;
}

BOOL PartySettingWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 180)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void PartySettingWndInfo :: MakeDefault()
{
	x = 280;
	y = 246;
	w = 280;
	h = 180;
}

BOOL ItemParamChangeDisplayWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 34)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemParamChangeDisplayWndInfo :: MakeDefault()
{
	x = 280;
	y = 246;
	w = 280;
	h = 34;
}

BOOL ItemCollectionWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemCollectionWndInfo :: MakeDefault()
{
	x = 280;
	y = 126;
	w = 280;
	h = 120;
}

BOOL CombinedCardItemCollectionWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void CombinedCardItemCollectionWndInfo :: MakeDefault()
{
	x = 280;
	y = 126+128+24;
	w = 280;
	h = 120;
}

BOOL GuildWndInfo :: IsValidInfo()
{
	if (w != guildWndWidth)
		return false;
	if (h != guildWndHeight)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	if (curTab<0 || curTab>=7)
		return false;
	return TRUE;
}

void GuildWndInfo :: MakeDefault()
{
	x = 0;
	y = 34;
	w = guildWndWidth;
	h = guildWndHeight;
	curTab = 0;
}

BOOL DetailLevelWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void DetailLevelWndInfo :: MakeDefault()
{
	x = 200;
	y = 200;
	w = 280;
	h = 120;
}

BOOL ExchangeAcceptWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ExchangeAcceptWndInfo :: MakeDefault()
{
	x = 200;
	y = 200;
	w = 280;
	h = 120;
}

const int	equipOrgHeight = 120+27;

void	EquipWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 280;
	h = 120+27;
	orgHeight = 120+27;
	show = 0;
}

BOOL	EquipWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != equipOrgHeight && h != 17 && h != equipOrgHeight + 20)
		return false;
	if (orgHeight != equipOrgHeight && orgHeight != equipOrgHeight + 20)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

BOOL SkillListWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void SkillListWndInfo :: MakeDefault()
{
	x = 640-280;
	y = 200;
	w = 280;
	h = 240;
}

BOOL ItemStoreWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemStoreWndInfo :: MakeDefault()
{
	x = 640-280;
	y = 200;
	w = 280;
	h = 240+32*6;
}

BOOL ItemShopWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemShopWndInfo :: MakeDefault()
{
	x = 0;
	y = 120;
	w = 280;
	h = 312;
}

BOOL ItemSellWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemSellWndInfo :: MakeDefault()
{
	x = 280;
	y = 280;
	w = 280;
	h = 152;
}

BOOL MessengerGroupWndInfo :: IsValidInfo()
{
	if (w < 140)
		return false;
	if (h < 155)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	if (radio!=0 && radio!=1)
		return false;
	return TRUE;
}

void MessengerGroupWndInfo :: MakeDefault()
{
	w = 140;
	h = 155;
	x = 300;
	y = 200;
	show = 0;
	radio = 1;
}

BOOL ItemPurchaseWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemPurchaseWndInfo :: MakeDefault()
{
	x = 280;
	y = 280;
	w = 280;
	h = 152;
}

void	ChatRoomWndInfo :: MakeDefault()
{
	x = 640-140;
	y = 240-60;
	w = 280;
	h = 120;
}

BOOL	ChatRoomWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	WhisperWndInfo :: MakeDefault()
{
	w = 280;
	h = 120;
	x = 0;
	y = 120+27+200;
}

BOOL WhisperWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	StatusWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 280;
	h = 120;
	orgHeight = 120;
	show = 0;
}

BOOL	StatusWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120 && h != 17)
		return false;
	if (orgHeight != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	OptionWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 280;
	h = 120;
	orgHeight = 120;
	show = 0;
}

BOOL	OptionWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120 && h != 17)
		return false;
	if (orgHeight != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	ShortCutWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 280;
	h = 34;
	show = 0;
}

BOOL	ShortCutWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 34)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	ShortenItemWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 140;
	h = 30;
	show = 0;
}

BOOL	ShortenItemWndInfo :: IsValidInfo()
{
	if (w != 140)
		return false;
	if (h != 30)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void	ShortenSkillWndInfo :: MakeDefault()
{
	x = 185;
	y = 300;
	w = 140;
	h = 30;
}

BOOL	ShortenSkillWndInfo :: IsValidInfo()
{
	if (w != 140)
		return false;
	if (h != 30)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

BOOL MerchantItemWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantItemWndInfo :: MakeDefault()
{
	x = 200;
	y = 120;
	w = 280;
	h = 120;
}

BOOL MerchantMirrorItemWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantMirrorItemWndInfo :: MakeDefault()
{
	x = 0;
	y = 120;
	w = 280;
	h = 120;
}

BOOL MerchantShopMakeWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 220)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantShopMakeWndInfo :: MakeDefault()
{
	x = 0;
	y = 240;
	w = 280;
	h = 220;
}

BOOL MerchantItemShopWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantItemShopWndInfo :: MakeDefault()
{
	x = 0;
	y = 120;
	w = 280;
	h = 312;
}

BOOL MerchantItemMyShopWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantItemMyShopWndInfo :: MakeDefault()
{
	x = 200;
	y = 120;
	w = 280;
	h = 120;
}

BOOL MerchantItemPurchaseWndInfo :: IsValidInfo()
{
	if (w < 280)
		return false;
	if (h < 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void MerchantItemPurchaseWndInfo :: MakeDefault()
{
	x = 280;
	y = 280;
	w = 280;
	h = 152;
}

BOOL FriendOptionWndInfo :: IsValidInfo()
{
	if (w != 280)
		return false;
	if (h != 120)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void FriendOptionWndInfo :: MakeDefault()
{
	x = 200;
	y = 120;
	w = 280;
	h = 120;
}

BOOL ItemIdentifyWndInfo :: IsValidInfo()
{
	if (w != 200)
		return false;
	if (h != 200)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemIdentifyWndInfo :: MakeDefault()
{
	x = UIX(200);
	y = UICY(200);
	w = 200;
	h = 200;
}

BOOL ItemCompositionWndInfo :: IsValidInfo()
{
	if (w != 200)
		return false;
	if (h != 200)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}

void ItemCompositionWndInfo :: MakeDefault()
{
	x = UIX(200);
	y = UICY(200);
	w = 200;
	h = 200;
}

void	EmotionWndInfo :: MakeDefault()
{
	x = 200;
	y = 150;
	w = 375;
	h = 280;
}

BOOL	EmotionWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return FALSE;
	if (h <= 0)
		return FALSE;
	if (x<0 || x>g_renderer->m_width-w)
		return FALSE;
	if (y<0 || y>g_renderer->m_height-h)
		return FALSE;
	return TRUE;
}

void	EmotionListWndInfo :: MakeDefault()
{
	x = 200;
	y = 200;
	w = 300;
	h = 300;
}

BOOL	EmotionListWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return FALSE;
	if (h <= 0)
		return FALSE;
	if (x<0 || x>g_renderer->m_width-w)
		return FALSE;
	if (y<0 || y>g_renderer->m_height-h)
		return FALSE;
	return TRUE;
}

void	PetInfoWndInfo :: MakeDefault()
{
	x = 150;
	y = 100;
	w = 280;
	h = 200;
}

BOOL	PetInfoWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return FALSE;
	if (h <= 0)
		return FALSE;
	if (x < 0 || x > g_renderer->m_width - w)
		return FALSE;
	if (y < 0 || y > g_renderer->m_height - h)
		return FALSE;
	return TRUE;
}

void	HomunInfoWndInfo :: MakeDefault()
{
	x = 150;
	y = 100;
	w = 280;
	h = 180;
}

BOOL	HomunInfoWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return FALSE;
	if (h <= 0)
		return FALSE;
	if (x < 0 || x > g_renderer->m_width - w)
		return FALSE;
	if (y < 0 || y > g_renderer->m_height - h)
		return FALSE;
	return TRUE;
}

void	MerInfoWndInfo :: MakeDefault()
{
	x = 150;
	y = 100;
	w = 280;
	h = 180;
}

BOOL	MerInfoWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return FALSE;
	if (h <= 0)
		return FALSE;
	if (x < 0 || x > g_renderer->m_width - w)
		return FALSE;
	if (y < 0 || y > g_renderer->m_height - h)
		return FALSE;
	return TRUE;
}

void	SubChatWndInfo :: MakeDefault( int mode )
{
	switch ( mode )
	{
		case	MODE_ST_CHAT:
			x = 0 ;
			break;
		case	MODE_BT_CHAT:
			x = 300 ;
			break;
	}

	y = g_renderer->m_height - 95 - 24 ;
	w = 300 ;
	h = 95 ;
}

BOOL	SubChatWndInfo :: IsValidInfo()
{
	if (w <= 0)
		return false;
	if (h <= 0)
		return false;
	if (x<0 || x>g_renderer->m_width-w)
		return false;
	if (y<0 || y>g_renderer->m_height-h)
		return false;
	return TRUE;
}


void	UIWindowMgr :: MakeDefaultUIRectInfo()
{
	static bool isFirst = true;
	if (isFirst) {
		isFirst = false;
		basicInfoWndInfo.MakeDefault();
		itemWndInfo.MakeDefault();
		questWndInfo.MakeDefault();
		statusWndInfo.MakeDefault();
		equipWndInfo.MakeDefault();
		optionWndInfo.MakeDefault();
		shortenItemWndInfo.MakeDefault();
		shortenSkillWndInfo.MakeDefault();
		sayDialogWndInfo.MakeDefault();
		chooseWndInfo.MakeDefault();
		chatRoomWndInfo.MakeDefault();
		itemStoreWndInfo.MakeDefault();
		itemShopWndInfo.MakeDefault();
		itemSellWndInfo.MakeDefault();
		itemPurchaseWndInfo.MakeDefault();
		messengerGroupWndInfo.MakeDefault();
		shortCutWndInfo.MakeDefault();
		exchangeAcceptWndInfo.MakeDefault();
		skillListWndInfo.MakeDefault();
		merchantItemWndInfo.MakeDefault();
		merchantMirrorItemWndInfo.MakeDefault();
		merchantShopMakeWndInfo.MakeDefault();
		merchantItemShopWndInfo.MakeDefault();
		merchantItemMyShopWndInfo.MakeDefault();
		merchantItemPurchaseWndInfo.MakeDefault();
		itemCollectionWndInfo.MakeDefault();
		combinedCardItemCollectionWndInfo.MakeDefault();
		itemParamChangeDisplayWndInfo.MakeDefault();
		partySettingWndInfo.MakeDefault();
		detailLevelWndInfo.MakeDefault();
		guildWndInfo.MakeDefault();
		whisperWndInfo.MakeDefault();
		itemCompositionWndInfo.MakeDefault();
		itemIdentifyWndInfo.MakeDefault();
		emotionWndInfo.MakeDefault();
		emotionListWndInfo.MakeDefault();
		petInfoWndInfo.MakeDefault();



#if defined (_HOMUN)
		homunInfoWndInfo.MakeDefault();
#endif

#if defined (_MER) || defined (_MER_R2)
		merInfoWndInfo.MakeDefault();
#endif

#if defined (NEW_CHAT_WND) || defined (_MER_R2)
		subChatWndStInfo.MakeDefault(MODE_ST_CHAT);
		subChatWndBtInfo.MakeDefault(MODE_BT_CHAT);
#endif
	}
}

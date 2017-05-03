// $Id: SellScreenTCB.h 10159 2012-05-02 06:23:33Z mrimer $

#ifndef SELLSCREENTCB_H
#define SELLSCREENTCB_H

#include "SellScreen.h"

#include <FrontEndLib/SubtitleEffect.h>
#include <queue>

class CFiredCharacterCommand;
class CMonsterMessage;
class CDbPlayer;
class CSellScreenTCB : public CSellScreen3
{
protected:
	friend class CDrodScreenManager;
	CSellScreenTCB();

	virtual void OnBetweenEvents();
	virtual void OnDeactivate();
	virtual bool SetForActivate();

	virtual void PaintCabinetEdge(SDL_Surface* pDestSurface);

	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_TCB); }

	virtual int GetScreenshotY() const { return 16; }

public:
	virtual void Paint(bool bUpdateRect=true);

	static void ResetSellParameters(CDbPlayer* pPlayer);

private:
	void AddSubtitle(CFiredCharacterCommand *pFiredCommand, const Uint32 dwDuration);
	void ClearSpeech();
	void DisableWidgetsForPrompt(const bool bDisable);
	UINT GetMessageAnswer(const CMonsterMessage *pMsg);

	void ProcessCommand(CCueEvents& CueEvents, const UINT command, const UINT wX=-1, const UINT wY=-1);
	void ProcessEvents(CCueEvents& CueEvents);

	void LoadPlayerSettings(CDbPlayer *pPlayer);
	void SavePlayerSettings();

	CCurrentGame *pGame;
	CCueEvents    cueEvents;
	SUBTITLES     subtitles;
	bool          bShowingSubtitlesWithVoice;
	Uint32 dwNextSpeech; //time next speech can begin
	std::deque<CFiredCharacterCommand*> speech; //speech dialog queued to play
	CDialogWidget *pMenuDialog;

	struct ChannelInfo {UINT wChannel; bool bUsingPos; float pos[3];};
	vector<ChannelInfo> speechChannels;   //audio channels speech sounds are playing on

	int posterIndex; //which poster is displayed
	bool bPromptingUser;
};

#endif //...#ifndef SELLSCREENTCB_H
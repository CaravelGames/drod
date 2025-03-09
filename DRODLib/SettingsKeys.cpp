#include "SettingsKeys.h"

#define STR_VALUE(arg) #arg
#define DEF(key) const char key[] = STR_VALUE(key)

namespace INISection
{
	DEF(Customizing);
	DEF(Graphics);
	DEF(Localization);
	DEF(Songs);
	DEF(Startup);
	DEF(Waves);
}

namespace INIKey
{
	DEF(AllowWindowResizing);
	DEF(AlwaysFullBlit);
	DEF(AutoLogin);
	DEF(CrossfadeDuration);
	DEF(ExportSpeech);
	DEF(ExportText);
	DEF(FullScoreUpload);
	DEF(FullScreenMinimize);
	DEF(FullScreenMode);
	DEF(Keyboard);
	DEF(Language);
	DEF(LastNews);
	DEF(LogErrors);
	DEF(LogVars);
	DEF(MaxDelayForUndo);
	DEF(QuickPlayerExport);
	DEF(RoomTransitionSpeed);
	DEF(Style);
	DEF(ValidateSavesOnImport);
	DEF(Windib);
}

namespace Settings
{
	DEF(Alpha);
	DEF(AutoSave);
	DEF(AutoSaveOptions);
	DEF(AutoUndoOnDeath);
	DEF(CloudActivated);
	DEF(CloudHoldVersion);
	DEF(CloudHoldDemosVersion);
	DEF(CNetProgressIsOld);
	DEF(ConnectToInternet);
	DEF(DemoDateFormat);
	DEF(DisplayCombos);
	DEF(EnableChatInGame);
	DEF(ExportPath);
	DEF(EyeCandy);
	DEF(Fullscreen);
	DEF(Gamma);
	DEF(GEMI); //groupEditorMenuItems
	DEF(ImportImagePath);
	DEF(ImportSoundPath);
	DEF(ImportVideoPath);
	DEF(ItemTips);
	DEF(Language);
	DEF(LastNotice);
	DEF(MoveCounter);
	DEF(MovementOrderHint);
	DEF(Music);
	DEF(MusicVolume);
	DEF(NoFocusPlaysMusic);
	DEF(PlayHoldManagementDemo);
	DEF(PlaySessions);
	DEF(PuzzleMode);
	DEF(ReceiveWhispersOnlyInGame);
	DEF(RepeatRate);
	DEF(ResizableWindow);
	DEF(ShowCheckpoints);
	DEF(ShowDemosFromLevel);
	DEF(ShowErrors);
	DEF(ShowSubtitlesWithVoices);
	DEF(SoundEffects);
	DEF(SoundEffectsVolume);
	DEF(TarstuffAlpha);
	DEF(TCBTotalPlayTime);
	DEF(TotalPlayTime);
	DEF(TutorialFinished);
	DEF(UndoLevel);
	DEF(Voices);
	DEF(VoicesVolume);

	DEF(PuzzleMode_VisibilityEyeBeams);
	DEF(PuzzleMode_VisibilityReverseEyeBeams);
	DEF(PuzzleMode_VisibilityBrokenWalls);
	DEF(PuzzleMode_VisibilitySecretWalls);
	DEF(PuzzleMode_VisibilitySpiders);

	DEF(PuzzleMode_VisibilityHideAnimations);
	DEF(PuzzleMode_VisibilityHideBuildMarkers);
	DEF(PuzzleMode_VisibilityHideLighting);
	DEF(PuzzleMode_VisibilityHideWeather);

	DEF(PuzzleMode_GridOpacity);
	DEF(PuzzleMode_GridStyle);
}
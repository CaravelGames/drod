#include "CharacterCommand.h"
#include "DbSpeech.h"
#include <BackEndLib/Ports.h>

#include "../Texts/MIDs.h"

#include <map>
#include <vector>

const int ImageOverlayCommand::NO_LOOP_MAX = -1;
const int ImageOverlayCommand::DEFAULT_LAYER = 3; //last layer
const int ImageOverlayCommand::ALL_LAYERS = -2;
const int ImageOverlayCommand::NO_LAYERS = -3;
const int ImageOverlayCommand::DEFAULT_GROUP = 0;
const int ImageOverlayCommand::NO_GROUP = -1;

//*****************************************************************************
CColorText::~CColorText() { delete pText; }

CImageOverlay::~CImageOverlay() { }

//*****************************************************************************
CCharacterCommand::CCharacterCommand()
	: command((CharCommand)0)
	, x(0), y(0), w(0), h(0), flags(0), pSpeech(NULL)
{}

CCharacterCommand::CCharacterCommand(const CCharacterCommand& that, const bool bReplicateData)
	: command(that.command)
	, x(that.x), y(that.y), w(that.w), h(that.h), flags(that.flags), label(that.label), pSpeech(NULL)
{
	if (that.pSpeech)
		this->pSpeech = new CDbSpeech(*that.pSpeech, bReplicateData);
}

CCharacterCommand::~CCharacterCommand()
{
	delete this->pSpeech;
}

CCharacterCommand& CCharacterCommand::operator=(const CCharacterCommand& that)
{
	CCharacterCommand self(that);
	swap(self);
	return *this;
}
void CCharacterCommand::swap(CCharacterCommand &that) {
	std::swap(command, that.command);
	std::swap(w, that.w);
	std::swap(x, that.x);
	std::swap(y, that.y);
	std::swap(h, that.h);
	std::swap(flags, that.flags);
	std::swap(label, that.label);
	std::swap(pSpeech, that.pSpeech);
}

//*****************************************************************************
SPEAKER getSpeakerType(const MONSTERTYPE eType)
//Return: corresponding speaker enumeration for monster type, if supported.
{
	if (eType > CUSTOM_CHARACTER_FIRST)
		return Speaker_HoldCharacter;

	switch (eType)
	{
		//Character monster psuedo-types.
		case M_BEETHRO: return Speaker_Beethro;
		case M_BEETHRO_IN_DISGUISE: return Speaker_BeethroInDisguise;
		case M_GUNTHRO: return Speaker_Gunthro;
		case M_NEGOTIATOR: return Speaker_Negotiator;
		case M_NEATHER:
		case M_CITIZEN1: return Speaker_Citizen1;
		case M_CITIZEN2: return Speaker_Citizen2;
		case M_CITIZEN3: return Speaker_Citizen3;
		case M_CITIZEN4: return Speaker_Citizen4;
		case M_GOBLINKING: return Speaker_GoblinKing;
		case M_INSTRUCTOR: return Speaker_Instructor;
		case M_MUDCOORDINATOR: return Speaker_MudCoordinator;
		case M_TARTECHNICIAN: return Speaker_TarTechnician;
		case M_EYE_ACTIVE: return Speaker_EyeActive;

		//Monster types.
		case M_ROACH: return Speaker_Roach;
		case M_QROACH: return Speaker_QRoach;
		case M_REGG: return Speaker_RoachEgg;
		case M_GOBLIN: return Speaker_Goblin;
		case M_WWING: return Speaker_WWing;
		case M_EYE: return Speaker_Eye;
		case M_SERPENT: return Speaker_Serpent;
		case M_TARMOTHER: return Speaker_TarMother;
		case M_TARBABY: return Speaker_TarBaby;
		case M_BRAIN: return Speaker_Brain;
		case M_MIMIC: return Speaker_Mimic;
		case M_SPIDER: return Speaker_Spider;
		case M_SERPENTG: return Speaker_SerpentG;
		case M_SERPENTB: return Speaker_SerpentB;
		case M_ROCKGOLEM: return Speaker_RockGolem;
		case M_WATERSKIPPER: return Speaker_WaterSkipper;
		case M_SKIPPERNEST: return Speaker_WaterSkipperNest;
		case M_AUMTLICH: return Speaker_Aumtlich;
		case M_CLONE: return Speaker_Clone;
		case M_DECOY: return Speaker_Decoy;
		case M_WUBBA: return Speaker_Wubba;
		case M_SEEP: return Speaker_Seep;
		case M_STALWART: return Speaker_Stalwart;
		case M_STALWART2: return Speaker_Stalwart2;
		case M_HALPH: return Speaker_Halph;
		case M_HALPH2: return Speaker_Halph2;
		case M_SLAYER: return Speaker_Slayer;
		case M_SLAYER2: return Speaker_Slayer2;
		case M_FEGUNDO: return Speaker_Fegundo;
		case M_FEGUNDOASHES: return Speaker_FegundoAshes;
		case M_GUARD: return Speaker_Guard;
		case M_MUDMOTHER: return Speaker_MudMother;
		case M_MUDBABY: return Speaker_MudBaby;
		case M_GELMOTHER: return Speaker_GelMother;
		case M_GELBABY: return Speaker_GelBaby;
		case M_CITIZEN: return Speaker_Citizen;
		case M_ARCHITECT: return Speaker_Architect;
		case M_ROCKGIANT: return Speaker_RockGiant;
		case M_CONSTRUCT: return Speaker_Construct;
		case M_GENTRYII: return Speaker_Gentryii;
		case M_TEMPORALCLONE: return Speaker_TemporalClone;
		case M_FLUFFBABY: return Speaker_FluffBaby;

		default: return Speaker_None;
	}
}

//*****************************************************************************
UINT getSpeakerNameText(const UINT wSpeaker, std::string& color)
//Returns: MID corresponding to name text for this speaker.
//Outputs color string for HTML text.
{
	UINT dwSpeakerTextID;
	switch (wSpeaker)
	{
		//Custom speakers.
		case Speaker_BeethroInDisguise: dwSpeakerTextID = MID_BeethroInDisguise; color = "1010A0"; break;
		case Speaker_Beethro: dwSpeakerTextID = MID_Beethro; color = "0000A0"; break;
		case Speaker_Gunthro: dwSpeakerTextID = MID_Gunthro; color = "000080"; break;
		case Speaker_Negotiator: dwSpeakerTextID = MID_Negotiator; color = "0000FF"; break;
		case Speaker_Citizen1: dwSpeakerTextID = MID_Citizen1; color = "800080"; break;
		case Speaker_Citizen2: dwSpeakerTextID = MID_Citizen2; color = "0000A0"; break;
		case Speaker_Citizen3: dwSpeakerTextID = MID_Citizen3; color = "800080"; break;
		case Speaker_Citizen4: dwSpeakerTextID = MID_Citizen4; color = "0000A0"; break;
		case Speaker_GoblinKing: dwSpeakerTextID = MID_GoblinKing; color = "004000"; break;
		case Speaker_Instructor: dwSpeakerTextID = MID_Instructor; color = "0000FF"; break;
		case Speaker_MudCoordinator: dwSpeakerTextID = MID_MudCoordinator; color = "800000"; break;
		case Speaker_TarTechnician: dwSpeakerTextID = MID_TarTechnician; color = "0000FF"; break;
		case Speaker_Custom: dwSpeakerTextID = MID_Custom; color = "202020"; break;
		case Speaker_None: dwSpeakerTextID = MID_None; color = "000000"; break;
		case Speaker_Self: dwSpeakerTextID = MID_Self; color = "000000"; break;
		case Speaker_Player: dwSpeakerTextID = MID_Player; color = "0000A0"; break;
		case Speaker_EyeActive: dwSpeakerTextID = MID_EvilEyeActive; color = "FF0000"; break;

		//Monster speakers.
		case Speaker_Architect: dwSpeakerTextID = MID_Architect; color = "E0E040"; break;
		case Speaker_Halph: dwSpeakerTextID = MID_Halph; color = "804020"; break;
		case Speaker_Halph2: dwSpeakerTextID = MID_Halph2; color = "804020"; break;
		case Speaker_Slayer: dwSpeakerTextID = MID_Slayer; color = "B00060"; break;
		case Speaker_Slayer2: dwSpeakerTextID = MID_Slayer2; color = "B00060"; break;
		case Speaker_Goblin: dwSpeakerTextID = MID_Goblin; color = "008000"; break;
		case Speaker_RockGolem: dwSpeakerTextID = MID_RockGolem; color = "800000"; break;
		case Speaker_Guard: dwSpeakerTextID = MID_Guard; color = "400000"; break;
		case Speaker_Stalwart: dwSpeakerTextID = MID_Stalwart; color = "D0D080"; break;
		case Speaker_Stalwart2: dwSpeakerTextID = MID_Stalwart2; color = "806040"; break;
		case Speaker_Roach: dwSpeakerTextID = MID_Roach; color = "202020"; break;
		case Speaker_QRoach: dwSpeakerTextID = MID_RoachQueen; color = "404040"; break;
		case Speaker_RoachEgg: dwSpeakerTextID = MID_Roach; color = "202020"; break;
		case Speaker_WWing: dwSpeakerTextID = MID_Wraithwing; color = "000000"; break;
		case Speaker_Eye: dwSpeakerTextID = MID_EvilEye; color = "0000FF"; break;
		case Speaker_Serpent: dwSpeakerTextID = MID_Serpent; color = "FF0000"; break;
		case Speaker_TarMother: dwSpeakerTextID = MID_TarMother; color = "0000FF"; break;
		case Speaker_TarBaby: dwSpeakerTextID = MID_TarBaby; color = "0000FF"; break;
		case Speaker_Brain: dwSpeakerTextID = MID_Brain; color = "FF0000"; break;
		case Speaker_Mimic: dwSpeakerTextID = MID_Mimic; color = "0000A0"; break;
		case Speaker_Spider: dwSpeakerTextID = MID_Spider; color = "101010"; break;
		case Speaker_SerpentG: dwSpeakerTextID = MID_GreenSerpent; color = "00FF00"; break;
		case Speaker_SerpentB: dwSpeakerTextID = MID_BlueSerpent; color = "0000FF"; break;
		case Speaker_WaterSkipper: dwSpeakerTextID = MID_Waterskipper; color = "000000"; break;
		case Speaker_WaterSkipperNest: dwSpeakerTextID = MID_WaterskipperNest; color = "000000"; break;
		case Speaker_Aumtlich: dwSpeakerTextID = MID_Aumtlich; color = "303030"; break;
		case Speaker_Clone: dwSpeakerTextID = MID_Clone; color = "0000A0"; break;
		case Speaker_Decoy: dwSpeakerTextID = MID_Decoy; color = "000080"; break;
		case Speaker_Wubba: dwSpeakerTextID = MID_Wubba; color = "000000"; break;
		case Speaker_Seep: dwSpeakerTextID = MID_Seep; color = "000000"; break;
		case Speaker_Fegundo: dwSpeakerTextID = MID_Fegundo; color = "800000"; break;
		case Speaker_FegundoAshes: dwSpeakerTextID = MID_Fegundo; color = "800000"; break;
		case Speaker_MudMother: dwSpeakerTextID = MID_MudMother; color = "FF0000"; break;
		case Speaker_MudBaby: dwSpeakerTextID = MID_MudBaby; color = "FF0000"; break;
		case Speaker_GelMother: dwSpeakerTextID = MID_GelMother; color = "00FF00"; break;
		case Speaker_GelBaby: dwSpeakerTextID = MID_GelBaby; color = "00FF00"; break;
		case Speaker_Citizen: dwSpeakerTextID = MID_Citizen; color = "D0D000"; break;
		case Speaker_RockGiant: dwSpeakerTextID = MID_RockGiant; color = "800000"; break;
		case Speaker_Construct: dwSpeakerTextID = MID_Construct; color = "402020"; break;
		case Speaker_Gentryii: dwSpeakerTextID = MID_Gentryii; color = "303030"; break;
		case Speaker_TemporalClone: dwSpeakerTextID = MID_TemporalClone; color = "0000A0"; break;
		case Speaker_FluffBaby: dwSpeakerTextID = MID_FluffBaby; color = "000000"; break;

		default: dwSpeakerTextID = MID_None; color = "FF0000"; break;
	}
	return dwSpeakerTextID;
}

//*****************************************************************************
typedef std::map<string, ImageOverlayCommand::IOC> CommandMap;
CommandMap commandMap;

ImageOverlayCommand::IOC matchCommand(const char* pText, UINT& index)
{
	ASSERT(pText);

	if (commandMap.empty()) {
		commandMap[string("addx")] = ImageOverlayCommand::AddX;
		commandMap[string("addy")] = ImageOverlayCommand::AddY;
		commandMap[string("cancelall")] = ImageOverlayCommand::CancelAll;
		commandMap[string("cancelgroup")] = ImageOverlayCommand::CancelGroup;
		commandMap[string("cancellayer")] = ImageOverlayCommand::CancelLayer;
		commandMap[string("center")] = ImageOverlayCommand::Center;
		commandMap[string("display ")] = ImageOverlayCommand::DisplayDuration;
		commandMap[string("displayms")] = ImageOverlayCommand::DisplayDuration; //deprecated
		commandMap[string("displayrect")] = ImageOverlayCommand::DisplayRect;
		commandMap[string("displayrectmodify")] = ImageOverlayCommand::DisplayRectModify;
		commandMap[string("displaysize")] = ImageOverlayCommand::DisplaySize;
		commandMap[string("displayturns")] = ImageOverlayCommand::TurnDuration;
		commandMap[string("fadetoalpha")] = ImageOverlayCommand::FadeToAlpha;
		commandMap[string("group")] = ImageOverlayCommand::Group;
		commandMap[string("grow")] = ImageOverlayCommand::Grow;
		commandMap[string("jitter")] = ImageOverlayCommand::Jitter;
		commandMap[string("layer")] = ImageOverlayCommand::Layer;
		commandMap[string("loop")] = ImageOverlayCommand::Loop;
		commandMap[string("move ")] = ImageOverlayCommand::Move;
		commandMap[string("moveto")] = ImageOverlayCommand::MoveTo;
		commandMap[string("pfadetoalpha")] = ImageOverlayCommand::ParallelFadeToAlpha;
		commandMap[string("pgrow")] = ImageOverlayCommand::ParallelGrow;
		commandMap[string("pjitter")] = ImageOverlayCommand::ParallelJitter;
		commandMap[string("pmove ")] = ImageOverlayCommand::ParallelMove;
		commandMap[string("pmoveto")] = ImageOverlayCommand::ParallelMoveTo;
		commandMap[string("protate")] = ImageOverlayCommand::ParallelRotate;
		commandMap[string("rotate")] = ImageOverlayCommand::Rotate;
		commandMap[string("scale")] = ImageOverlayCommand::Scale;
		commandMap[string("setalpha")] = ImageOverlayCommand::SetAlpha;
		commandMap[string("setangle")] = ImageOverlayCommand::SetAngle;
		commandMap[string("setx")] = ImageOverlayCommand::SetX;
		commandMap[string("sety")] = ImageOverlayCommand::SetY;
		commandMap[string("srcxy")] = ImageOverlayCommand::SrcXY;
	}

	for (CommandMap::const_iterator it=commandMap.begin(); it!=commandMap.end(); ++it) {
		const string& command = it->first;
		if (!_strnicmp(command.c_str(), pText + index, command.size())) {
			index += command.size();
			return it->second;
		}
	}

	return ImageOverlayCommand::Invalid;
}

#define skipWhitespace while (pos < textLength && isspace(pText[pos])) ++pos

bool parseNumber(
	const char* pText, const UINT textLength,
	UINT& pos, int& val) //(out)
{
	skipWhitespace;

	if (pText[pos] == '$') {
		//support parse validation only
		++pos;
		while (pos < textLength && pText[pos] != '$')
			++pos;
		if (pos == textLength)
			return false; //no matching '$'
		++pos;

		val = 0;
		return true;
	}

	const UINT oldPos = pos;
	if (pos < textLength && pText[pos] == '-')
		++pos;
	while (pos < textLength && isdigit(pText[pos]))
		++pos;
	if (pos == oldPos)
		return false;

	val = atoi(pText+oldPos);
	return true;
}

bool CImageOverlay::parse(const WSTRING& wtext, ImageOverlayCommands& commands)
{
	commands.clear();

	if (wtext.empty())
		return true;

	const string text = UnicodeToUTF8(wtext);
	const UINT textLength = text.length();
	const char *pText = (const char*)text.c_str();
	UINT pos=0;

	while (pos < textLength) {
		skipWhitespace;
		if (pos >= textLength)
			return true;

		ImageOverlayCommand::IOC eCommand = matchCommand(pText, pos);
		if (eCommand == ImageOverlayCommand::Invalid)
			return false;

		int val[4] = {0,0,0,0};
		int arg_index=0;
		switch (eCommand) {
			case ImageOverlayCommand::DisplayRect:
			case ImageOverlayCommand::DisplayRectModify:
				//four arguments
				if (!parseNumber(pText, textLength, pos, val[arg_index++]))
					return false;
			//no break
			case ImageOverlayCommand::Move:
			case ImageOverlayCommand::ParallelMove:
			case ImageOverlayCommand::MoveTo:
			case ImageOverlayCommand::ParallelMoveTo:
				//three arguments
				if (!parseNumber(pText, textLength, pos, val[arg_index++]))
					return false;
			//no break
			case ImageOverlayCommand::DisplaySize:
			case ImageOverlayCommand::FadeToAlpha:
			case ImageOverlayCommand::Grow:
			case ImageOverlayCommand::Jitter:
			case ImageOverlayCommand::ParallelFadeToAlpha:
			case ImageOverlayCommand::ParallelGrow:
			case ImageOverlayCommand::ParallelJitter:
			case ImageOverlayCommand::ParallelRotate:
			case ImageOverlayCommand::Rotate:
			case ImageOverlayCommand::SrcXY:
				//two arguments
				if (!parseNumber(pText, textLength, pos, val[arg_index++]))
					return false;
			//no break
			default:
				//one argument
				if (!parseNumber(pText, textLength, pos, val[arg_index++]))
					return false;
			//no break
			case ImageOverlayCommand::CancelAll:
			case ImageOverlayCommand::Center:
				//no arguments
			break;
		}
		commands.push_back(ImageOverlayCommand(eCommand, val));
	}

	return true;
}

#undef skipWhitespace

int CImageOverlay::clearsImageOverlays() const
{
	for (ImageOverlayCommands::const_iterator it=commands.begin();
			it!=commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::CancelAll)
			return ImageOverlayCommand::ALL_LAYERS;
		if (c.type == ImageOverlayCommand::CancelLayer)
			return c.val[0];
	}

	return ImageOverlayCommand::NO_LAYERS;
}

int CImageOverlay::clearsImageOverlayGroup() const
{
	for (ImageOverlayCommands::const_iterator it = commands.begin();
		it != commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::CancelGroup)
			return c.val[0];
	}

	return ImageOverlayCommand::NO_GROUP;
}

int CImageOverlay::getLayer() const
{
	for (ImageOverlayCommands::const_iterator it=commands.begin();
			it!=commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::Layer)
			return c.val[0];
	}

	return ImageOverlayCommand::DEFAULT_LAYER;
}

int CImageOverlay::getGroup() const
{
	for (ImageOverlayCommands::const_iterator it = commands.begin();
		it != commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::Group)
			return c.val[0];
	}

	return ImageOverlayCommand::DEFAULT_GROUP;
}

bool CImageOverlay::loopsForever() const
{
	for (ImageOverlayCommands::const_iterator it=commands.begin();
			it!=commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::Loop && c.val[0] == ImageOverlayCommand::NO_LOOP_MAX)
			return true;
	}

	return false;
}

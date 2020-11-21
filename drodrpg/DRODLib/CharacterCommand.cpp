#include "CharacterCommand.h"
#include "DbSpeech.h"
#include <BackEndLib/Ports.h>

#include "../Texts/MIDs.h"

#include <map>
#include <vector>

const UINT NPC_DEFAULT_SWORD = UINT(-1);

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

CCharacterCommand::~CCharacterCommand() {
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
bool CCharacterCommand::IsEachEventCommand(CharCommand command)
{
	switch (command) {
		case CCharacterCommand::CC_EachAttack:
		case CCharacterCommand::CC_EachDefend:
		case CCharacterCommand::CC_EachUse:
		case CCharacterCommand::CC_EachVictory:
			return true;
		default:
			return false;
	}

	return false;
}

//*****************************************************************************
bool addWithClamp(int& val, const int operand)
//Multiplies two integers, ensuring the product doesn't overflow.
//
//Returns: false if actual result can't be given (i.e. value overflowed), otherwise true
{
	const double newVal = (double)val + operand;
	if (newVal > INT_MAX)
	{
		val = INT_MAX;
		return false;
	}
	if (newVal < INT_MIN)
	{
		val = INT_MIN;
		return false;
	}
	val += operand;
	return true;
}

//*****************************************************************************
SPEAKER getSpeakerType(const MONSTERTYPE eType)
//Return: corresponding speaker enumeration for monster type, if supported.
{
	switch (eType)
	{
		//Character monster psuedo-types.
		case M_BEETHRO: return Speaker_Beethro;
		case M_BEETHRO_IN_DISGUISE: return Speaker_BeethroInDisguise;
		case M_NEGOTIATOR: return Speaker_Negotiator;
		case M_CITIZEN1: return Speaker_Citizen1;
		case M_CITIZEN2: return Speaker_Citizen2;
		case M_CITIZEN3: return Speaker_Citizen3;
		case M_CITIZEN4: return Speaker_Citizen4;
		case M_INSTRUCTOR: return Speaker_Instructor;
		case M_MUDCOORDINATOR: return Speaker_MudCoordinator;
		case M_TARTECHNICIAN: return Speaker_TarTechnician;
		case M_STALWART: return Speaker_Stalwart;

		//Monster types.
		case M_ROACH: return Speaker_Roach;
		case M_QROACH: return Speaker_QRoach;
		case M_REGG: return Speaker_RoachEgg;
		case M_GOBLIN: return Speaker_Goblin;
		case M_GOBLINKING: return Speaker_GoblinKing;
		case M_WWING: return Speaker_WWing;
		case M_EYE: return Speaker_Eye;
		case M_MADEYE: return Speaker_MadEye;
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
		case M_PIRATE: return Speaker_Pirate;
		case M_HALPH:
		case M_NEATHER: return Speaker_Halph;
		case M_SLAYER: return Speaker_Slayer;
		case M_FEGUNDO: return Speaker_Fegundo;
		case M_FEGUNDOASHES: return Speaker_FegundoAshes;
		case M_GUARD: return Speaker_Guard;
		case M_MUDMOTHER: return Speaker_MudMother;
		case M_MUDBABY: return Speaker_MudBaby;
		case M_GELMOTHER: return Speaker_GelMother;
		case M_GELBABY: return Speaker_GelBaby;
		case M_CITIZEN: return Speaker_Citizen;
		case M_ROCKGIANT: return Speaker_RockGiant;

		default: return Speaker_None;
	}
}

//*****************************************************************************
UINT getSpeakerNameText(const UINT wSpeaker, string& color)
//Returns: MID corresponding to name text for this speaker.
//Outputs color string for HTML text.
{
	UINT dwSpeakerTextID;
	switch (wSpeaker)
	{
		//Custom speakers.
		case Speaker_BeethroInDisguise: dwSpeakerTextID = MID_BeethroInDisguise; color = "1010A0"; break;
		case Speaker_Beethro: dwSpeakerTextID = MID_Beethro; color = "0000A0"; break;
		case Speaker_Negotiator: dwSpeakerTextID = MID_Negotiator; color = "0000FF"; break;
		case Speaker_Citizen1: dwSpeakerTextID = MID_Citizen1; color = "800080"; break;
		case Speaker_Citizen2: dwSpeakerTextID = MID_Citizen2; color = "0000A0"; break;
		case Speaker_Citizen3: dwSpeakerTextID = MID_Citizen3; color = "800080"; break;
		case Speaker_Citizen4: dwSpeakerTextID = MID_Citizen4; color = "0000A0"; break;
		case Speaker_Instructor: dwSpeakerTextID = MID_Instructor; color = "0000FF"; break;
		case Speaker_MudCoordinator: dwSpeakerTextID = MID_MudCoordinator; color = "800000"; break;
		case Speaker_TarTechnician: dwSpeakerTextID = MID_TarTechnician; color = "0000FF"; break;
		case Speaker_Custom: dwSpeakerTextID = MID_Custom; color = "202020"; break;
		case Speaker_None: dwSpeakerTextID = MID_None; color = "000000"; break;
		case Speaker_Self: dwSpeakerTextID = MID_Self; color = "000000"; break;
		case Speaker_Player: dwSpeakerTextID = MID_Player; color = "0000A0"; break;
		case Speaker_Stalwart: dwSpeakerTextID = MID_Stalwart; color = "A0A080"; break;

		//Monster speakers.
		case Speaker_Halph: dwSpeakerTextID = MID_Halph; color = "804020"; break;
		case Speaker_Slayer: dwSpeakerTextID = MID_Slayer; color = "B00060"; break;
		case Speaker_Goblin: dwSpeakerTextID = MID_Goblin; color = "008000"; break;
		case Speaker_GoblinKing: dwSpeakerTextID = MID_GoblinKing; color = "004000"; break;
		case Speaker_RockGolem: dwSpeakerTextID = MID_StoneGolem; color = "800000"; break;
		case Speaker_Guard: dwSpeakerTextID = MID_Guard; color = "400000"; break;
		case Speaker_Pirate: dwSpeakerTextID = MID_Pirate; color = "A0A040"; break;
		case Speaker_Roach: dwSpeakerTextID = MID_Roach; color = "202020"; break;
		case Speaker_QRoach: dwSpeakerTextID = MID_RoachQueen; color = "404040"; break;
		case Speaker_RoachEgg: dwSpeakerTextID = MID_Roach; color = "202020"; break;
		case Speaker_WWing: dwSpeakerTextID = MID_Wraithwing; color = "000000"; break;
		case Speaker_Eye: dwSpeakerTextID = MID_EvilEye; color = "0000FF"; break;
		case Speaker_MadEye: dwSpeakerTextID = MID_MadEye; color = "FF0000"; break;
		case Speaker_Serpent: dwSpeakerTextID = MID_Serpent; color = "FF0000"; break;
		case Speaker_TarMother: dwSpeakerTextID = MID_TarMother; color = "0000FF"; break;
		case Speaker_TarBaby: dwSpeakerTextID = MID_TarBaby; color = "0000FF"; break;
		case Speaker_Brain: dwSpeakerTextID = MID_Brain; color = "FF0000"; break;
		case Speaker_Mimic: dwSpeakerTextID = MID_Mimic; color = "0000A0"; break;
		case Speaker_Spider: dwSpeakerTextID = MID_Spider; color = "101010"; break;
		case Speaker_SerpentG: dwSpeakerTextID = MID_GreenSerpent; color = "00FF00"; break;
		case Speaker_SerpentB: dwSpeakerTextID = MID_BlueSerpent; color = "0000FF"; break;
		case Speaker_WaterSkipper: dwSpeakerTextID = MID_Ant; color = "000000"; break;
		case Speaker_WaterSkipperNest: dwSpeakerTextID = MID_AntHill; color = "000000"; break;
		case Speaker_Aumtlich: dwSpeakerTextID = MID_Zombie; color = "303030"; break;
		case Speaker_Clone: dwSpeakerTextID = MID_Clone; color = "0000A0"; break;
		case Speaker_Decoy: dwSpeakerTextID = MID_Decoy; color = "000080"; break;
		case Speaker_Wubba: dwSpeakerTextID = MID_Wubba; color = "000000"; break;
		case Speaker_Seep: dwSpeakerTextID = MID_Ghost; color = "000000"; break;
		case Speaker_Fegundo: dwSpeakerTextID = MID_Phoenix; color = "800000"; break;
		case Speaker_FegundoAshes: dwSpeakerTextID = MID_Phoenix; color = "800000"; break;
		case Speaker_MudMother: dwSpeakerTextID = MID_MudMother; color = "FF0000"; break;
		case Speaker_MudBaby: dwSpeakerTextID = MID_MudBaby; color = "FF0000"; break;
		case Speaker_GelMother: dwSpeakerTextID = MID_GelMother; color = "00FF00"; break;
		case Speaker_GelBaby: dwSpeakerTextID = MID_GelBaby; color = "00FF00"; break;
		case Speaker_Citizen: dwSpeakerTextID = MID_Citizen; color = "A0A000"; break;
		case Speaker_RockGiant: dwSpeakerTextID = MID_Splitter; color = "800000"; break;
		default: dwSpeakerTextID = MID_None; color = "FF0000"; break;
	}
	return dwSpeakerTextID;
}

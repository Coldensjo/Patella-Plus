// Creatures.cpp: reconstructed from Tibia.exe 7.72.
//
// The creature table, the player's statistics and walk, the battle list and
// the VIP list. Functions in line order. Where the binary order differs: the
// compiler emits a function only after the bodies of the functions it calls,
// so the ones that call a function defined further down (lines 278-592,
// InitCreatures at line 1366 and SetPosition at line 108) come after
// ResetCreatures (line 1414) in the binary. The functions without exception
// frame have no line evidence; they come first in the binary, before the
// line-142 function, and keep that place and order here
// (notes/decisions.md).

#include "Creatures.h"
#include "types/GraphicsTypes.h" // TRGBColor
#include "Map.h"
#include "Objects.h"
#include "Preferences.h"
#include "sysinc.h"
#include "common.h"
#include "platform/Platform.h"
#include "types/FixedString.h"
#include "types/Time32.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// The data is TCreatures' and TPlayerState's (Creatures.h, issue #106).

static_assert(sizeof(TCreature) == 0x9c, "TCreature is 0x9c bytes");
static_assert(offsetof(TCreature, SkullTime) == 0x98 && sizeof(TCreature::SkullTime) == 4, "TCreature's 4-byte skull time (Time32, issue #62)");
static_assert(sizeof(TBuddyEntry) == 0x2c, "TBuddyEntry is 0x2c bytes");
// Name a TFixedString (issue #57): the original's offsets.
static_assert(offsetof(TBuddyEntry, Name) == 0x04 && offsetof(TBuddyEntry, Online) == 0x22
		&& offsetof(TBuddyEntry, LoginAction) == 0x24 && offsetof(TBuddyEntry, Icon) == 0x28,
	"TBuddyEntry's fields");
// Name a TFixedString and Colors a std::array (issue #60); the globals std::arrays.
static_assert(offsetof(TCreature, Name) == 0x04 && sizeof(TCreature::Name) == 32 && offsetof(TCreature, Colors) == 0x64
		&& sizeof(TCreature::Colors) == 16,
	"TCreature's arrays");
// The globals' arrays: TCreatures' and TPlayerState's static_asserts (Creatures.h).

patella::TCreatures::TCreatures(TCreaturesPlayer &PlayerStateRef, TCreaturesGUI &GUIRef)
	: PlayerState(PlayerStateRef), GUI(GUIRef)
{
}

patella::TPlayerState::TPlayerState(TObjectTypes &ObjectTypesRef, TMap &MapRef, TCreatures &CreaturesRef,
	TCreaturesGUI &GUIRef)
	: ObjectTypes(ObjectTypesRef), Map(MapRef), Creatures(CreaturesRef), GUI(GUIRef)
{
}

// Health in percent.
TRGBColor TCreature::GetHealthColor() const
{
	TRGBColor Color;
	if (In(Health, 0, 3)) {
		Color.R = 0x60;
		Color.G = 0;
		Color.B = 0;
	} else if (In(Health, 4, 9)) {
		Color.R = 0xc0;
		Color.G = 0;
		Color.B = 0;
	} else if (In(Health, 10, 29)) {
		Color.R = 0xc0;
		Color.G = 0x30;
		Color.B = 0x30;
	} else if (In(Health, 30, 59)) {
		Color.R = 0xc0;
		Color.G = 0xc0;
		Color.B = 0;
	} else if (In(Health, 60, 94)) {
		Color.R = 0x60;
		Color.G = 0xc0;
		Color.B = 0x60;
	} else {
		Color.R = 0;
		Color.G = 0xc0;
		Color.B = 0;
	}
	return Color;
}

bool TCreature::IsMarked() const
{
	return GetMilliseconds() <= MarkTime;
}

// With Delay a removed skull is kept and UpdateSkulls takes it away four
// seconds later.
void TCreature::SetSkull(int Skull, bool Delay)
{
	if (Delay && this->PKFlag != PK_NONE && Skull == PK_NONE) {
		SkullTime = StoreTime32(patella::platform::Platform().Clock.WallSeconds());
		return;
	}
	SkullTime = 0;
	this->PKFlag = Skull;
}

int patella::TPlayerState::GetNumberOfPlayerSteps() const
{
	return NumberOfPlayerSteps;
}

// The map centre (GetMapPosition) plus the step that the server has not
// confirmed yet.
void patella::TPlayerState::GetPlayerPosition(int *x, int *y, int *z) const
{
	Map.GetMapPosition(x, y, z);
	*x += PlayerStepDX;
	*y += PlayerStepDY;
}

void patella::TPlayerState::ClearPlayerSteps()
{
	DestinationX = 0;
	NumberOfPlayerSteps = 0;
	CurrentPlayerStep = 0;
	NewPlayerDestination = false;
}

void patella::TCreatures::HideAllCreatures()
{
	for (int i = 0; i < CREATURES; i++)
		Creatures[i].Visible = false;
}

void patella::TCreatures::SetBeat(int Value)
{
	Beat = Value;
}

bool patella::TCreatures::IsPlayer(uint32 CreatureID)
{
	return CreatureID < 0x40000000;
}

void patella::TPlayerState::SetPlayerID(uint32 CreatureID)
{
	PlayerID = CreatureID;
}

void patella::TPlayerState::SetPlayerHealth(int Health, int MaxHealth)
{
	PlayerHealth = Health;
	PlayerMaxHealth = MaxHealth;
}

void patella::TPlayerState::SetPlayerMagicLevel(int Level, int Progress)
{
	PlayerMagicLevel = Level;
	PlayerMagicLevelProgress = Progress;
}

void patella::TPlayerState::SetPlayerMana(int Mana, int MaxMana)
{
	PlayerMana = Mana;
	PlayerMaxMana = MaxMana;
}

void patella::TPlayerState::SetPlayerSoul(int Soul)
{
	PlayerSoul = Soul;
}

void patella::TPlayerState::SetPlayerCapacity(int Capacity)
{
	PlayerCapacity = Capacity;
}

void patella::TPlayerState::SetPlayerState(int State)
{
	PlayerState = State;
}

uint32 patella::TPlayerState::GetPlayerID() const
{
	return PlayerID;
}

int patella::TPlayerState::GetPlayerHealth() const
{
	return PlayerHealth;
}

int patella::TPlayerState::GetPlayerMaxHealth() const
{
	return PlayerMaxHealth;
}

int patella::TPlayerState::GetPlayerExperience() const
{
	return PlayerExperience;
}

int patella::TPlayerState::GetPlayerLevel() const
{
	return PlayerLevel;
}

int patella::TPlayerState::GetPlayerLevelProgress() const
{
	return PlayerLevelProgress;
}

int patella::TPlayerState::GetPlayerMana() const
{
	return PlayerMana;
}

int patella::TPlayerState::GetPlayerMaxMana() const
{
	return PlayerMaxMana;
}

int patella::TPlayerState::GetPlayerSoul() const
{
	return PlayerSoul;
}

int patella::TPlayerState::GetPlayerMagicLevel() const
{
	return PlayerMagicLevel;
}

int patella::TPlayerState::GetPlayerMagicLevelProgress() const
{
	return PlayerMagicLevelProgress;
}

int patella::TPlayerState::GetPlayerCapacity() const
{
	return PlayerCapacity;
}

// The state bits in the order of the protocol's icons.
bool patella::TPlayerState::IsPlayerPoisoned() const
{
	return (PlayerState & 1) != 0;
}

bool patella::TPlayerState::IsPlayerBurning() const
{
	return (PlayerState >> 1 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerElectrified() const
{
	return (PlayerState >> 2 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerDrunk() const
{
	return (PlayerState >> 3 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerManaShielded() const
{
	return (PlayerState >> 4 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerParalyzed() const
{
	return (PlayerState >> 5 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerHasted() const
{
	return (PlayerState >> 6 & 1) != 0;
}

bool patella::TPlayerState::IsPlayerInFight() const
{
	return (PlayerState >> 7 & 1) != 0;
}

void patella::TCreatures::SetAttackID(uint32 CreatureID)
{
	AttackID = CreatureID;
	FollowID = 0;
}

void patella::TCreatures::SetFollowID(uint32 CreatureID)
{
	FollowID = CreatureID;
	AttackID = 0;
}

void patella::TCreatures::SetHighlightID(uint32 CreatureID)
{
	HighlightID = CreatureID;
}

uint32 patella::TCreatures::GetAttackID() const
{
	return AttackID;
}

uint32 patella::TCreatures::GetFollowID() const
{
	return FollowID;
}

uint32 patella::TCreatures::GetHighlightID() const
{
	return HighlightID;
}

// Inlined into SetBuddyState and RemoveBuddy.
int patella::TCreatures::GetBuddyPosition(uint32 CharacterID) const
{
	for (int i = 0; i < NumberOfBuddies; i++) {
		if (Buddies[i].CharacterID == CharacterID)
			return i;
	}
	return -1;
}

bool patella::TCreatures::IsBuddy(const char *Name) const
{
	for (int i = 0; i < NumberOfBuddies; i++) {
		if (MsvcStricmp(Name, Buddies[i].Name) == 0)
			return true;
	}
	return false;
}

// --- Creatures -------------------------------------------------------------

void TCreature::SetPosition(int x, int y, int z)
{
	if (RealX != x || RealY != y || RealZ != z) {
		RealX = x;
		RealY = y;
		RealZ = z;
		DeltaX = 0;
		DeltaY = 0;
		Moving = false;
		CheckMonsterHint(this);
	}
}

// For the player the step not confirmed yet is already drawn.
int TCreature::GetDeltaX() const
{
	const patella::TPlayerState &Player = patella::bridge::PlayerState();
	if (CreatureID == Player.PlayerID)
		return Player.PlayerStepDX * 32 + DeltaX;
	return DeltaX;
}

int TCreature::GetDeltaY() const
{
	const patella::TPlayerState &Player = patella::bridge::PlayerState();
	if (CreatureID == Player.PlayerID)
		return Player.PlayerStepDY * 32 + DeltaY;
	return DeltaY;
}

int TCreature::GetDrawDirection() const
{
	const patella::TPlayerState &Player = patella::bridge::PlayerState();
	if (CreatureID == Player.PlayerID && (Player.PlayerStepDX != 0 || Player.PlayerStepDY != 0))
		return Player.PlayerStepDirection;
	if (Moving)
		return MoveDirection;
	return Direction;
}

// Starts the animation of a step by (dx,dy) onto a ground with the given
// waypoints. For the player a step that StartPlayerStep predicted only clears
// the prediction (the animation runs already); any other step counts as the
// next one of the walk. A diagonal step blocks the next step for three
// times the unrounded duration.
void TCreature::StartMovement(int dx, int dy, int Waypoints)
{
	if (dx > 0)
		Direction = DIRECTION_EAST;
	else if (dx < 0)
		Direction = DIRECTION_WEST;
	else if (dy < 0)
		Direction = DIRECTION_NORTH;
	else if (dy > 0)
		Direction = DIRECTION_SOUTH;
	patella::TPlayerState &Player = patella::bridge::PlayerState();
	if (Waypoints != 0 && CreatureID == Player.PlayerID) {
		const bool Predicted = Player.PlayerStepDX == dx && Player.PlayerStepDY == dy;
		Player.PlayerStepDX = 0;
		Player.PlayerStepDY = 0;
		if (Predicted)
			return;
		Player.CurrentPlayerStep++;
	}
	MoveDirection = Direction;
	int Duration;
	if (Speed > 0)
		Duration = Waypoints * 1000 / Speed;
	else
		Duration = 1000;
	// Beat is the game login's word (Communication.cpp ReceiveGameLogin): 0 from the server trapped here in the
	// original, whose catch wrapper made it a TMyException (issue #67).
	const int Beat = patella::bridge::Creatures().Beat;
	Duration = (Beat - 1 + Duration) / PATELLA_DIVISOR(Beat) * Beat;
	DeltaX = -dx * 32;
	DeltaY = -dy * 32;
	StepX = abs(dx) * 32;
	StepY = abs(dy) * 32;
	StepDuration = Duration;
	StepEnd = GetMilliseconds() + Duration;
	if (dx != 0 && dy != 0) {
		if (Speed > 0)
			Duration = Waypoints * 3000 / Speed;
		else
			Duration = 1000;
	}
	NextStep = GetMilliseconds() + Duration;
	Moving = true;
}

void TCreature::SetMark(int Value)
{
	Mark = Value;
	MarkTime = GetMilliseconds() + 1000;
}

// Phase 1 stands still; walking runs through phases 2 to
// AvailablePhases, one every 8 pixels.
int TCreature::GetAnimationPhase(int AvailablePhases) const
{
	PATELLA_ASSERT_VALUES(AvailablePhases>=1, AvailablePhases);
	if (AvailablePhases == 1)
		return 1;
	if (DeltaX == 0 && DeltaY == 0 && GetMilliseconds() > StepEnd)
		return 1;
	int Distance = abs(DeltaX);
	if (Distance <= abs(DeltaY))
		Distance = abs(DeltaY);
	return Distance / 8 % (AvailablePhases - 1) + 2;
}

// --- The player's walk -----------------------------------------------------

int patella::TPlayerState::GetPlayerStep(int Step) const
{
	PATELLA_ASSERT_VALUES(In(Step,0,NumberOfPlayerSteps-1), Step, NumberOfPlayerSteps-1);
	return PlayerSteps[Step];
}

// Starts the current step of the walk at once, before the server confirms
// it, if the field is free and not more than one level higher. The map
// functions take screen coordinates; the player stands at (8,6).
void patella::TPlayerState::StartPlayerStep()
{
	PATELLA_ASSERT_VALUES(CurrentPlayerStep<NumberOfPlayerSteps, CurrentPlayerStep);
	if (NewPlayerDestination || PlayerHealth == 0)
		return;
	TCreature *Player = Creatures.GetCreature(GetPlayerID());
	if (Player == nullptr)
		return;
	if (PlayerStepDX != 0 || PlayerStepDY != 0) {
		Player->Moving = true;
		return;
	}
	int dx = 0;
	int dy = 0;
	switch (PlayerSteps[CurrentPlayerStep]) {
		// clang-format off
	case STEP_EAST: dx = 1; dy = 0; break;
	case STEP_NORTHEAST: dx = 1; dy = -1; break;
	case STEP_NORTH: dx = 0; dy = -1; break;
	case STEP_NORTHWEST: dx = -1; dy = -1; break;
	case STEP_WEST: dx = -1; dy = 0; break;
	case STEP_SOUTHWEST: dx = -1; dy = 1; break;
	case STEP_SOUTH: dx = 0; dy = 1; break;
	case STEP_SOUTHEAST: dx = 1; dy = 1; break;
	default: break; // no step: dx and dy stay 0, as without it (bugprone-switch-missing-default-case, issue #110)
		// clang-format on
	}
	const TObject Bank = Map.GetFirstObject(dx + 8, dy + 6, Map.GetMapLevel());
	if (Bank.Type == 0 || !ObjectTypes.GetObjectFlag(Bank.Type, BANK))
		return;
	if (!Map.IsFieldPassable(dx + 8, dy + 6, Map.GetMapLevel(), false))
		return;
	const int Height = Map.GetFieldHeight(8, 6, Map.GetMapLevel());
	if (Map.GetFieldHeight(dx + 8, dy + 6, Map.GetMapLevel()) > Height + 1)
		return;
	Player->StartMovement(dx, dy, ObjectTypes.GetObjectWaypoints(Bank.Type));
	PlayerStepDX = dx;
	PlayerStepDY = dy;
	if (dx > 0)
		PlayerStepDirection = DIRECTION_EAST;
	else if (dx < 0)
		PlayerStepDirection = DIRECTION_WEST;
	else if (dy < 0)
		PlayerStepDirection = DIRECTION_NORTH;
	else if (dy > 0)
		PlayerStepDirection = DIRECTION_SOUTH;
}

// Computes the path to the destination and starts its first step; 1 if
// there is a path. A result of 2 from the path finder (FindPath) keeps
// the destination.
int patella::TPlayerState::FindPlayerPath()
{
	NumberOfPlayerSteps = 0;
	NewPlayerDestination = false;
	if (DestinationX == 0)
		return 0;
	if (abs(PlayerStepDY) + abs(PlayerStepDX) > 1) {
		TCreature *Player = Creatures.GetCreature(GetPlayerID());
		if (Player != nullptr)
			Player->Moving = true;
		return 0;
	}
	int x, y, z;
	Map.GetMapPosition(&x, &y, &z);
	x += PlayerStepDX;
	y += PlayerStepDY;
	const int Result = Map.FindPath(x, y, z, DestinationX, DestinationY, DestinationZ, DestinationExact,
		&NumberOfPlayerSteps, PlayerSteps.data(), DestinationDiagonal);
	if (Result != 2)
		DestinationX = 0;
	if (Result > 0) {
		CurrentPlayerStep = 0;
		StartPlayerStep();
		return 1;
	}
	switch (Result) {
	case -4:
		GUI.SetStatusMessage("There is no way.", false);
		return 0;
	case -3:
		GUI.SetStatusMessage("Destination is out of range.", false);
		return 0;
	case -2:
		GUI.SetStatusMessage("First go downstairs.", false);
		return 0;
	case -1:
		GUI.SetStatusMessage("First go upstairs.", false);
		return 0;
	default: // the 0 below, as without it (bugprone-switch-missing-default-case, issue #110)
		break;
	}
	return 0;
}

// 0 if nothing is to be sent, otherwise the result of FindPlayerPath, or 2
// when the new path waits for the current step. The second comparison adds
// the unconfirmed step twice (GetPlayerPosition has added it already);
// kept from the binary.
int patella::TPlayerState::SetPlayerDestination(int x, int y, int z, bool Exact, bool Diagonal)
{
	int PlayerX, PlayerY, PlayerZ;
	GetPlayerPosition(&PlayerX, &PlayerY, &PlayerZ);
	if (x == DestinationX && y == DestinationY && z == DestinationZ)
		return 0;
	if (x == PlayerX + PlayerStepDX && y == PlayerY + PlayerStepDY && z == PlayerZ)
		return 0;
	DestinationX = x;
	DestinationY = y;
	DestinationZ = z;
	DestinationExact = Exact;
	DestinationDiagonal = Diagonal;
	Map.WorldToMap(&x, &y, &z);
	if (In(x, 1, 15) && In(y, 1, 11) && z == Map.GetMapLevel() && (x != 8 || y != 6)) {
		if (!Map.IsFieldPassable(x, y, z, true)) {
			GUI.SetStatusMessage("Sorry, not possible.", false);
			DestinationX = 0;
			return 0;
		}
	}
	if (!NewPlayerDestination) {
		if (NumberOfPlayerSteps == 0)
			return FindPlayerPath();
		if (CurrentPlayerStep < NumberOfPlayerSteps) {
			NewPlayerDestination = true;
			return 2;
		}
	}
	return 0;
}

void patella::TPlayerState::StopPlayer()
{
	DestinationX = 0;
	NumberOfPlayerSteps = 0;
	CurrentPlayerStep = 0;
	NewPlayerDestination = false;
	PlayerStepDX = 0;
	PlayerStepDY = 0;
	TCreature *Player = Creatures.GetCreature(GetPlayerID());
	if (Player != nullptr) {
		Player->MoveDirection = Player->Direction;
		Player->Moving = false;
		Player->DeltaX = 0;
		Player->DeltaY = 0;
	}
}

// The server refused a step: turn to NewDirection and plan the path
// again; true if a new walk is to be sent.
bool patella::TPlayerState::CancelPlayerWalk(int NewDirection)
{
	NumberOfPlayerSteps = 0;
	TCreature *Player = Creatures.GetCreature(GetPlayerID());
	if (Player == nullptr) {
		PlayerStepDX = 0;
		PlayerStepDY = 0;
	} else {
		Player->Direction = NewDirection;
		if (PlayerStepDX != 0 || PlayerStepDY != 0 || !Player->Moving) {
			Player->StartMovement(0, 0, -1);
			return FindPlayerPath() == 1;
		}
	}
	return false;
}

// --- The creature table ----------------------------------------------------

// Takes the slot of RemoveID (a free slot when RemoveID is 0); without one
// the last slot is overwritten. A reused slot keeps its name, position and
// battle list links.
TCreature *patella::TCreatures::AddCreature(uint32 RemoveID, uint32 NewCreatureID)
{
	PATELLA_ASSERT(NewCreatureID!=0);
	int Index = -1;
	for (int i = 0; i < CREATURES; i++) {
		if (Creatures[i].CreatureID == RemoveID) {
			Index = i;
			break;
		}
	}
	if (Index == -1)
		Index = CREATURES - 1;
	TCreature *Creature = &Creatures[Index];
	if (Creature->CreatureID == 0)
		Creature->NextOpponent = NOT_AN_OPPONENT;
	Creature->PKFlag = PK_NONE;
	Creature->PARTYFlag = PARTY_NONE;
	Creature->SkullTime = 0;
	Creature->CreatureID = NewCreatureID;
	return Creature;
}

TCreature *patella::TCreatures::GetCreature(uint32 CreatureID)
{
	PATELLA_ASSERT(CreatureID!=0);
	for (int i = 0; i < CREATURES; i++) {
		if (Creatures[i].CreatureID == CreatureID)
			return &Creatures[i];
	}
	return nullptr;
}

// Removes the skulls that SetSkull kept.
void patella::TCreatures::UpdateSkulls()
{
	const std::int64_t Now = patella::platform::Platform().Clock.WallSeconds();
	for (int i = 0; i < CREATURES; i++) {
		TCreature *Creature = &Creatures[i];
		if (Creature->SkullTime != 0 && !Creature->SkullKept(Now)) {
			Creature->SkullTime = 0;
			Creature->PKFlag = PK_NONE;
		}
	}
}

void patella::TCreatures::HideCreature(int CreatureID)
{
	TCreature *Creature = GetCreature(CreatureID);
	if (Creature != nullptr)
		Creature->Visible = false;
}

// Advances the walk animations; when the player's step is over the next
// one starts. True if a new walk is to be sent.
bool patella::TCreatures::MoveCreatures()
{
	bool Result = false;
	const int Now = GetMilliseconds();
	for (int i = 0; i < CREATURES; i++) {
		TCreature *Creature = &Creatures[i];
		if (!Creature->Moving)
			continue;
		if (Creature->DeltaX != 0) {
			while (Creature->DeltaX > 0
				&& Creature->StepDuration * Creature->DeltaX > (Creature->StepEnd - Now) * Creature->StepX)
				Creature->DeltaX--;
			while (Creature->DeltaX < 0
				&& -(Creature->StepDuration * Creature->DeltaX) > (Creature->StepEnd - Now) * Creature->StepX)
				Creature->DeltaX++;
		}
		if (Creature->DeltaY != 0) {
			while (Creature->DeltaY > 0
				&& Creature->StepDuration * Creature->DeltaY > (Creature->StepEnd - Now) * Creature->StepY)
				Creature->DeltaY--;
			while (Creature->DeltaY < 0
				&& -(Creature->StepDuration * Creature->DeltaY) > (Creature->StepEnd - Now) * Creature->StepY)
				Creature->DeltaY++;
		}
		if (Creature->DeltaX == 0 && Creature->DeltaY == 0 && Now > Creature->NextStep) {
			Creature->Moving = false;
			if (Creature->CreatureID == PlayerState.GetPlayerID() && PlayerState.ContinuePlayerWalk())
				Result = true;
		}
	}
	return Result;
}

// Help hints (CheckEventHelpName) for the monsters next to the player. Without a
// player creature a monster in the battle list fails IsOpponent's assertion.
void patella::TCreatures::CheckMonsterHint(TCreature *Creature)
{
	PATELLA_ASSERT(Creature!=nullptr);
	if (!Creature->Visible)
		return;
	const TCreature *Player = GetCreature(PlayerState.GetPlayerID());
	if (Player != nullptr && Creature->CreatureID == Player->CreatureID) {
		const int Number = GetNumberOfOpponents();
		for (int i = 0; i < Number; i++) {
			TCreature *Opponent = GetOpponent(i);
			PATELLA_ASSERT(Opponent!=nullptr);
			if (!IsPlayer(Opponent->CreatureID) && abs(Opponent->RealX - Player->RealX) <= 2
				&& abs(Opponent->RealY - Player->RealY) <= 2)
				GUI.CheckEventHelpName(Opponent->Name);
		}
	} else if (!IsPlayer(Creature->CreatureID) && IsOpponent(Creature)
		// IsOpponent asserts the player creature (a null Player throws there; MSVC /analyze's C6011, issue #110).
		PATELLA_MSVC_WARNING(suppress : 6011)
		&& abs(Creature->RealX - Player->RealX) <= 2 && abs(Creature->RealY - Player->RealY) <= 2) {
		GUI.CheckEventHelpName(Creature->Name);
	}
}

// Communication.cpp passes the skull of every creature it reads.
void patella::TCreatures::NotifySkull(int Flag)
{
	if (Flag != PK_NONE)
		GUI.AddEventHelp(HINT_PLAYER_KILLING);
}

// --- Experience and skills -------------------------------------------------

// The help hints for the first experience and the levels 2, 8 and 20; none
// while the level is unknown (0).
void patella::TPlayerState::SetPlayerExperience(int Experience, int Level, int Progress)
{
	PATELLA_ASSERT_VALUES(In(Progress,-1,100), Progress);
	if (PlayerLevel != 0 && PlayerExperience == 0 && Experience > 0)
		GUI.AddEventHelp(HINT_FIRST_EXPERIENCE_POINTS);
	if (PlayerLevel != 0 && PlayerLevel < 2 && Level >= 2)
		GUI.AddEventHelp(HINT_LEVEL_2);
	if (PlayerLevel != 0 && PlayerLevel < 8 && Level >= 8)
		GUI.AddEventHelp(HINT_LEVEL_8);
	if (PlayerLevel != 0 && PlayerLevel < 20 && Level >= 20)
		GUI.AddEventHelp(HINT_LEVEL_20);
	PlayerExperience = Experience;
	PlayerLevel = Level;
	PlayerLevelProgress = Progress;
}

// SkillNumber is not checked.
void patella::TPlayerState::SetPlayerSkill(int SkillNumber, int Value, int Progress)
{
	PATELLA_ASSERT_VALUES(In(Progress,0,100), Progress);
	if (PlayerSkillValue[SkillNumber] != 0 && PlayerSkillValue[SkillNumber] < Value)
		GUI.AddEventHelp(HINT_FIRST_SKILL_ADVANCE);
	PlayerSkillProgress[SkillNumber] = Progress;
	PlayerSkillValue[SkillNumber] = Value;
}

int patella::TPlayerState::GetPlayerSkillValue(int SkillNumber) const
{
	PATELLA_ASSERT_VALUES(In(SkillNumber,0,SKILLS-1), SkillNumber);
	return PlayerSkillValue[SkillNumber];
}

int patella::TPlayerState::GetPlayerSkillProgress(int SkillNumber) const
{
	PATELLA_ASSERT_VALUES(In(SkillNumber,0,SKILLS-1), SkillNumber);
	return PlayerSkillProgress[SkillNumber];
}

const char *patella::TPlayerState::GetSkillName(int SkillNumber)
{
	PATELLA_ASSERT_VALUES(In(SkillNumber,0,SKILLS-1), SkillNumber);
	switch (SkillNumber) {
	case 0: return "Fist Fighting";
	case 1: return "Club Fighting";
	case 2: return "Sword Fighting";
	case 3: return "Axe Fighting";
	case 4: return "Distance Fighting";
	case 5: return "Shielding";
	case 6: return "Fishing";
	default: return "";
	}
}

// No check for a missing player creature.
bool patella::TPlayerState::HasPlayerSkull() const
{
	return Creatures.GetCreature(GetPlayerID())->PKFlag != PK_NONE;
}

// Party flag 1, 3 or 4; ContextMenus.cpp offers "Leave Party" then.
bool patella::TPlayerState::IsPlayerInParty() const
{
	const int Shield = Creatures.GetCreature(GetPlayerID())->PARTYFlag;
	return Shield == PARTY_LEADER || Shield == PARTY_MEMBER || Shield == PARTY_INVITER;
}

int patella::TPlayerState::GetPlayerPartyFlag() const
{
	return Creatures.GetCreature(GetPlayerID())->PARTYFlag;
}

// --- The battle list -------------------------------------------------------

// A visible, living creature on the player's level within the screen; not
// monsters that are invisible (outfit 0 showing object 0).
bool patella::TCreatures::IsOpponent(TCreature *Creature)
{
	PATELLA_ASSERT(Creature!=nullptr);
	if (Creature->CreatureID == 0 || Creature->CreatureID == PlayerState.GetPlayerID() || !Creature->Visible)
		return false;
	if (!IsPlayer(Creature->CreatureID) && Creature->OutfitType == 0 && Creature->Colors[0] == 0)
		return false;
	if (Creature->Health < 1)
		return false;
	const TCreature *Player = GetCreature(PlayerState.GetPlayerID());
	PATELLA_ASSERT(Player!=nullptr);
	if (Creature->RealZ != Player->RealZ || abs(Creature->RealX - Player->RealX) > 7)
		return false;
	return abs(Creature->RealY - Player->RealY) <= 5;
}

// Brings the battle list up to date: new opponents are appended, the others
// taken out. Returns the number of opponents.
int patella::TCreatures::GetNumberOfOpponents()
{
	int Number = 0;
	for (int i = 0; i < CREATURES; i++) {
		TCreature *Creature = &Creatures[i];
		if (Creature->CreatureID == 0)
			continue;
		if (IsOpponent(Creature)) {
			if (Creature->NextOpponent == NOT_AN_OPPONENT) {
				Creature->NextOpponent = -1;
				Creature->PrevOpponent = LastOpponent;
				if (LastOpponent != -1)
					Creatures[LastOpponent].NextOpponent = i;
				LastOpponent = i;
				if (FirstOpponent == -1)
					FirstOpponent = i;
			}
			Number++;
		} else if (Creature->NextOpponent != NOT_AN_OPPONENT) {
			if (Creature->NextOpponent != -1)
				Creatures[Creature->NextOpponent].PrevOpponent = Creature->PrevOpponent;
			if (Creature->PrevOpponent != -1)
				Creatures[Creature->PrevOpponent].NextOpponent = Creature->NextOpponent;
			if (FirstOpponent == i)
				FirstOpponent = Creature->NextOpponent;
			if (LastOpponent == i)
				LastOpponent = Creature->PrevOpponent;
			Creature->NextOpponent = NOT_AN_OPPONENT;
		}
	}
	return Number;
}

// Number counts from 0 in list order.
TCreature *patella::TCreatures::GetOpponent(int Number)
{
	PATELLA_ASSERT_VALUES(Number>=0, Number);
	int i = FirstOpponent;
	while (i != -1 && Number > 0) {
		i = Creatures[i].NextOpponent;
		Number--;
	}
	if (i == -1)
		return nullptr;
	return &Creatures[i];
}

// --- The VIP list ----------------------------------------------------------

// True if buddy i may stand before buddy j (also when they are
// equal).
bool patella::TCreatures::CompareBuddies(int i, int j) const
{
	PATELLA_ASSERT_VALUES(i!=j, i);
	PATELLA_ASSERT_VALUES(In(i,0,NumberOfBuddies-1), i, NumberOfBuddies-1);
	PATELLA_ASSERT_VALUES(In(j,0,NumberOfBuddies-1), j, NumberOfBuddies-1);
	for (int k = 0; k < 3; k++) {
		switch (BuddySortMethods[k]) {
		case 0: {
			const int Result = strcmp(Buddies[i].Name, Buddies[j].Name);
			if (Result < 0)
				return true;
			if (Result > 0)
				return false;
			break;
		}
		case 1:
			if (Buddies[i].Icon < Buddies[j].Icon)
				return true;
			if (Buddies[i].Icon > Buddies[j].Icon)
				return false;
			break;
		case 2:
			if (Buddies[i].Online && !Buddies[j].Online)
				return true;
			if (!Buddies[i].Online && Buddies[j].Online)
				return false;
			break;
		default: // another method compares nothing, as without it (bugprone-switch-missing-default-case, issue #110)
			break;
		}
	}
	return true;
}

void patella::TCreatures::SwapBuddies(int i, int j)
{
	PATELLA_ASSERT_VALUES(i!=j, i);
	PATELLA_ASSERT_VALUES(In(i,0,NumberOfBuddies-1), i, NumberOfBuddies-1);
	PATELLA_ASSERT_VALUES(In(j,0,NumberOfBuddies-1), j, NumberOfBuddies-1);
	const TBuddyEntry Help = Buddies[i];
	Buddies[i] = Buddies[j];
	Buddies[j] = Help;
}

void patella::TCreatures::SortBuddies()
{
	for (int i = 0; i < NumberOfBuddies - 1; i++) {
		for (int j = i + 1; j < NumberOfBuddies; j++) {
			if (!CompareBuddies(i, j))
				SwapBuddies(i, j);
		}
	}
}

// Moves buddy i to its place. CompareBuddies is also true for equal
// buddies, so i first moves up past every equal one.
void patella::TCreatures::MoveBuddyToSortedPlace(int i)
{
	PATELLA_ASSERT_VALUES(In(i,0,NumberOfBuddies-1), i, NumberOfBuddies-1);
	while (i > 0 && CompareBuddies(i, i - 1)) {
		SwapBuddies(i, i - 1);
		i--;
	}
	while (i < NumberOfBuddies - 1 && !CompareBuddies(i, i + 1)) {
		SwapBuddies(i, i + 1);
		i++;
	}
}

// Adds the buddy or updates the entry. An entry that was online and is
// updated as offline stays counted as online.
void patella::TCreatures::AddBuddy(uint32 CharacterID, const char *Name, bool Online)
{
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Name)<30, strlen(Name));
	int Position = GetBuddyPosition(CharacterID);
	if (Position == -1) {
		PATELLA_ASSERT_VALUES(NumberOfBuddies<BUDDIES, NumberOfBuddies);
		Position = NumberOfBuddies++;
		if (Online)
			NumberOfOnlineBuddies++;
	} else {
		PATELLA_ASSERT_VALUES(NumberOfBuddies<=BUDDIES, NumberOfBuddies);
		if (Online && !Buddies[Position].Online)
			NumberOfOnlineBuddies++;
	}
	TBuddyEntry *Buddy = &Buddies[Position];
	Buddy->CharacterID = CharacterID;
	Buddy->Name.Assign(Name);
	Buddy->Online = Online;
	Buddy->LoginAction = 0;
	Buddy->Icon = GetBuddyIcon(CharacterID);
	MoveBuddyToSortedPlace(Position);
}

void patella::TCreatures::SetBuddyState(uint32 CharacterID, bool Online)
{
	const int Position = GetBuddyPosition(CharacterID);
	if (Position == -1)
		return;
	TBuddyEntry *Buddy = &Buddies[Position];
	if (Buddy->Online)
		NumberOfOnlineBuddies--;
	Buddy->Online = Online;
	if (Online)
		NumberOfOnlineBuddies++;
	Buddy->LoginAction = GUI.GetActionNumber();
	if (Online && GetBuddyNotify(CharacterID)) {
		TFixedString<256> Text;
		Text.Format("{} has logged in.", Buddy->Name);
		GUI.SetStatusMessage(Text, false);
	}
	MoveBuddyToSortedPlace(Position);
}

// Stores the buddy's data in the preferences (an empty entry deletes it
// there) and resorts the list.
void patella::TCreatures::EditBuddy(int CharacterID, int Icon, const char *Description, bool Notify)
{
	PATELLA_ASSERT(CharacterID!=0);
	PATELLA_ASSERT(Description!=nullptr);
	if (Icon == 0 && Description[0] == 0 && !Notify)
		DeleteBuddy(CharacterID);
	else
		SetBuddy(CharacterID, Icon, Description, Notify);
	const int Position = GetBuddyPosition(CharacterID);
	if (Position != -1) {
		Buddies[Position].Icon = Icon;
		MoveBuddyToSortedPlace(Position);
	}
}

// Message 0x1a (Status.cpp) gets a copy of the name.
void patella::TCreatures::RemoveBuddy(uint32 CharacterID)
{
	const int Position = GetBuddyPosition(CharacterID);
	if (Position == -1)
		return;
	GUI.AddMessage(MESSAGE_TEMPORARY_UNIGNORE, reinterpret_cast<std::intptr_t>(MsvcStrdup(Buddies[Position].Name)), 0);
	if (Buddies[Position].Online)
		NumberOfOnlineBuddies--;
	for (int i = Position; i < NumberOfBuddies - 1; i++)
		Buddies[i] = Buddies[i + 1];
	NumberOfBuddies--;
	DeleteBuddy(CharacterID);
}

// Method becomes the most significant sort key. With offline buddies
// hidden the online state is made the first key at once.
void patella::TCreatures::SetBuddySortMethod(int Method)
{
	if (Method == BuddySortMethods[0])
		return;
	if (Method == BuddySortMethods[2])
		BuddySortMethods[2] = BuddySortMethods[1];
	BuddySortMethods[1] = BuddySortMethods[0];
	BuddySortMethods[0] = Method;
	if (!GetBuddyHideOffline() || Method == 2)
		SortBuddies();
	else
		SetBuddySortMethod(2);
	std::array<int, 3> Methods;
	for (int i = 0; i < 3; i++)
		Methods[i] = BuddySortMethods[i];
	SetBuddySortmethods(Methods.data());
}

void patella::TCreatures::HideOfflineBuddies(bool Hide)
{
	SetBuddyHideOffline(Hide);
	if (Hide)
		SetBuddySortMethod(2);
}

int patella::TCreatures::GetNumberOfBuddies() const
{
	if (GetBuddyHideOffline())
		return NumberOfOnlineBuddies;
	return NumberOfBuddies;
}

uint32 patella::TCreatures::GetBuddyListID(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return Buddies[Position].CharacterID;
}

const char *patella::TCreatures::GetBuddyListName(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return Buddies[Position].Name;
}

bool patella::TCreatures::IsBuddyOnline(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return Buddies[Position].Online;
}

// Online and logged in less than ten actions ago.
bool patella::TCreatures::IsBuddyNewlyOnline(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	if (Buddies[Position].Online && GUI.GetActionNumber() - 10 < Buddies[Position].LoginAction)
		return true;
	return false;
}

int patella::TCreatures::GetBuddyListIcon(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return Buddies[Position].Icon;
}

const char *patella::TCreatures::GetBuddyListDescription(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return GetBuddyDescription(Buddies[Position].CharacterID);
}

bool patella::TCreatures::GetBuddyListNotify(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,NumberOfBuddies-1), Position, NumberOfBuddies-1);
	return GetBuddyNotify(Buddies[Position].CharacterID);
}

// --- Initialisation --------------------------------------------------------

// Called by InitControl.
void patella::TCreatures::InitCreatures()
{
	ResetCreatures();
}

// Called by InitCreatures and EnterGame. The beat, soul, highlighted
// creature, step direction and the destination's y and z stay; the sort
// methods come from the preferences when they are valid.
void patella::TCreatures::ResetCreatures()
{
	memset(Creatures.data(), 0, sizeof(Creatures));
	for (int i = 0; i < CREATURES; i++)
		Creatures[i].CreatureID = 0;
	FirstOpponent = -1;
	LastOpponent = -1;
	PlayerState.ResetPlayer();
	AttackID = 0;
	FollowID = 0;
	NumberOfBuddies = 0;
	NumberOfOnlineBuddies = 0;
	BuddySortMethods[0] = 2;
	BuddySortMethods[1] = 1;
	BuddySortMethods[2] = 0;
	std::array<int, 3> Methods;
	GetBuddySortmethods(Methods.data());
	for (int i = 0; i < 3; i++) {
		if (In(Methods[i], 0, 2))
			BuddySortMethods[i] = Methods[i];
	}
}

// Ours (issue #106; TCreaturesPlayer): ResetCreatures' writes of the player's state, in its order (it wrote
// AttackID and FollowID between PlayerCapacity and PlayerState; nothing reads between the writes).
void patella::TPlayerState::ResetPlayer()
{
	PlayerStepDX = 0;
	PlayerStepDY = 0;
	DestinationX = 0;
	NumberOfPlayerSteps = 0;
	CurrentPlayerStep = 0;
	NewPlayerDestination = false;
	PlayerID = 0;
	PlayerHealth = 0;
	PlayerMaxHealth = 0;
	PlayerExperience = 0;
	PlayerLevel = 0;
	PlayerLevelProgress = 0;
	PlayerMana = 0;
	PlayerMaxMana = 0;
	PlayerMagicLevel = 0;
	PlayerMagicLevelProgress = 0;
	PlayerCapacity = 0;
	PlayerState = 0;
	for (int i = 0; i < SKILLS; i++) {
		PlayerSkillValue[i] = 0;
		PlayerSkillProgress[i] = 0;
	}
}

// Ours (issue #106; TCreaturesPlayer): MoveCreatures' code for the player's creature once its step is over: the
// next step of the walk, or the path again; true if a new walk is to be sent.
bool patella::TPlayerState::ContinuePlayerWalk()
{
	if (CurrentPlayerStep < NumberOfPlayerSteps) {
		StartPlayerStep();
		return false;
	}
	return FindPlayerPath() == 1;
}

// Empty. ExitControl calls it where Creatures.cpp's exit belongs in the reverse
// order of InitControl (name ours, low confidence).
void patella::TCreatures::ExitCreatures()
{
}

// The original's functions (issue #106): each calls its member on the one TClient's TPlayerState or TCreatures.

void GetPlayerPosition(int *x, int *y, int *z)
{
	patella::bridge::PlayerState().GetPlayerPosition(x, y, z);
}

void ClearPlayerSteps()
{
	patella::bridge::PlayerState().ClearPlayerSteps();
}

uint32 GetPlayerID()
{
	return patella::bridge::PlayerState().GetPlayerID();
}

int GetPlayerHealth()
{
	return patella::bridge::PlayerState().GetPlayerHealth();
}

int GetPlayerMaxHealth()
{
	return patella::bridge::PlayerState().GetPlayerMaxHealth();
}

int GetPlayerExperience()
{
	return patella::bridge::PlayerState().GetPlayerExperience();
}

int GetPlayerLevel()
{
	return patella::bridge::PlayerState().GetPlayerLevel();
}

int GetPlayerLevelProgress()
{
	return patella::bridge::PlayerState().GetPlayerLevelProgress();
}

int GetPlayerMana()
{
	return patella::bridge::PlayerState().GetPlayerMana();
}

int GetPlayerMaxMana()
{
	return patella::bridge::PlayerState().GetPlayerMaxMana();
}

int GetPlayerSoul()
{
	return patella::bridge::PlayerState().GetPlayerSoul();
}

int GetPlayerMagicLevel()
{
	return patella::bridge::PlayerState().GetPlayerMagicLevel();
}

int GetPlayerMagicLevelProgress()
{
	return patella::bridge::PlayerState().GetPlayerMagicLevelProgress();
}

int GetPlayerCapacity()
{
	return patella::bridge::PlayerState().GetPlayerCapacity();
}

bool IsPlayerPoisoned()
{
	return patella::bridge::PlayerState().IsPlayerPoisoned();
}

bool IsPlayerBurning()
{
	return patella::bridge::PlayerState().IsPlayerBurning();
}

bool IsPlayerElectrified()
{
	return patella::bridge::PlayerState().IsPlayerElectrified();
}

bool IsPlayerDrunk()
{
	return patella::bridge::PlayerState().IsPlayerDrunk();
}

bool IsPlayerManaShielded()
{
	return patella::bridge::PlayerState().IsPlayerManaShielded();
}

bool IsPlayerParalyzed()
{
	return patella::bridge::PlayerState().IsPlayerParalyzed();
}

bool IsPlayerHasted()
{
	return patella::bridge::PlayerState().IsPlayerHasted();
}

bool IsPlayerInFight()
{
	return patella::bridge::PlayerState().IsPlayerInFight();
}

int SetPlayerDestination(int x, int y, int z, bool Exact, bool Diagonal)
{
	return patella::bridge::PlayerState().SetPlayerDestination(x, y, z, Exact, Diagonal);
}

int GetPlayerSkillValue(int SkillNumber)
{
	return patella::bridge::PlayerState().GetPlayerSkillValue(SkillNumber);
}

int GetPlayerSkillProgress(int SkillNumber)
{
	return patella::bridge::PlayerState().GetPlayerSkillProgress(SkillNumber);
}

const char *GetSkillName(int SkillNumber)
{
	return patella::bridge::PlayerState().GetSkillName(SkillNumber);
}

bool HasPlayerSkull()
{
	return patella::bridge::PlayerState().HasPlayerSkull();
}

bool IsPlayerInParty()
{
	return patella::bridge::PlayerState().IsPlayerInParty();
}

int GetPlayerPartyFlag()
{
	return patella::bridge::PlayerState().GetPlayerPartyFlag();
}

bool IsPlayer(uint32 CreatureID)
{
	return patella::bridge::Creatures().IsPlayer(CreatureID);
}

void SetAttackID(uint32 CreatureID)
{
	patella::bridge::Creatures().SetAttackID(CreatureID);
}

void SetFollowID(uint32 CreatureID)
{
	patella::bridge::Creatures().SetFollowID(CreatureID);
}

void SetHighlightID(uint32 CreatureID)
{
	patella::bridge::Creatures().SetHighlightID(CreatureID);
}

uint32 GetAttackID()
{
	return patella::bridge::Creatures().GetAttackID();
}

uint32 GetFollowID()
{
	return patella::bridge::Creatures().GetFollowID();
}

uint32 GetHighlightID()
{
	return patella::bridge::Creatures().GetHighlightID();
}

int GetBuddyPosition(uint32 CharacterID)
{
	return patella::bridge::Creatures().GetBuddyPosition(CharacterID);
}

bool IsBuddy(const char *Name)
{
	return patella::bridge::Creatures().IsBuddy(Name);
}

TCreature *GetCreature(uint32 CreatureID)
{
	return patella::bridge::Creatures().GetCreature(CreatureID);
}

void UpdateSkulls()
{
	patella::bridge::Creatures().UpdateSkulls();
}

void HideCreature(int CreatureID)
{
	patella::bridge::Creatures().HideCreature(CreatureID);
}

bool MoveCreatures()
{
	return patella::bridge::Creatures().MoveCreatures();
}

void CheckMonsterHint(TCreature *Creature)
{
	patella::bridge::Creatures().CheckMonsterHint(Creature);
}

bool IsOpponent(TCreature *Creature)
{
	return patella::bridge::Creatures().IsOpponent(Creature);
}

int GetNumberOfOpponents()
{
	return patella::bridge::Creatures().GetNumberOfOpponents();
}

TCreature *GetOpponent(int Number)
{
	return patella::bridge::Creatures().GetOpponent(Number);
}

void EditBuddy(int CharacterID, int Icon, const char *Description, bool Notify)
{
	patella::bridge::Creatures().EditBuddy(CharacterID, Icon, Description, Notify);
}

void RemoveBuddy(uint32 CharacterID)
{
	patella::bridge::Creatures().RemoveBuddy(CharacterID);
}

void SetBuddySortMethod(int Method)
{
	patella::bridge::Creatures().SetBuddySortMethod(Method);
}

void HideOfflineBuddies(bool Hide)
{
	patella::bridge::Creatures().HideOfflineBuddies(Hide);
}

int GetNumberOfBuddies()
{
	return patella::bridge::Creatures().GetNumberOfBuddies();
}

uint32 GetBuddyListID(int Position)
{
	return patella::bridge::Creatures().GetBuddyListID(Position);
}

const char *GetBuddyListName(int Position)
{
	return patella::bridge::Creatures().GetBuddyListName(Position);
}

bool IsBuddyOnline(int Position)
{
	return patella::bridge::Creatures().IsBuddyOnline(Position);
}

bool IsBuddyNewlyOnline(int Position)
{
	return patella::bridge::Creatures().IsBuddyNewlyOnline(Position);
}

int GetBuddyListIcon(int Position)
{
	return patella::bridge::Creatures().GetBuddyListIcon(Position);
}

const char *GetBuddyListDescription(int Position)
{
	return patella::bridge::Creatures().GetBuddyListDescription(Position);
}

bool GetBuddyListNotify(int Position)
{
	return patella::bridge::Creatures().GetBuddyListNotify(Position);
}

void InitCreatures()
{
	patella::bridge::Creatures().InitCreatures();
}

void ExitCreatures()
{
	patella::bridge::Creatures().ExitCreatures();
}

void ResetCreatures()
{
	patella::bridge::Creatures().ResetCreatures();
}

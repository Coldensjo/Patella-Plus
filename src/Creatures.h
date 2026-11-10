// Creatures.h: the creatures known to the client, the player's own data
// (statistics, skills, attack/follow targets, the steps of the current walk)
// and the VIP list ("buddies").
//
// Names from the binary: GetCreature, GetPlayerID and GetPlayerPartyFlag
// (assertion texts in ContextMenus.cpp and ConsoleWindow.cpp), CreatureID
// (catch blocks of the TCreature members), NumberOfPlayerSteps,
// CurrentPlayerStep, NumberOfBuddies, BUDDIES, SKILLS, Opponent, and the
// parameter names in the assertions and catch blocks (Step, NewCreatureID,
// AvailablePhases, Flag, Progress, SkillNumber, Value, Number, Position,
// CharacterID, Name, Description, Method, NewDirection, i, j). The rest is
// ours (notes/decisions.md, "Creatures.cpp").

#pragma once

#include "common.h"
#include "Events.h"
#include "Help.h" // HINT_*
#include "types/FixedString.h"
#include "types/GraphicsTypes.h" // TRGBColor
#include "Preferences.h"         // SKILLS

#include <array>
#include <cstdint>

inline constexpr int CREATURES = 150; // slots in the creature table (loops up to 0x96)
inline constexpr int BUDDIES = 200;   // "NumberOfBuddies<BUDDIES"

// TCreature::NextOpponent of a creature that is not in the battle list.
inline constexpr int NOT_AN_OPPONENT = 0x7fffffff;

// The values of TCreature::PKFlag, the skull beside a creature's name, as the server sends them. The prefix is the
// member's; the names are ours, the colours of the skull pictures (the code has only their skin numbers: 244-247 on
// the map, 240-243 in the battle list). The texts are those of the player's own skull in the status bar
// (ContentWindows.cpp, TPlayerStates).
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, enumerators for the reason in GUI.h
	PK_NONE = 0,
	PK_YELLOW = 1, // "You may be attacked by other players"
	PK_GREEN = 2,  // "You are a member of a party"
	PK_WHITE = 3,  // "You have attacked an unmarked player"
	PK_RED = 4,    // "You have killed too many unmarked players"
};

// The values of TCreature::PARTYFlag, the party shield beside the name: the creature's part in a party as the player
// sees it. The prefix is the member's; the names are ours, from what the context menus offer for each
// (ContextMenus.cpp) and from IsPlayerInParty.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, enumerators for the reason in GUI.h
	PARTY_NONE = 0,
	// Has invited. Another creature: it invited the player ("Join <name>'s Party"), and its shield is drawn only while
	// the player is in no party. The player: in a party (IsPlayerInParty) with invitations to revoke, nothing else.
	PARTY_INVITER = 1,
	// Invited by the player ("Revoke <name>'s Invitation"). The player's own counts as no party.
	PARTY_INVITEE = 2,
	PARTY_MEMBER = 3, // of the player's party ("Pass Leadership to <name>"); the player: the menu offers nothing
	PARTY_LEADER = 4, // the player: may invite, revoke an invitation and pass the leadership
};

// One entry of the creature table.
class TCreature {
public:
	[[nodiscard]] TRGBColor GetHealthColor() const;
	[[nodiscard]] bool IsMarked() const;
	void SetSkull(int Skull, bool Delay);
	void SetPosition(int x, int y, int z);
	[[nodiscard]] int GetDeltaX() const;
	[[nodiscard]] int GetDeltaY() const;
	[[nodiscard]] int GetDrawDirection() const;
	void StartMovement(int dx, int dy, int Waypoints);
	void SetMark(int Value);
	[[nodiscard]] int GetAnimationPhase(int AvailablePhases) const;
	// Ours (issue #62): the skull SetSkull kept (Delay) is still shown at Now, the wall clock's 64-bit time:
	// SkullTime is set, and Now is from it to four seconds after it.
	[[nodiscard]] bool SkullKept(std::int64_t Now) const
	{
		if (SkullTime == 0)
			return false;
		return Now >= SkullTime && Now <= SkullTime + 4;
	}

	uint32 CreatureID; // 0 marks a free slot; players have IDs below 0x40000000
	TFixedString<32> Name;
	int RealX; // map position
	int RealY;
	int RealZ;
	int DeltaX; // pixel offset of the walk animation, counts down to 0
	int DeltaY;
	int StepEnd;      // GetMilliseconds() time the step animation ends
	int NextStep;     // time the next step may start (three steps' time after a diagonal one)
	int StepX;        // abs(dx)*32
	int StepY;        // abs(dy)*32
	int StepDuration; // milliseconds, a multiple of Beat
	bool Moving;
	int Direction;     // 0 north, 1 east, 2 south, 3 west (DIRECTION_*, Objects.h)
	int MoveDirection; // Direction when the current step started
	int PrevOpponent;  // battle list links (slot numbers, -1 at the ends)
	int NextOpponent;  // NOT_AN_OPPONENT when not in the list
	int OutfitType;
	std::array<int, 4> Colors; // head, body, legs, feet; Colors[0] is the object type when OutfitType is 0
	int LightStrength;
	int LightColor;
	int Mark;     // colour of the square around the creature
	int MarkTime; // shown up to this GetMilliseconds() time
	int Health;   // percent
	int Speed;
	bool Visible;
	int PKFlag;             // the skull (PK_*); name from "Creature->PKFlag" (MapWindow.cpp)
	int PARTYFlag;          // the party shield (PARTY_*); "Creature->PARTYFlag" (MapWindow.cpp)
	std::int64_t SkullTime; // the wall clock's time when a skull was removed (0: none); it stays four seconds longer
};

// A VIP list entry. Preferences.h has the stored data
// (icon, description, notification) as TBuddy.
struct TBuddyEntry {
	uint32 CharacterID;
	TFixedString<30> Name; // "strlen(Name)<30"
	bool Online;
	int LoginAction; // GetActionNumber() at the last change of Online
	int Icon;
};

namespace patella {

class TMap;
class TObjectTypes;

// What TCreatures asks of the player's state (Patella-Modern issue #106; rule 5 of notes/decisions.md, "#104"):
// TPlayerState is made after TCreatures (it reads the creature table), so TCreatures reaches it only through this
// interface, which TMapState implements (MapState.h). GetPlayerID is the original's (IsOpponent, CheckMonsterHint and
// MoveCreatures call it); ContinuePlayerWalk is MoveCreatures' step of the walk once the player's step animation is
// over, ResetPlayer ResetCreatures' writes of the player's state (both ours: the original's code of those two
// functions, moved into TPlayerState).
class TCreaturesPlayer {
public:
	TCreaturesPlayer() = default;
	TCreaturesPlayer(const TCreaturesPlayer &) = delete;
	TCreaturesPlayer &operator=(const TCreaturesPlayer &) = delete;
	TCreaturesPlayer(TCreaturesPlayer &&) = delete;
	TCreaturesPlayer &operator=(TCreaturesPlayer &&) = delete;
	virtual uint32 GetPlayerID() = 0;
	virtual bool ContinuePlayerWalk() = 0;
	virtual void ResetPlayer() = 0;

protected:
	~TCreaturesPlayer() = default;
};

// What TCreatures and TPlayerState ask of the GUI (Patella-Modern issue #118; rule 5 of notes/decisions.md, "#104"):
// Talk.cpp's status message, Help.cpp's hints, Status.cpp's message queue and action number, all TGUI's, a later
// subsystem than TMapState, so they reach them only through this interface. Client.cpp implements it with those
// files' free functions (the golden tests fake them by their symbols). Each member is the original's call of the same
// name and parameters.
class TCreaturesGUI {
public:
	TCreaturesGUI() = default;
	TCreaturesGUI(const TCreaturesGUI &) = delete;
	TCreaturesGUI &operator=(const TCreaturesGUI &) = delete;
	TCreaturesGUI(TCreaturesGUI &&) = delete;
	TCreaturesGUI &operator=(TCreaturesGUI &&) = delete;
	virtual void SetStatusMessage(const char *Message, bool Permanent) = 0;            // Talk.cpp
	virtual void AddEventHelp(int Number) = 0;                                         // Help.cpp
	virtual void CheckEventHelpName(const char *Name) = 0;                             // Help.cpp
	virtual void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2) = 0; // Status.cpp
	virtual int GetActionNumber() = 0;                                                 // Status.cpp

protected:
	~TCreaturesGUI() = default;
};

// Creatures.cpp's creature table, battle list and VIP list (Patella-Modern issue #106, M7; notes/decisions.md, "#104:
// TClient and the subsystem objects" and "#106"): Creatures, FirstOpponent, LastOpponent, Beat, the attack, follow and
// highlight targets and the buddy window's list, in the original's .bss order, every member 0 at the start; the
// functions that use them are the members of the same names. The data is private, as the file-local statics were;
// TCreature's StartMovement reads Beat (a friend). TMapState owns it (MapState.h), after TMap.
class TCreatures {
public:
	TCreatures(TCreaturesPlayer &PlayerState, TCreaturesGUI &GUI);
	TCreatures(const TCreatures &) = delete;
	TCreatures &operator=(const TCreatures &) = delete;
	TCreatures(TCreatures &&) = delete;
	TCreatures &operator=(TCreatures &&) = delete;
	~TCreatures() = default;

	// The beat, the battle and follow targets and the buddy list's lookups.
	void HideAllCreatures();
	void SetBeat(int Value);
	static bool IsPlayer(uint32 CreatureID);
	void SetAttackID(uint32 CreatureID);    // clears the follow target
	void SetFollowID(uint32 CreatureID);    // clears the attack target
	void SetHighlightID(uint32 CreatureID); // battle list entry under the mouse
	[[nodiscard]] uint32 GetAttackID() const;
	[[nodiscard]] uint32 GetFollowID() const;
	[[nodiscard]] uint32 GetHighlightID() const;
	[[nodiscard]] int GetBuddyPosition(uint32 CharacterID) const; // position or -1
	bool IsBuddy(const char *Name) const;

	// The creature table.
	TCreature *AddCreature(uint32 RemoveID, uint32 NewCreatureID);
	TCreature *GetCreature(uint32 CreatureID);
	void UpdateSkulls();
	void HideCreature(int CreatureID); // a callback for Map.cpp
	bool MoveCreatures();
	void CheckMonsterHint(TCreature *Creature);
	void NotifySkull(int Flag);

	// The battle list.
	bool IsOpponent(TCreature *Creature);
	int GetNumberOfOpponents(); // also updates the list
	TCreature *GetOpponent(int Number);

	// The VIP list.
	[[nodiscard]] bool CompareBuddies(int i, int j) const;
	void SwapBuddies(int i, int j);
	void SortBuddies();
	void MoveBuddyToSortedPlace(int i);
	void AddBuddy(uint32 CharacterID, const char *Name, bool Online);
	void SetBuddyState(uint32 CharacterID, bool Online);
	void EditBuddy(int CharacterID, int Icon, const char *Description, bool Notify);
	void RemoveBuddy(uint32 CharacterID);
	void SetBuddySortMethod(int Method);
	void HideOfflineBuddies(bool Hide);
	[[nodiscard]] int GetNumberOfBuddies() const; // only the online ones when they are hidden
	[[nodiscard]] uint32 GetBuddyListID(int Position) const;
	[[nodiscard]] const char *GetBuddyListName(int Position) const;
	[[nodiscard]] bool IsBuddyOnline(int Position) const;
	[[nodiscard]] bool IsBuddyNewlyOnline(int Position) const;
	[[nodiscard]] int GetBuddyListIcon(int Position) const;
	[[nodiscard]] const char *GetBuddyListDescription(int Position) const;
	[[nodiscard]] bool GetBuddyListNotify(int Position) const;

	void InitCreatures();
	static void ExitCreatures();
	void ResetCreatures();

private:
	friend class ::TCreature;

	TCreaturesPlayer &PlayerState;
	TCreaturesGUI &GUI; // issue #118

	// Data, in .bss order.
	std::array<int, 3> BuddySortMethods{}; // 0 name, 1 icon, 2 online; most significant first
	std::array<TBuddyEntry, BUDDIES> Buddies{};
	int NumberOfOnlineBuddies{};
	int NumberOfBuddies{};
	uint32 HighlightID{};
	uint32 FollowID{};
	uint32 AttackID{};
	int Beat{};         // step durations are rounded up to a multiple
	int LastOpponent{}; // battle list ends (slot numbers, -1: empty)
	int FirstOpponent{};
	std::array<TCreature, CREATURES> Creatures{};

	// ResetCreatures clears the table with a memset over its size.
	static_assert(sizeof(Creatures) == CREATURES * sizeof(TCreature), "Creatures");
};

// Creatures.cpp's player state (Patella-Modern issue #106; notes/decisions.md, "#104" and "#106"): PlayerID, the
// statistics and skills, the state icons and the walk (the steps, the destination, the step not confirmed yet), in the
// original's .bss order, every member 0 at the start; the functions that use them are the members of the same names.
// The data is private; TCreature's members read and
// write the step not confirmed yet (a friend). TMapState owns it, after TCreatures, whose creature table it reads; it
// reads the map and the object types too.
class TPlayerState {
public:
	TPlayerState(TObjectTypes &ObjectTypes, TMap &Map, TCreatures &Creatures, TCreaturesGUI &GUI);
	TPlayerState(const TPlayerState &) = delete;
	TPlayerState &operator=(const TPlayerState &) = delete;
	TPlayerState(TPlayerState &&) = delete;
	TPlayerState &operator=(TPlayerState &&) = delete;
	~TPlayerState() = default;

	// The player's creature, steps and statistics.
	[[nodiscard]] int GetNumberOfPlayerSteps() const;
	void GetPlayerPosition(int *x, int *y, int *z) const; // including the step not confirmed yet
	void ClearPlayerSteps();
	void SetPlayerID(uint32 CreatureID);
	void SetPlayerHealth(int Health, int MaxHealth);
	void SetPlayerMagicLevel(int Level, int Progress);
	void SetPlayerMana(int Mana, int MaxMana);
	void SetPlayerSoul(int Soul);
	void SetPlayerCapacity(int Capacity);
	void SetPlayerState(int State);
	[[nodiscard]] uint32 GetPlayerID() const;
	[[nodiscard]] int GetPlayerHealth() const;
	[[nodiscard]] int GetPlayerMaxHealth() const;
	[[nodiscard]] int GetPlayerExperience() const;
	[[nodiscard]] int GetPlayerLevel() const;
	[[nodiscard]] int GetPlayerLevelProgress() const;
	[[nodiscard]] int GetPlayerMana() const;
	[[nodiscard]] int GetPlayerMaxMana() const;
	[[nodiscard]] int GetPlayerSoul() const;
	[[nodiscard]] int GetPlayerMagicLevel() const;
	[[nodiscard]] int GetPlayerMagicLevelProgress() const;
	[[nodiscard]] int GetPlayerCapacity() const;
	[[nodiscard]] bool IsPlayerPoisoned() const;     // state bit 0
	[[nodiscard]] bool IsPlayerBurning() const;      // bit 1
	[[nodiscard]] bool IsPlayerElectrified() const;  // bit 2
	[[nodiscard]] bool IsPlayerDrunk() const;        // bit 3
	[[nodiscard]] bool IsPlayerManaShielded() const; // bit 4
	[[nodiscard]] bool IsPlayerParalyzed() const;    // bit 5
	[[nodiscard]] bool IsPlayerHasted() const;       // bit 6
	[[nodiscard]] bool IsPlayerInFight() const;      // bit 7
	[[nodiscard]] int GetPlayerStep(int Step) const;
	void StartPlayerStep();
	int FindPlayerPath();
	int SetPlayerDestination(int x, int y, int z, bool Exact, bool Diagonal);
	void StopPlayer();
	bool CancelPlayerWalk(int NewDirection);
	void SetPlayerExperience(int Experience, int Level, int Progress);
	void SetPlayerSkill(int SkillNumber, int Value, int Progress);
	[[nodiscard]] int GetPlayerSkillValue(int SkillNumber) const;
	[[nodiscard]] int GetPlayerSkillProgress(int SkillNumber) const;
	static const char *GetSkillName(int SkillNumber);
	[[nodiscard]] bool HasPlayerSkull() const;
	[[nodiscard]] bool IsPlayerInParty() const;
	[[nodiscard]] int GetPlayerPartyFlag() const;

	// Ours (TCreaturesPlayer): MoveCreatures' continuation of the walk (true if a new walk is to be sent) and
	// ResetCreatures' writes of the player's state, the original's code of those two functions.
	bool ContinuePlayerWalk();
	void ResetPlayer();

private:
	friend class ::TCreature;

	TObjectTypes &ObjectTypes;
	TMap &Map;
	TCreatures &Creatures;
	TCreaturesGUI &GUI; // issue #118

	// Data, in .bss order.
	int PlayerState{}; // the state icons, one bit each
	std::array<int, SKILLS> PlayerSkillProgress{};
	std::array<int, SKILLS> PlayerSkillValue{};
	int PlayerCapacity{};
	int PlayerSoul{};
	int PlayerMaxMana{};
	int PlayerMana{};
	int PlayerMagicLevelProgress{};
	int PlayerLevelProgress{};
	int PlayerMagicLevel{};
	int PlayerLevel{};
	int PlayerExperience{};
	int PlayerMaxHealth{};
	int PlayerHealth{};
	uint32 PlayerID{};
	bool NewPlayerDestination{}; // a destination came while the old path was walked
	int CurrentPlayerStep{};
	int NumberOfPlayerSteps{};
	std::array<int, 10> PlayerSteps{}; // 1 east, 2 north-east, ..., 8 south-east (STEP_*, Map.h); the path finder stops at 10
	bool DestinationDiagonal{};        // passed on to the path finder
	bool DestinationExact{};           // passed on to the path finder
	int DestinationZ{};
	int DestinationY{};
	int DestinationX{};        // 0: no destination
	int PlayerStepDirection{}; // Direction of the step not confirmed yet
	int PlayerStepDY{};        // the step sent but not confirmed by the server
	int PlayerStepDX{};

	// TMap::FindPath writes up to ten steps into PlayerSteps through a pointer.
	static_assert(sizeof(PlayerSteps) == 10 * sizeof(int), "PlayerSteps");
};

// The transition's access path (issue #106; notes/decisions.md, "#104: TClient and the subsystem objects"): the
// creatures and the player's state of the one TClient (Client.cpp). They go when no free function needs them.
namespace bridge {
TCreatures &Creatures();
TPlayerState &PlayerState();
} // namespace bridge

} // namespace patella

// The original's functions (the callers' names, and the golden tests'): each calls the member of the same name of
// TCreatures on bridge::Creatures() or of TPlayerState on bridge::PlayerState().
// The player's creature, steps and statistics.
void GetPlayerPosition(int *x, int *y, int *z); // including the step not confirmed yet
void ClearPlayerSteps();
bool IsPlayer(uint32 CreatureID);
uint32 GetPlayerID();
int GetPlayerHealth();
int GetPlayerMaxHealth();
int GetPlayerExperience();
int GetPlayerLevel();
int GetPlayerLevelProgress();
int GetPlayerMana();
int GetPlayerMaxMana();
int GetPlayerSoul();
int GetPlayerMagicLevel();
int GetPlayerMagicLevelProgress();
int GetPlayerCapacity();
bool IsPlayerPoisoned();                // state bit 0
bool IsPlayerBurning();                 // bit 1
bool IsPlayerElectrified();             // bit 2
bool IsPlayerDrunk();                   // bit 3
bool IsPlayerManaShielded();            // bit 4
bool IsPlayerParalyzed();               // bit 5
bool IsPlayerHasted();                  // bit 6
bool IsPlayerInFight();                 // bit 7
void SetAttackID(uint32 CreatureID);    // clears the follow target
void SetFollowID(uint32 CreatureID);    // clears the attack target
void SetHighlightID(uint32 CreatureID); // battle list entry under the mouse
uint32 GetAttackID();
uint32 GetFollowID();
uint32 GetHighlightID();
int GetBuddyPosition(uint32 CharacterID); // position or -1
bool IsBuddy(const char *Name);
int SetPlayerDestination(int x, int y, int z, bool Exact, bool Diagonal);

// The creature table.
TCreature *GetCreature(uint32 CreatureID);
void UpdateSkulls();
void HideCreature(int CreatureID); // a callback for Map.cpp
bool MoveCreatures();
void CheckMonsterHint(TCreature *Creature);

// Experience and skills.
int GetPlayerSkillValue(int SkillNumber);
int GetPlayerSkillProgress(int SkillNumber);
const char *GetSkillName(int SkillNumber);
bool HasPlayerSkull();
bool IsPlayerInParty();
int GetPlayerPartyFlag();

// The battle list.
bool IsOpponent(TCreature *Creature);
int GetNumberOfOpponents(); // also updates the list
TCreature *GetOpponent(int Number);

// The VIP list.
void EditBuddy(int CharacterID, int Icon, const char *Description, bool Notify);
void RemoveBuddy(uint32 CharacterID);
void SetBuddySortMethod(int Method);
void HideOfflineBuddies(bool Hide);
int GetNumberOfBuddies(); // only the online ones when they are hidden
uint32 GetBuddyListID(int Position);
const char *GetBuddyListName(int Position);
bool IsBuddyOnline(int Position);
bool IsBuddyNewlyOnline(int Position);
int GetBuddyListIcon(int Position);
const char *GetBuddyListDescription(int Position);
bool GetBuddyListNotify(int Position);

void InitCreatures();
void ExitCreatures();
void ResetCreatures();

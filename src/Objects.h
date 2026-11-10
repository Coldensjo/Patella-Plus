// Objects.h: the object, outfit, effect and missile properties read from
// Tibia.dat, and their accessors.
//
// Names from the binary: ObjectProperties, the Entry fields in the
// assertions (Flags, Width, Height, Layers, PatternWidth, PatternHeight,
// PatternDepth, Phases), the flags BANK, CUMULATIVE, LIQUIDCONTAINER,
// LIQUIDPOOL, HANG, LIGHT, SHIFT, HEIGHT, AUTOMAP and LENSHELP, CREATURE
// (MapWindow.cpp), and the parameter names Entry, Type, Number, Flag,
// Direction, Obj, X, Y, Layer, PatternX/Y/Z, Phase and FileName. The rest is
// ours (notes/decisions.md, "Objects.cpp").

#pragma once

#include "common.h"
#include "Container.h"
#include <cstddef>
#include <memory>

// A reference to an object (passed by value). Type is named in
// MapWindow.cpp's "SrcObj.Type!=CREATURE" (CREATURE is 99), Data in Dialogs.cpp's
// "In(DraggedObject.Data,1,100)" (the count); Data2 is ours. The default constructor
// clears the object: Map.cpp's TMapPoint, ten TObjects and plain ints, has a
// compiler-generated constructor that zeroes exactly the ten objects. Written with
// default member initializers so that the type stays an aggregate. MapWindow.cpp
// compares objects with numbers ("SrcObj!=0", "Obj!=CREATURE") and prints one as the
// int value of Obj in its catch wrapper: the original converts a TObject to its Type.
struct TObject {
	int Type = 0;
	int Data = 0;
	int Data2 = 0;
	operator int() const { return Type; }
};

// Object type of a creature on a map field ("SrcObj.Type!=CREATURE").
inline constexpr int CREATURE = 99;

// An object's position when it is not on the map, in the three coordinates of the Send functions and of the drag,
// trade and use objects: x is POSITION_NOT_ON_MAP, y an inventory slot (0: the first free one) or
// POSITION_CONTAINER_BASE plus the container number, and z the position in that container. As z,
// POSITION_PARENT_CONTAINER moves the object up to the container the y container is in. Enumerators for the reason
// in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	POSITION_NOT_ON_MAP = 0xffff,
	POSITION_CONTAINER_BASE = 0x40,
	POSITION_PARENT_CONTAINER = 0xfe,
};

// The way a creature faces: TCreature::Direction and MoveDirection, the player's PlayerStepDirection (Creatures.h),
// the direction the server sends with a creature and with a refused step (CancelPlayerWalk's NewDirection), and the
// Direction of GetOutfitSpriteNumber, which is the outfit's pattern column less one. Clockwise from north; the names
// are ours, from StartMovement, which turns a step's dx and dy into them (east or west for a diagonal step). The steps
// of a path have their own numbers, STEP_* in Map.h. Enumerators for the reason in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	DIRECTION_NORTH = 0,
	DIRECTION_EAST = 1,
	DIRECTION_SOUTH = 2, // the outfit dialog's and DrawOutfit's
	DIRECTION_WEST = 3,
};

// Tibia.dat flags: the bit numbers in TProperties::Flags. The names of
// 1-20 follow CipSoft's game server (medium confidence); 22, 23, 26 and 27
// are ours (low).
enum : int {  // NOLINT(cppcoreguidelines-use-enum-class): int constants ("The other ten stay unscoped"), the flags' bit numbers
	BANK = 0, // ground; reads Waypoints
	CLIP = 1,
	BOTTOM = 2,
	TOP = 3,
	CONTAINER = 4,
	CUMULATIVE = 5, // stackable; the count chooses the pattern
	FORCEUSE = 6,
	MULTIUSE = 7,
	WRITE = 8,     // reads MaxLength
	WRITEONCE = 9, // reads MaxLength
	LIQUIDCONTAINER = 10,
	LIQUIDPOOL = 11,
	UNPASS = 12, // also true for creatures
	UNMOVE = 13,
	UNTHROW = 14,
	AVOID = 15,
	TAKE = 16,
	HANG = 17,
	HOOKSOUTH = 18,
	HOOKEAST = 19,
	ROTATE = 20,
	LIGHT = 21, // reads Brightness and LightColor
	DONTHIDE = 22,
	TRANSLUCENT = 23,
	SHIFT = 24,  // reads ShiftX and ShiftY
	HEIGHT = 25, // reads Elevation
	LYINGOBJECT = 26,
	ANIMATEALWAYS = 27,
	AUTOMAP = 28,  // reads AutomapColor
	LENSHELP = 29, // reads LensHelp
	FULLBANK = 30, // ours: a ground that covers the whole field; MapWindow.cpp hides the levels below
};

// One Tibia.dat entry; the same layout serves objects, outfits,
// effects and missiles.
struct TProperties {
	int Width;
	int Height;
	int ExactSize; // 32 unless Width or Height is above 1
	int Layers;
	int PatternWidth;
	int PatternHeight;
	int PatternDepth;
	int Phases;
	uint16 *Sprites;  // Width*Height*Layers*PatternWidth*PatternHeight*PatternDepth*Phases
	uint8 Flags[4];   // bit Flag%8 of Flags[Flag/8]; NOLINT(modernize-avoid-c-arrays): an assertion reads *(int*)Entry.Flags, frozen until M5 (issue #60)
	int Waypoints;    // BANK
	int MaxLength;    // WRITE and WRITEONCE
	int Brightness;   // LIGHT
	int LightColor;   // LIGHT
	int ShiftX;       // SHIFT
	int ShiftY;       // SHIFT
	int Elevation;    // HEIGHT
	int AutomapColor; // AUTOMAP
	int LensHelp;     // LENSHELP
};

// The byte of Flag's bit, Flags[Flag / 8], for every flag up to 0xff as the original indexes it: a flag above 31 is a
// bit of the members after Flags (inside TProperties). Through the entry's bytes
// (Patella-Modern issue #91): Flags indexed past its four elements is undefined behaviour, which UBSan's bounds check
// reports.
inline uint8 &FlagByte(TProperties &Entry, int Flag)
{
	static_assert(offsetof(TProperties, Flags) + 0xff / 8 < sizeof(TProperties), "every flag's byte is in TProperties");
	return reinterpret_cast<uint8 *>(&Entry)[offsetof(TProperties, Flags) + static_cast<std::size_t>(Flag / 8)];
}
inline uint8 FlagByte(const TProperties &Entry, int Flag)
{
	return reinterpret_cast<const uint8 *>(&Entry)[offsetof(TProperties, Flags) + static_cast<std::size_t>(Flag / 8)];
}

namespace patella {

// Objects.cpp's data and functions (Patella-Modern issue #106, M7; notes/decisions.md, "#104: TClient and the
// subsystem objects" and "#106"): Tibia.dat's four tables, read once, then read-only, and the functions of the same
// names as members. TMapState owns it (MapState.h), first: TMap, TCreatures and TInventory read it. Of the data (.bss:
// every member starts empty) DatSignature and NoObject are public: Communication.cpp sends
// DatSignature and compares with NoObject, Inventory.cpp and Map.cpp return NoObject.
class TObjectTypes {
public:
	TObjectTypes() = default;
	TObjectTypes(const TObjectTypes &) = delete;
	TObjectTypes &operator=(const TObjectTypes &) = delete;
	TObjectTypes(TObjectTypes &&) = delete;
	TObjectTypes &operator=(TObjectTypes &&) = delete;
	~TObjectTypes() = default;

	// Object types. Type CREATURE has only UNPASS.
	[[nodiscard]] bool GetObjectFlag(int Type, int Flag) const;
	[[nodiscard]] bool GetObjectFlag(const TObject &Obj, int Flag) const { return GetObjectFlag(Obj.Type, Flag); }
	[[nodiscard]] int GetObjectWaypoints(int Type) const;
	[[nodiscard]] int GetObjectBrightness(int Type) const;
	[[nodiscard]] int GetObjectLightColor(int Type) const;
	[[nodiscard]] int GetObjectShiftX(int Type) const;
	[[nodiscard]] int GetObjectShiftY(int Type) const;
	[[nodiscard]] int GetObjectElevation(int Type) const;
	[[nodiscard]] int GetObjectAutomapColor(int Type) const;
	[[nodiscard]] int GetObjectLensHelp(int Type) const;
	[[nodiscard]] int GetObjectWidth(int Type) const;
	[[nodiscard]] int GetObjectHeight(int Type) const;
	[[nodiscard]] int GetObjectExactSize(int Type) const;
	[[nodiscard]] int GetObjectLayers(int Type) const;
	[[nodiscard]] int GetObjectPatternWidth(int Type) const;
	[[nodiscard]] int GetObjectPatternHeight(int Type) const;
	[[nodiscard]] int GetObjectPatternDepth(int Type) const;
	[[nodiscard]] int GetObjectPhases(int Type) const;
	[[nodiscard]] int GetObjectSpriteNumber(int Type, int X, int Y, int Layer, int PatternX, int PatternY, int PatternZ,
		int Phase) const;

	// Outfits; Direction 0-3 selects the pattern column.
	[[nodiscard]] bool GetOutfitFlag(int Number, int Flag) const;
	[[nodiscard]] int GetOutfitBrightness(int Number) const;
	[[nodiscard]] int GetOutfitLightColor(int Number) const;
	[[nodiscard]] int GetOutfitShiftX(int Number) const;
	[[nodiscard]] int GetOutfitShiftY(int Number) const;
	[[nodiscard]] int GetOutfitWidth(int Number) const;
	[[nodiscard]] int GetOutfitExactSize(int Number) const;
	[[nodiscard]] int GetOutfitHeight(int Number) const;
	[[nodiscard]] int GetOutfitLayers(int Number) const;
	[[nodiscard]] int GetOutfitPhases(int Number) const;
	[[nodiscard]] int GetOutfitSpriteNumber(int Number, int X, int Y, int Layer, int Direction, int Phase) const;

	// Effects.
	[[nodiscard]] bool GetEffectFlag(int Number, int Flag) const;
	[[nodiscard]] int GetEffectBrightness(int Number) const;
	[[nodiscard]] int GetEffectLightColor(int Number) const;
	[[nodiscard]] int GetEffectWidth(int Number) const;
	[[nodiscard]] int GetEffectHeight(int Number) const;
	[[nodiscard]] int GetEffectLayers(int Number) const;
	[[nodiscard]] int GetEffectPhases(int Number) const;
	[[nodiscard]] int GetEffectSpriteNumber(int Number, int X, int Y, int Layer, int Phase) const;

	// Missiles; Direction 0-7 clockwise from north.
	[[nodiscard]] bool GetMissileFlag(int Number, int Flag) const;
	[[nodiscard]] int GetMissileBrightness(int Number) const;
	[[nodiscard]] int GetMissileLightColor(int Number) const;
	[[nodiscard]] int GetMissileWidth(int Number) const;
	[[nodiscard]] int GetMissileHeight(int Number) const;
	[[nodiscard]] int GetMissileLayers(int Number) const;
	[[nodiscard]] int GetMissileSpriteNumber(int Number, int X, int Y, int Layer, int Direction) const;

	// The pattern (1-based) that shows an object's count, liquid or wall side.
	void GetCumulativePattern(TObject &Obj, int *PatternX, int *PatternY) const;
	void GetLiquidPattern(TObject &Obj, int *PatternX, int *PatternY) const;
	void GetHangPattern(TObject &Obj, int *PatternX, int *PatternY) const;

	void ReleaseObjects();
	void LoadObjects(const char *FileName);
	void InitObjects();
	void ExitObjects();

private:
	// Data, in .bss order. The four tables own their arrays (std::unique_ptr since issue #58; the entries' Sprites
	// stay raw, freed by ReleaseObjects).
	std::unique_ptr<array<TProperties>> MissileProperties;
	std::unique_ptr<array<TProperties>> EffectProperties;
	std::unique_ptr<array<TProperties>> OutfitProperties;
	std::unique_ptr<array<TProperties>> ObjectProperties; // from 100

public:
	uint32 DatSignature{}; // sent at login and when patching
	TObject NoObject;      // never written; owner inferred from the address
};

// The transition's access path (issue #106; notes/decisions.md, "#104: TClient and the subsystem objects"): the object
// types of the one TClient (Client.cpp). It goes when no free function needs it.
namespace bridge {
TObjectTypes &ObjectTypes();
} // namespace bridge

} // namespace patella

// The original's functions (the callers' names, and the golden tests'): each calls TObjectTypes' member of the same
// name on bridge::ObjectTypes().
// Object types. Type CREATURE has only UNPASS.
bool GetObjectFlag(int Type, int Flag);
inline bool GetObjectFlag(TObject &Obj, int Flag) { return GetObjectFlag(Obj.Type, Flag); }
int GetObjectBrightness(int Type);
int GetObjectLightColor(int Type);
int GetObjectShiftX(int Type);
int GetObjectShiftY(int Type);
int GetObjectElevation(int Type);
int GetObjectLensHelp(int Type);
int GetObjectWidth(int Type);
int GetObjectHeight(int Type);
int GetObjectExactSize(int Type);
int GetObjectLayers(int Type);
int GetObjectPatternWidth(int Type);
int GetObjectPatternHeight(int Type);
int GetObjectPatternDepth(int Type);
int GetObjectPhases(int Type);
int GetObjectSpriteNumber(int Type, int X, int Y, int Layer, int PatternX, int PatternY, int PatternZ,
	int Phase);

// Outfits; Direction 0-3 selects the pattern column.
bool GetOutfitFlag(int Number, int Flag);
int GetOutfitBrightness(int Number);
int GetOutfitLightColor(int Number);
int GetOutfitShiftX(int Number);
int GetOutfitShiftY(int Number);
int GetOutfitWidth(int Number);
int GetOutfitExactSize(int Number);
int GetOutfitHeight(int Number);
int GetOutfitLayers(int Number);
int GetOutfitPhases(int Number);
int GetOutfitSpriteNumber(int Number, int X, int Y, int Layer, int Direction, int Phase);

// Effects.
bool GetEffectFlag(int Number, int Flag);
int GetEffectBrightness(int Number);
int GetEffectLightColor(int Number);
int GetEffectWidth(int Number);
int GetEffectHeight(int Number);
int GetEffectLayers(int Number);
int GetEffectPhases(int Number);
int GetEffectSpriteNumber(int Number, int X, int Y, int Layer, int Phase);

// Missiles; Direction 0-7 clockwise from north.
bool GetMissileFlag(int Number, int Flag);
int GetMissileBrightness(int Number);
int GetMissileLightColor(int Number);
int GetMissileWidth(int Number);
int GetMissileHeight(int Number);
int GetMissileLayers(int Number);
int GetMissileSpriteNumber(int Number, int X, int Y, int Layer, int Direction);

// The pattern (1-based) that shows an object's count, liquid or wall side.
void GetCumulativePattern(TObject &Obj, int *PatternX, int *PatternY);
void GetLiquidPattern(TObject &Obj, int *PatternX, int *PatternY);
void GetHangPattern(TObject &Obj, int *PatternX, int *PatternY);

void InitObjects();
void ExitObjects();

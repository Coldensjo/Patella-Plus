// Map.h: the client's map. The visible part of the world is a cube of
// MAPSIZE_X x MAPSIZE_Y x MAPSIZE_Z map points with up to OBJECTS_PER_POINT
// objects each; graphical effects, animated texts and missiles live in a
// table of EFFECTS entries linked to the points; the automap keeps the colours
// and speeds of the explored world in blocks of 256x256 fields that are cached
// in memory and stored in files "<bx><by><bz>.map".
//
// Coordinates: rx, ry, rz are map (relative) coordinates, x, y, z world
// (absolute) ones. rz grows upwards; the player's floor is rz 7-z above
// ground and rz 2 below it (GetMapLevel).

#pragma once

#include "common.h"
#include "AutomapLayer.h" // TAutomapLayer
#include "types/FixedString.h"
#include "types/IndexedVector.h"
#include "types/SlotTable.h"
#include "Container.h"
#include "types/GraphicsTypes.h" // TRGBColor
#include "MapPoint.h"            // TMapPoint, OBJECTS_PER_POINT
#include "Objects.h"             // TObject
#include <array>
#include <cstddef>
#include <iterator> // std::default_sentinel_t, TMapEffects
#include <vector>

inline constexpr int MAPSIZE_X = 18;
inline constexpr int MAPSIZE_Y = 14;
inline constexpr int MAPSIZE_Z = 8;
inline constexpr int EFFECTS = 200;

// An effect. Status 0 is a free entry, 1 a graphical effect, 2 an animated text, 3 a missile. The three kinds share the
// space after Type. Its anonymous structs are a Microsoft extension (GCC and Clang have it too), which MSVC's C4201 and
// -Wpedantic name: allowed for TEffect alone (their members are TEffect's own names, Text, Direction..., throughout
// Map.cpp and the windows).
PATELLA_MSVC_WARNING(push)
PATELLA_MSVC_WARNING(disable : 4201)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
struct TEffect {
	int Status;
	int Type; // the colour of a text
	union {
		int Phase;                 // graphical effect
		struct {                   // animated text
			TFixedString<12> Text; // at most 9 characters
			int TextPhase;
		};
		struct {           // missile; world coordinates
			int Direction; // 0 north, clockwise, 4 south
			int DestX;
			int DestY;
			int DestZ;
			int x; // the field it is linked to
			int y;
			int OffsetX; // pixels from the destination
			int OffsetY;
			int EndTime;
			int DistanceX; // abs(start - destination) * 32
			int DistanceY;
			int Duration; // milliseconds
		};
	};
	int Field;          // the map point (index into the point table, not an effect)
	TEffectId Previous; // effects of the same point; None for the first
	TEffectId Next;     // None for the last
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
PATELLA_MSVC_WARNING(pop)

// An automap mark. The constructor clears it.
struct TMark {
	TMark()
	{
		x = 0;
		y = 0;
		Type = 0;
		Text[0] = 0;
	}

	int x; // world coordinates; 0,0 is a deleted mark
	int y;
	int Type; // the mark symbol
	TFixedString<100> Text;
};

// A world position, as passed to AddAutomapMark ("Coordinate.x").
struct TCoordinate {
	int x;
	int y;
	int z;
};

// One automap block: 256x256 fields of one floor. The compiler-generated
// destructor frees Marks. Color and Speed are TAutomapLayer
// (AutomapLayer.h): the bytes of the original's uint8 [256][256] in the
// same order, indexed through Color.view()[x, y].
struct TAutomapBlock {
	TAutomapBlock() : Marks(0, 0, 10) { MarkCount = 0; }

	int bx; // -1 for an unused block
	int by;
	int bz;
	int Previous;        // the cache's LRU list, toward the most recent
	int Next;            // toward the least recent
	TAutomapLayer Color; // [x, y], palette colour
	TAutomapLayer Speed; // [x, y], ground speed, 250 unknown, 255 blocked
	bool Changed;
	// The marks from 0 to MarkCount - 1. A TIndexedVector: LoadAutomapBlock and AddAutomapMark write at an index, and
	// the vector grows to hold it; the reload clears min to max.
	TIndexedVector<TMark> Marks;
	int MarkCount;
};

// A node of FindPath's search.
struct TPathNode {
	TPathNode()
	{
		Cost = -1;
		Estimate = -1;
		Predecessor = nullptr;
		Next = nullptr;
	}

	int x; // relative to the start
	int y;
	bool Blocked;
	int Cost; // from the destination
	int Speed;
	int Estimate;           // cost plus the estimate to the start
	TPathNode *Predecessor; // toward the destination
	TPathNode *Next;        // the open list, sorted by Estimate
};

// The steps of a path: FindPath's Directions, the player's PlayerSteps (Creatures.h) and the bytes of SendWalk's
// path. Counter-clockwise from east, with the diagonals, from 1; the names are ours, from FindPath and StartPlayerStep,
// which turn the differences of x and y into them and back. They are not a creature's DIRECTION_* (Objects.h),
// four values clockwise from north: a step becomes one in StartMovement, by its dx and dy. Enumerators for the
// reason in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	STEP_EAST = 1,
	STEP_NORTHEAST = 2,
	STEP_NORTH = 3,
	STEP_NORTHWEST = 4,
	STEP_WEST = 5,
	STEP_SOUTHWEST = 6,
	STEP_SOUTH = 7,
	STEP_SOUTHEAST = 8,
};

inline constexpr int AUTOMAP_BLOCKS = 10;

// The effect table: EFFECTS entries, numbered by TEffectId.
using TEffectTable = TSlotTable<TEffect, EFFECTS, TEffectId>;

// The effects of one map point, first to last, by their Next links: TEffectTable::Chain from the point's EffectNumber
// (TMap::PointEffects), the range Map.cpp's loops walk. A loop's body may delete the effect it is given (ClearField
// does): DeleteEffect keeps the deleted entry's Next, which the step reads.
using TPointEffects = TEffectTable::TChain;

namespace patella {

class TObjectTypes;

// The default console, where TMap reports an automap file it could not read or save: Talk.cpp's, TGUI's, a later
// subsystem than TMapState, so TMap reaches it only through this interface; Client.cpp's adapter implements it with
// TTalk's members. HasDefaultConsole is the original's check of DefaultTextBuffer before the message is made,
// AddDefaultConsoleLine its AddTextBufferLine(DefaultTextBuffer, 0, Message, 20, NULL, 0) and SetConsoleChanged.
class TMapConsole {
public:
	TMapConsole() = default;
	TMapConsole(const TMapConsole &) = delete;
	TMapConsole &operator=(const TMapConsole &) = delete;
	TMapConsole(TMapConsole &&) = delete;
	TMapConsole &operator=(TMapConsole &&) = delete;
	virtual bool HasDefaultConsole() = 0;
	virtual void AddDefaultConsoleLine(const char *Message) = 0;

protected:
	~TMapConsole() = default;
};

// Map.cpp's data and functions: the map points and their position, the effect table and the two iterators, the automap
// blocks and their cache list, the ambient light, all private (Fields and FieldNumber start as their initialisers make
// them, the others 0). The functions are the members of the same names, the file-local ones private. TMapState owns it
// (MapState.h), after TObjectTypes, which it reads.
class TMap {
public:
	TMap(TObjectTypes &ObjectTypes, TMapConsole &Console);
	TMap(const TMap &) = delete;
	TMap &operator=(const TMap &) = delete;
	TMap(TMap &&) = delete;
	TMap &operator=(TMap &&) = delete;
	~TMap() = default;

	// Position and coordinates.
	void SetMapPosition(int x, int y, int z);
	void GetMapPosition(int *x, int *y, int *z) const;
	[[nodiscard]] int GetMapLevel() const;
	void MapToWorld(int *x, int *y, int *z) const;
	void WorldToMap(int *x, int *y, int *z) const;

	// Ambient light.
	void SetAmbientLight(int Brightness, int Color);
	void UpdateAmbientLight();
	[[nodiscard]] TRGBColor GetAmbientColor() const;
	[[nodiscard]] int GetAmbientBrightness() const;

	// Objects.
	TObject GetFirstObject(int rx, int ry, int rz);
	TObject GetNextObject();
	TObject GetMapObject(int rx, int ry, int rz, int Position);
	TObject GetTopMoveObject(int rx, int ry, int rz, int &Position);
	TObject GetTopLookObject(int rx, int ry, int rz, int &Position);
	TObject GetTopUseObject(int rx, int ry, int rz, int &Position);
	TObject GetTopMultiUseObject(int rx, int ry, int rz, int &Position);
	void AddObject(int rx, int ry, int rz, TObject Obj);
	void AppendObject(int rx, int ry, int rz, TObject Obj);
	bool DeleteObject(int rx, int ry, int rz, int Position);
	void ChangeObject(int rx, int ry, int rz, int Position, TObject Obj);

	// Effects.
	TEffectId GetFirstEffect(int rx, int ry, int rz);
	TEffectId GetNextEffect();
	[[nodiscard]] int GetEffectStatus(TEffectId EffectNumber) const;
	[[nodiscard]] int GetGraphicalEffectType(TEffectId EffectNumber) const;
	[[nodiscard]] int GetGraphicalEffectPhase(TEffectId EffectNumber) const;
	[[nodiscard]] int GetTextualEffectColor(TEffectId EffectNumber) const;
	char *GetTextualEffectText(TEffectId EffectNumber);
	[[nodiscard]] int GetTextualEffectPhase(TEffectId EffectNumber) const;
	[[nodiscard]] int GetMissileEffectType(TEffectId EffectNumber) const;
	[[nodiscard]] int GetMissileEffectDirection(TEffectId EffectNumber) const;
	[[nodiscard]] int GetMissileEffectX(TEffectId EffectNumber) const;
	[[nodiscard]] int GetMissileEffectY(TEffectId EffectNumber) const;
	void AddGraphicalEffect(int rx, int ry, int rz, int Type);
	void AddTextualEffect(int rx, int ry, int rz, int Color, const char *Text);
	void AddMissileEffect(int sx, int sy, int sz, int dx, int dy, int dz, int Type);
	void ProcessEffects();
	void ProcessMissileEffects();

	// Map points and scrolling.
	void ClearField(int rx, int ry, int rz, bool DeleteEffects);
	void ClearMap();
	void ShiftMap(int dx, int dy);
	void ShiftMapLevel(int dz);
	void RequestFieldRefresh(int rx, int ry, int rz);
	int GetFieldHeight(int rx, int ry, int rz);
	bool IsFieldPassable(int rx, int ry, int rz, bool CheckHeight);
	bool IsFieldThrowable(int rx, int ry, int rz);
	bool IsFieldTranslucent(int rx, int ry, int rz);
	bool FieldRefreshRequested(int rx, int ry, int rz);

	// The automap. The block cache and the file functions are static.
	bool TakeAutomapChanged();
	void UpdateAutomap(int rx, int ry);
	TRGBColor GetAutomapColor(int x, int y, int z);
	void GetAutomapMarks(int x, int y, int z, std::vector<TMark> *Marks, int *Count);
	void AddAutomapMark(TCoordinate &Coordinate, const char *Text, int Type);
	void DeleteAutomapMark(TMark &Deleted, int z);

	// Paths, initialisation.
	int FindPath(int StartX, int StartY, int StartZ, int DestX, int DestY, int DestZ, bool MustReach, int *Steps,
		int *Directions, bool ForceDiagonal);
	void InitMap(void (*Callback)(int CreatureID));
	void ExitMap();

private:
	void SortObjects(int rx, int ry, int rz);
	void InsertObject(int rx, int ry, int rz, int Position, TObject Obj);
	[[nodiscard]] TPointEffects PointEffects(const TMapPoint &Point) const noexcept { return Effects.Chain(Point.EffectNumber); }
	TEffectId GetEffectIteratorNumber(int n);
	TEffectId NewEffect(int Status, int rx, int ry, int rz);
	void DeleteEffect(TEffectId Number);
	void ShiftMapWest();
	void ShiftMapEast();
	void ShiftMapNorth();
	void ShiftMapSouth();
	void ShiftMapUp();
	void ShiftMapDown();
	void GetFieldAutomapInfo(int rx, int ry, int rz, uint8 *Color, uint8 *Speed);
	void LoadAutomapBlock(int Position, int bx, int by, int bz);
	void SaveAutomapBlock(int Position);
	TAutomapBlock *GetAutomapBlock(int bx, int by, int bz);
	bool IsAutomapBlocked(int x, int y, int z);
	int GetAutomapSpeed(int x, int y, int z);

	TObjectTypes &ObjectTypes;
	TMapConsole &Console;

	void (*CreatureCallback)(int CreatureID){}; // called for every creature ClearField removes
	int AmbienteActualBrightness{};             // follows AmbienteBrightness step by step
	int AmbienteBrightness{};                   // -1: take the next value at once
	bool AutomapChanged{};
	int LeastRecentBlock{}; // tail of the block cache's LRU list
	int MostRecentBlock{};  // head
	int EffectIteratorCount{};
	int EffectIteratorField{};
	int ObjectIteratorCount{};
	int ObjectIteratorField{};
	int MapZ{}; // world position of the map's centre
	int MapY{};
	int MapX{};
	TEffectTable Effects;
	array<TMapPoint> Fields{0, MAPSIZE_X * MAPSIZE_Y * MAPSIZE_Z - 1};
	matrix3d<int> FieldNumber{0, MAPSIZE_X - 1, 0, MAPSIZE_Y - 1, 0, MAPSIZE_Z - 1};
	std::array<TAutomapBlock, AUTOMAP_BLOCKS> AutomapBlocks;
	TRGBColor AmbienteColor;
	TRGBColor AmbienteActualColor;
};

} // namespace patella

// The one client's map (Client.cpp).
patella::TMap &Map();

// The effects of map point rx, ry, rz by the map's GetFirstEffect and GetNextEffect, as the original's numbers:
// TMapWindow::DrawField's effect loop. begin() calls GetFirstEffect, each step GetNextEffect, in the original loop's
// order; the range ends at the first -1, and gives any other number as it comes (the loop checks the range itself).
class TMapEffects {
public:
	class TIterator {
	public:
		using value_type = int;
		using difference_type = std::ptrdiff_t;

		TIterator() = default;
		explicit TIterator(int First) noexcept : Number(First) {}

		[[nodiscard]] int operator*() const noexcept { return Number; }
		TIterator &operator++()
		{
			Number = EffectNumberOf(Map().GetNextEffect());
			return *this;
		}
		TIterator operator++(int)
		{
			TIterator Before = *this;
			++*this;
			return Before;
		}
		[[nodiscard]] bool operator==(std::default_sentinel_t /*End*/) const noexcept { return Number == -1; }

	private:
		int Number = -1;
	};

	TMapEffects(int rx, int ry, int rz) noexcept : FieldX(rx), FieldY(ry), FieldZ(rz) {}

	[[nodiscard]] TIterator begin() const
	{
		return TIterator{EffectNumberOf(Map().GetFirstEffect(FieldX, FieldY, FieldZ))};
	}
	[[nodiscard]] static std::default_sentinel_t end() noexcept { return std::default_sentinel; }

private:
	int FieldX;
	int FieldY;
	int FieldZ;
};

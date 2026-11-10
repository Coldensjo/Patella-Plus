// graphics.h: the sprites of Tibia.spr, the pictures of Tibia.pic, the fonts
// (cut from pictures), the memory cache of decoded 32x32 tiles, the video
// cache of tiles on the engine's cache surfaces, and the drawing functions
// that put sprites, objects, outfits, pictures and text on a surface.
//
// A sprite is 32x32 pixels; a picture is TilesX x TilesY tiles of 32x32.
// Decoded tiles are 32*32 RGB triples (3072 bytes). The memory cache holds
// MemoryCacheSize of them (InitGraphics' MemoryCacheBytes / 3072), the video
// cache VideoCacheSize slots on the cache surfaces (surfaces 6, 7, ...); both
// are LRU lists with a hash table of HASH_TABLE_SIZE chains.

#pragma once

#include "Skins.h"
#include "Surface.h"
#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include "types/GraphicsTypes.h" // IWYU pragma: export (TRect, TRGBColor, TFont)
#include <array>
#include <memory>

struct TObject; // Objects.h, passed to DrawObject

// The original's PATH_MAX (its assertions' name), renamed: mingw-w64's and the Linux headers define PATH_MAX as a
// macro.
inline constexpr int GRAPHICS_PATH_MAX = 1024;
inline constexpr int HASH_TABLE_SIZE = 100;

// Kinds of cache items. Pictures and font characters are both PICTURE_ITEM. An unscoped int enum: the items' Type is an
// int (as Objects.h's flags and Talk.h's modes).
enum : int { // NOLINT(cppcoreguidelines-use-enum-class)
	EMPTY_ITEM = 0,
	SPRITE_ITEM = 1,
	PICTURE_ITEM = 2,
};

// A sprite of Tibia.spr. Offset 0 is an empty sprite.
struct TSprite {
	int Offset;         // in Tibia.spr
	TRGBColor ColorKey; // the transparent colour, from the sprite's first three bytes
};

// A picture of Tibia.pic; "Picture" in the assertions.
struct TPicture {
	int TilesX;
	int TilesY;
	int *Offsets; // TilesX*TilesY offsets in Tibia.pic, row by row
	TRGBColor ColorKey;
};

// A font cut from a picture. Characters 33-127 have the
// indices 1-95, characters 128-255 the indices 96-223.
struct TFontInfo {
	int PictureNumber;
	int CharsPerLine;
	int CharWidth;
	int CharHeight;
	std::array<int, 224> Widths;  // the visible width of each character
	std::array<int, 224> Heights; // the visible height of each character
	int Spacing;                  // added after every character
	int Height;                   // the highest visible height
	int LineHeight;
};

// A decoded tile in the memory cache; "Item" in the assertions.
struct TMemoryCacheItem {
	uint8 *Data; // 32*32 RGB
	int Type;
	int Number; // sprite or picture number
	int TileX;
	int TileY;
	int Previous; // LRU list: towards the most recent item
	int Next;
	int HashPrevious;
	int HashNext;
	int HashValue;
};

// A tile on a cache surface. Masked sprites (outfits) keep the mask and four colours, pictures the tile, font
// characters the tile and the text colour, in the same place. Tile's type is unnamed inside an anonymous union, an
// extension of every compiler here that Clang's -Wpedantic names (-Wnested-anon-types): allowed for this structure
// alone, as Map.h's TEffect.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif
struct TVideoCacheItem {
	TVideoCacheItem();
	int Surface; // cache surface; the engine's surface number is Surface+6
	int x;
	int y;
	int Type;
	int Number; // sprite or picture number
	union {
		int MaskNumber; // sprites; 0 without mask
		int TileX;      // pictures
	};
	union {
		std::array<TRGBColor, 4> Colors; // masked sprites
		// A named member: GCC and Clang allow no member with a constructor (Color) in an anonymous struct.
		struct {
			int TileY;       // pictures
			TRGBColor Color; // font characters; 0,0,0 for pictures
		} Tile;
	};
	int Previous;
	int Next;
	int HashPrevious;
	int HashNext;
	int HashValue;
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// TGraphics, the fifth of TClient's subsystems: graphics.cpp's data and functions (its members of the same names; the
// free functions below forward to them), Surface.cpp's TGraphicsEngine (Surface.h) and Skins.cpp's TSkins with
// GUI.cpp's SkinProperties (Skins.h). Of graphics.cpp's data the two signatures are public: the login packets send
// them.
namespace patella {

class TGraphics {
public:
	TGraphics() = default;
	TGraphics(const TGraphics &) = delete;
	TGraphics &operator=(const TGraphics &) = delete;
	TGraphics(TGraphics &&) = delete;
	TGraphics &operator=(TGraphics &&) = delete;
	~TGraphics() = default;

	// Start-up and shut-down.
	void InitGraphics(int MemoryCacheBytes);
	void InitVideoCache();
	void ResetVideoCache();
	void ExitGraphics();

	// The files and the fonts.
	void LoadSprites(const char *FileName);
	void LoadSprite(int SpriteNumber, uint8 *Buffer);
	void LoadPictures(const char *FileName);
	void LoadPictureTile(int PictureNumber, int TileX, int TileY, uint8 *Buffer);
	void InitFont(int FontName, int PictureNumber, int CharsPerLine, int CharWidth, int CharHeight,
		int Spacing, int LineHeight);
	void InitFonts();

	// The memory cache.
	void InitMemoryCache(int MemoryCacheBytes);
	void TouchMemoryItem(int Position);
	[[nodiscard]] int FindMemorySprite(int SpriteNumber) const;
	[[nodiscard]] int FindMemoryPicture(int PictureNumber, int TileX, int TileY) const;
	void RemoveMemoryItem(int Position);
	void InsertMemoryItem(int Position, int HashValue);
	TMemoryCacheItem *GetMemorySprite(int SpriteNumber);
	TMemoryCacheItem *GetMemoryPicture(int PictureNumber, int TileX, int TileY);

	// The video cache.
	void TouchVideoItem(int Position);
	[[nodiscard]] int FindVideoSprite(int SpriteNumber) const;
	int FindVideoMaskedSprite(int SpriteNumber, int MaskNumber, TRGBColor *Colors) const;
	[[nodiscard]] int FindVideoPicture(int PictureNumber, int TileX, int TileY) const;
	[[nodiscard]] int FindVideoFontChar(int PictureNumber, int TileX, int TileY, TRGBColor Color) const;
	void RemoveVideoItem(int Position);
	void InsertVideoItem(int Position, int HashValue);
	TVideoCacheItem *GetVideoSprite(int SpriteNumber);
	TVideoCacheItem *GetVideoMaskedSprite(int SpriteNumber, int MaskNumber, TRGBColor *Colors);
	TVideoCacheItem *GetVideoPicture(int PictureNumber, int TileX, int TileY);
	TVideoCacheItem *GetVideoFontChar(int FontNumber, int TileX, int TileY, TRGBColor Color);

	// Sprites, objects and outfits. Sprite number 0 draws nothing (except in the
	// masked forms, where it fails the assertion).
	void DrawSprite(int Surface, int x, int y, int SpriteNumber);
	void DrawSpriteClipped(int Surface, int x, int y, int SpriteNumber, TRect Clip);
	void DrawSpriteScaled(int Surface, int x, int y, int Width, int Height, int SpriteNumber,
		TRGBColor Highlight, TRect Clip);
	void DrawMaskedSprite(int Surface, int x, int y, int SpriteNumber, int MaskNumber, TRGBColor *Colors);
	void DrawMaskedSpriteScaled(int Surface, int x, int y, int Width, int Height, int SpriteNumber,
		int MaskNumber, TRGBColor *Colors, TRGBColor Highlight, TRect Clip);
	void DrawObject(int Surface, int x, int y, int Size, TObject Obj, TRGBColor Highlight, TRect Clip,
		TFont Font, bool ShowCount);
	// Colors: the four outfit colours (TCreature::Colors); a fixed extent, passed as the pointer was.
	void DrawOutfit(int Surface, int x, int y, int Size, int OutfitNumber, patella::TSpan<const int, 4> Colors,
		TRGBColor Highlight, TRect Clip, TFont Font);

	// Pictures.
	void DrawPicture(int Surface, int x, int y, int PictureNumber);
	void DrawPicturePart(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
		int Width, int Height);
	void DrawPicturePartClipped(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
		int Width, int Height, TRect Clip);

	// Text. Characters 10 and 32 are blanks, other control characters show
	// character 127.
	void PrintTextPart(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
		int Length, int Alignment);
	void PrintTextClipped(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
		int Alignment, TRect Clip);
	void PrintText(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
		int Alignment);
	void PrintFormattedText(int Surface, int x, int y, TFont Format, const char *Text);
	void PrintFormattedTextClipped(int Surface, int x, int y, TFont Format, const char *Text, TRect Clip);
	void PrintFormattedTextPart(int Surface, int x, int y, TFont Format, const char *Text, int Length);
	[[nodiscard]] int GetFontLineHeight(int FontNumber) const;
	[[nodiscard]] int GetFontHeight(int FontNumber) const;
	int GetTextHeight(int FontNumber, const char *Text) const;
	int GetTextWidth(int FontNumber, const char *Text) const;
	int GetTextPartWidth(int FontNumber, const char *Text, int Length) const;
	int GetFittingTextLength(int FontNumber, const char *Text, int Width) const;
	void GetTextPosition(int *TextX, int *TextY, int x, int y, int Width, int Height, TFont *Format) const;

	// Surface.cpp's and Skins.cpp's (with GUI.cpp's SkinProperties) objects.
	TGraphicsEngine Engine;
	TSkins Skins;

private:
	// graphics.cpp's data.
	std::array<int, HASH_TABLE_SIZE> MemoryHashTable{};
	std::array<int, HASH_TABLE_SIZE> VideoHashTable{};
	int NumberOfFonts{};
	int NumberOfPictures{};

public:
	uint32 PictureSignature{}; // sent at login

private:
	TFixedString<GRAPHICS_PATH_MAX> PictureFileName{};
	int NumberOfSprites{};

public:
	uint32 SpriteSignature{}; // sent at login

private:
	TFixedString<GRAPHICS_PATH_MAX> SpriteFileName{};
	int LeastRecentMemoryItem{}; // tail of the memory cache's LRU list
	int MostRecentMemoryItem{};  // head
	int MemoryCacheSize{};
	std::unique_ptr<uint8[]> MemoryCacheData{}; // MemoryCacheSize tiles of 32*32*3 bytes
	int LeastRecentVideoItem{};
	int MostRecentVideoItem{};
	int VideoCacheSize{};
	std::unique_ptr<TVideoCacheItem[]> VideoCache{};
	std::unique_ptr<TMemoryCacheItem[]> MemoryCache{};
	std::unique_ptr<TSprite[]> Sprites{};
	std::unique_ptr<TPicture[]> Pictures{};
	std::unique_ptr<TFontInfo[]> Fonts{};
};

} // namespace patella

// The one client's graphics (Client.cpp).
patella::TGraphics &Graphics();

// The original's free functions (each forwards to its TGraphics member on the bridge).

// Start-up and shut-down.
void InitGraphics(int MemoryCacheBytes);
void InitVideoCache();
void ResetVideoCache();
void ExitGraphics();

// Sprites, objects and outfits. Sprite number 0 draws nothing (except in the
// masked forms, where it fails the assertion).
void DrawSprite(int Surface, int x, int y, int SpriteNumber);
void DrawSpriteClipped(int Surface, int x, int y, int SpriteNumber, TRect Clip);
void DrawMaskedSprite(int Surface, int x, int y, int SpriteNumber, int MaskNumber, TRGBColor *Colors);
void DrawObject(int Surface, int x, int y, int Size, TObject Obj, TRGBColor Highlight, TRect Clip,
	TFont Font, bool ShowCount);
// Colors: the four outfit colours (TCreature::Colors); a fixed extent, passed as the pointer was.
void DrawOutfit(int Surface, int x, int y, int Size, int OutfitNumber, patella::TSpan<const int, 4> Colors,
	TRGBColor Highlight, TRect Clip, TFont Font);

// Pictures.
void DrawPicture(int Surface, int x, int y, int PictureNumber);
void DrawPicturePart(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height);
void DrawPicturePartClipped(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height, TRect Clip);

// Text. Characters 10 and 32 are blanks, other control characters show
// character 127.
void PrintTextPart(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Length, int Alignment);
void PrintTextClipped(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Alignment, TRect Clip);
void PrintText(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Alignment);
void PrintFormattedText(int Surface, int x, int y, TFont Format, const char *Text);
void PrintFormattedTextClipped(int Surface, int x, int y, TFont Format, const char *Text, TRect Clip);
void PrintFormattedTextPart(int Surface, int x, int y, TFont Format, const char *Text, int Length);
int GetFontLineHeight(int FontNumber);
int GetFontHeight(int FontNumber);
int GetTextHeight(int FontNumber, const char *Text);
int GetTextWidth(int FontNumber, const char *Text);
int GetTextPartWidth(int FontNumber, const char *Text, int Length);
int GetFittingTextLength(int FontNumber, const char *Text, int Width);
void GetTextPosition(int *TextX, int *TextY, int x, int y, int Width, int Height, TFont *Format);

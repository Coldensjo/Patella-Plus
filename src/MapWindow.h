// MapWindow.h: the game window's map view and the frame around it.
//
// TMapWindow draws the visible part of the map: the levels from the bottom up,
// field by field, with objects, creatures, effects and the light of the light
// sources, then the creature names and health bars, the talkboxes, the status
// message and the animated texts. The 32x32 fields are composed in a cache of
// 16x12 tiles on surface 4 (TCacheField) and blitted to surface 2, the 480x352
// map surface; the map surface is copied (or stretched) to the screen.
// TMapFrame is the window MainWindow.cpp creates: it holds the map view, keeps
// it at an aspect ratio of 15:11 and draws the border.

#pragma once

#include "common.h"
#include "GUI.h"
#include "Map.h" // MAPSIZE_X, MAPSIZE_Y, EFFECTS, GetAmbientColor
#include "Creatures.h"
#include "Objects.h"
#include "graphics.h"
#include <array>

// The map surface in fields: 480x352 pixels.
inline constexpr int MAP_WIDTH = 15;
inline constexpr int MAP_HEIGHT = 11;

// A creature whose name and health bar are drawn above it.
struct TCreatureName {
	uint32 CreatureID;
	int x; // centre of the creature on the map surface
	int y; // its top
};

// An animated text to be drawn after the map.
struct TTextualEffectPosition {
	int EffectNumber;
	int x; // centre on the map surface
	int y;
	int FieldX; // the field it is on
	int FieldY;
	int FieldZ;
};

// A 32x32 tile of the field cache on surface 4: the sprites that
// make up a field, each with its offset. Sprite 0 is the empty tile of
// picture 7. Sprites counts the sprites of the current frame; while they are
// the same as the first CachedSprites of the tile, nothing is drawn again.
struct TCacheField {
	int x; // position on surface 4
	int y;
	int CachedSprites; // sprites in the tile
	std::array<int, 10> Sprite;
	std::array<int, 10> SpriteX;
	std::array<int, 10> SpriteY;
	int Sprites;
	bool Copied; // blitted to the map surface in this frame
};

class TMapWindow : public TWindow {
public:
	TMapWindow();
	// Its two RemoveHotkey calls are the close's (OnClose).
	~TMapWindow() override { CheckClosed("TMapWindow"); }
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return 240; }
	int GetMinimumHeight() override { return 176; }
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool HelpMouseMove(int x, int y) override;
	void HandleEvent(int Event) override;

	void DrawField(int posx, int posy, int FieldX, int FieldY, int FieldZ, int Mode);
	void DrawObject(int posx, int posy, int Level, TObject Obj, int x, int y, int z);
	void DrawCreature(int posx, int posy, int Level, TCreature *Creature);
	void DrawLightMask(int posx, int posy, int Brightness);
	void DrawFieldSprite(int x, int y, int Level, bool Animated, int SpriteNumber, int MaskNumber,
		TRGBColor *Colors);
	void CacheSprite(int FieldX, int FieldY, int SpriteNumber, int dx, int dy);
	void FlushField(int FieldX, int FieldY);
	TCacheField *GetCacheField(int FieldX, int FieldY);
	void AddLightSource(int posx, int posy, int z, int Brightness, TRGBColor Color);
	void ArrangeTalkboxes();
	void GetField(int x, int y, int *FieldX, int *FieldY, int *FieldZ);
	uint32 GetCreatureAt(int x, int y);

	// A sprite at Level shows on a field unless a ground above Level covers
	// it.
	bool IsFieldVisible(int FieldX, int FieldY, int Level)
	{
		if (!In(FieldX, 0, MAPSIZE_X - 1) || !In(FieldY, 0, MAPSIZE_Y - 1))
			return false;
		return CoverLevel[FieldX][FieldY] <= Level;
	}

	// The ambient colour at a brightness of 0-255.
	void SetLightColor(int FieldX, int FieldY, int Brightness)
	{
		if (Brightness < 0)
			Brightness = 0;
		else if (Brightness > 255)
			Brightness = 255;
		const TRGBColor Color = Map().GetAmbientColor();
		Light[FieldX][FieldY].R = Color.R * Brightness / 255;
		Light[FieldX][FieldY].G = Color.G * Brightness / 255;
		Light[FieldX][FieldY].B = Color.B * Brightness / 255;
	}

	int NumberOfNames;
	std::array<TCreatureName, CREATURES> Names;
	int NumberOfTextualEffects;
	std::array<TTextualEffectPosition, EFFECTS> TextualEffects;
	std::array<std::array<TRGBColor, MAPSIZE_Y>, MAPSIZE_X> Light; // the light on each field
	int PositionX;                                                 // GetMapPosition
	int PositionY;
	int PositionZ;
	int MaxLevel;                                                 // the highest level drawn
	std::array<std::array<int, MAPSIZE_Y>, MAPSIZE_X> CoverLevel; // the highest level with a full ground, 0 none
	int ScrollX;                                                  // the player's walk offset + 32
	int ScrollY;
	std::array<std::array<TCacheField, 12>, 16> Cache;
	TComponent *HintButton; // "Receive hint"

protected:
	// The two hotkeys, then the children.
	void OnClose() override
	{
		try {
			GUI().RemoveHotkey(KEY_CONTROL | 'N');
			GUI().RemoveHotkey(KEY_CONTROL | 'W');
		} catch (...) {
			CloseWhileUnwinding([this] { TWindow::OnClose(); });
			throw;
		}
		TWindow::OnClose();
	}
};

class TMapFrame : public TWindow {
public:
	TMapFrame();
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override;
	int GetMinimumHeight() override;
	void Arrange() override;

	TMapWindow *MapWindow;
	int ViewY; // position of the map view
	int ViewX;
	int ViewWidth;
	int ViewHeight;
};

// MapWindow.cpp: the map view (TMapWindow) and its frame (TMapFrame); see
// MapWindow.h.
//
// The drawing works on two surfaces: 2 is the map surface (480x352, the
// visible 15x11 fields of the 18x14 map), 4 the cache of 16x12 field tiles.
// Draw composes the levels bottom up; DrawField draws one field's objects,
// creatures and effects through DrawObject/DrawCreature/DrawFieldSprite,
// which put static sprites into the field's cache tile (CacheSprite) and draw
// animated ones directly onto the map surface after the tiles below them were
// blitted (FlushField). Light sources add up in Light[][], which is drawn
// last as shaded rectangles or as darkness tiles of picture 7.

#include "MapWindow.h"
#include "Communication.h"
#include "Creatures.h"
#include "DialogElements.h"
#include "Help.h"
#include "Inventory.h"
#include "Map.h"
#include "Objects.h"
#include "Preferences.h"
#include "Skins.h"
#include "Status.h"
#include "Surface.h"
#include "Talk.h"
#include "Utils.h"
#include "graphics.h"
#include "GUI.h"
#include "common.h"
#include "types/FixedString.h"

#include <cstdint>
#include <array>
#include <cmath>
#include <sstream>
#include <cstddef>
#include <cstring>

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextReceiveHint = "Receive hint";                           // 51
const char *TextSelectPlayerToTrade = "Select a player to trade with."; // 125

} // namespace

// cppcoreguidelines-owning-memory is off for the file, for the original's raw owners: the window tree owns its
// children, the message queue its blocks, the lists their entries.
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

namespace {

// Limits Value to the range between Bound1 and Bound2, which may come in
// either order.
inline void Limit(int &Value, int Bound1, int Bound2)
{
	if (Bound1 > Bound2) {
		if (Value < Bound2)
			Value = Bound2;
		if (Value > Bound1)
			Value = Bound1;
	} else {
		if (Value < Bound1)
			Value = Bound1;
		if (Value > Bound2)
			Value = Bound2;
	}
}

} // namespace

// A thrown error whose expression text is a bug number, like Map.cpp's
// assertions ("unknown creature party flag", "[bug0001106]").
#define TERROR1B(Message, V1, Bug) throw TMYEXCEPTION(Message, #V1, V1, "", 0, Bug)

// --- TMapWindow ------------------------------------------------------------

// The hotkey 0x44e (Ctrl+N) cycles the creature info, the hotkey 0x457
// (Ctrl+W) removes the oldest talkbox.
TMapWindow::TMapWindow()
{
	HintButton = nullptr;
	MaxLevel = 0;
	AddHotkey(KEY_CONTROL | 'N', TCallback(this, EVENT_CREATURE_INFO));
	AddHotkey(KEY_CONTROL | 'W', TCallback(this, EVENT_REMOVE_TALKBOX));
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 12; j++) {
			Cache[i][j].x = i << 5;
			Cache[i][j].y = j << 5;
			Cache[i][j].CachedSprites = 0;
		}
	}
}

void TMapWindow::SetSize(int Width, int Height)
{
	Talk().SetTalkChanged();
	TWindow::SetSize(Width, Height);
	Arrange();
}

void TMapWindow::Arrange()
{
	if (HintButton != nullptr)
		SetChildArea(HintButton, SkinProperties().MapHintButton);
}

// Starts dragging the top movable object of the field. Always reports the
// click as handled.
bool TMapWindow::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags)) {
		int FieldX, FieldY, FieldZ;
		GetField(x, y, &FieldX, &FieldY, &FieldZ);
		int Position;
		const TObject Obj = Map().GetTopMoveObject(FieldX, FieldY, FieldZ, Position);
		if (Obj.Type != 0) {
			int ax = FieldX, ay = FieldY, az = FieldZ;
			Map().MapToWorld(&ax, &ay, &az);
			Interaction().SetDragObject(Obj, Position, ax, ay, az);
		}
	}
	return true;
}

// Action state 7: use the object chosen before with the top multi-use object of the field (or
// the creature on it); 8: offer the trade object to the player under the mouse; 6: drop the
// dragged object here (a cumulative pile asks for the count unless Ctrl (0x400) moves it whole
// or 0x200, Shift or both buttons released, moves one); otherwise a click without both buttons
// walks to the field, Flags != 0 is handled as a right-button release.
bool TMapWindow::LeftButtonUp(int x, int y, int Flags)
{
	if (TWindow::LeftButtonUp(x, y, Flags))
		return true;
	int FieldX, FieldY, FieldZ;
	GetField(x, y, &FieldX, &FieldY, &FieldZ);
	if (Interaction().GetActionState() == ACTIONSTATE_USE_WITH) {
		int Position;
		const TObject Obj = Map().GetTopMultiUseObject(FieldX, FieldY, FieldZ, Position);
		if (Obj.Type != 0) {
			int SrcIndex, SrcX, SrcY, SrcZ;
			const TObject &SrcObj = *Interaction().GetUseObject(&SrcIndex, &SrcX, &SrcY, &SrcZ);
			PATELLA_ASSERT(SrcObj!=0);
			PATELLA_ASSERT(SrcObj.Type!=CREATURE);
			if (Obj.Type == CREATURE && !Creatures().IsPlayer(Obj.Data)) {
				SendUseObjectOnCreature(SrcX, SrcY, SrcZ, SrcObj.Type, SrcIndex, Obj.Data);
				return true;
			}
			int DestX = FieldX, DestY = FieldY, DestZ = FieldZ;
			Map().MapToWorld(&DestX, &DestY, &DestZ);
			SendUseObjectWith(SrcX, SrcY, SrcZ, SrcObj.Type, SrcIndex, DestX, DestY, DestZ, Obj.Type,
				Position);
			return true;
		}
	} else if (Interaction().GetActionState() == ACTIONSTATE_TRADE_WITH) {
		int TradeIndex, TradeX, TradeY, TradeZ;
		const TObject &TradeObject = *Interaction().GetTradeObject(&TradeIndex, &TradeX, &TradeY, &TradeZ);
		PATELLA_ASSERT(TradeObject!=0);
		const uint32 CreatureID = GetCreatureAt(x, y);
		if (CreatureID != 0 && CreatureID != PlayerState().GetPlayerID() && Creatures().IsPlayer(CreatureID)) {
			SendRequestTrade(TradeX, TradeY, TradeZ, TradeObject.Type, TradeIndex, CreatureID);
			return true;
		}
		SetStatusMessage(TextSelectPlayerToTrade, false);
		return true;
	} else if (Interaction().GetActionState() == ACTIONSTATE_DRAGGING) {
		int DragIndex, DragX, DragY, DragZ;
		const TObject *DragObject = Interaction().GetDragObject(&DragIndex, &DragX, &DragY, &DragZ);
		const int Type = DragObject->Type;
		const int Count = DragObject->Data;
		int DestX = FieldX, DestY = FieldY, DestZ = FieldZ;
		Map().MapToWorld(&DestX, &DestY, &DestZ);
		if (DragX != DestX || DragY != DestY || DragZ != DestZ) {
			if (ObjectTypes().GetObjectFlag(Type, CUMULATIVE) && Count > 1 && (Flags & SHIFT_OR_BOTH_BUTTONS) == 0) {
				// Patella-Plus, the option Drag Full Stacks: with it a pile moves whole without Ctrl and asks for the
				// count with it, as on a slot (AsksForCount in ContentWindows.cpp).
				if (((Flags & KEY_CONTROL) != 0) != GetDragFullStacks()) {
					SendMoveObject(DragX, DragY, DragZ, Type, DragIndex, DestX, DestY, DestZ, Count);
					return true;
				}
				Interaction().SetDragTarget(DestX, DestY, DestZ);
				AddMessage(MESSAGE_MOVE_OBJECTS_DIALOG, 0, 0);
				return true;
			}
			SendMoveObject(DragX, DragY, DragZ, Type, DragIndex, DestX, DestY, DestZ, 1);
			return true;
		}
	} else {
		if (Flags != 0)
			return RightButtonUp(x, y, Flags);
		int DestX = FieldX, DestY = FieldY, DestZ = Map().GetMapLevel();
		Map().MapToWorld(&DestX, &DestY, &DestZ);
		switch (PlayerState().SetPlayerDestination(DestX, DestY, DestZ, true, false)) {
		case 1:
			SendWalk();
			break;
		case 2:
			SendStopWalk();
			return true;
		default:
			break;
		}
	}
	return true;
}

// A field the player cannot see gives "You can't see anything." (fields next to
// him always count as visible). Otherwise the look and use objects of the field
// and the creature under the mouse become the trade object, use object and trade
// info (Status.cpp). With the classic control, Shift or both buttons (0x200)
// looks, Ctrl (0x400) on an object opens the context menu (message 0x34), and a
// plain click attacks the creature (unless the use object has FORCEUSE) or uses
// the object (message 0x2c for a multi-use one). Otherwise 0x800 attacks, 0x200
// looks, 0x400 uses and a plain click opens the context menu.
bool TMapWindow::RightButtonUp(int x, int y, int Flags)
{
	int FieldX, FieldY, FieldZ;
	GetField(x, y, &FieldX, &FieldY, &FieldZ);
	if ((!In(FieldX - 8, -1, 1) || !In(FieldY - 6, -1, 1)) && Light[FieldX][FieldY].R == 0) {
		Talk().AddMessageTalkbox(TALK_INFO_MESSAGE, "You can't see anything.");
		return true;
	}
	Interaction().SetTradeObject(TObject(), 0, 0, 0, 0);
	Interaction().SetUseObject(TObject(), 0, 0, 0, 0);
	Interaction().SetTradeInfo(0);
	int Position;
	TObject Obj = Map().GetTopLookObject(FieldX, FieldY, FieldZ, Position);
	if (Obj.Type != 0) {
		int ax = FieldX, ay = FieldY, az = FieldZ;
		Map().MapToWorld(&ax, &ay, &az);
		Interaction().SetTradeObject(Obj, Position, ax, ay, az);
	}
	Obj = Map().GetTopUseObject(FieldX, FieldY, FieldZ, Position);
	if (Obj.Type != 0) {
		int ax = FieldX, ay = FieldY, az = FieldZ;
		Map().MapToWorld(&ax, &ay, &az);
		Interaction().SetUseObject(Obj, Position, ax, ay, az);
	}
	const uint32 CreatureID = GetCreatureAt(x, y);
	if (CreatureID != 0)
		Interaction().SetTradeInfo(CreatureID);
	int Index, ox, oy, oz;
	if (GetClassicControl()) {
		if (Flags & SHIFT_OR_BOTH_BUTTONS) {
			const TObject *LookObj = Interaction().GetTradeObject(&Index, &ox, &oy, &oz);
			if (LookObj->Type != 0)
				SendLook(ox, oy, oz, LookObj->Type, Index);
			return true;
		}
		if ((Flags & KEY_CONTROL) && Interaction().GetTradeObject(&Index, &ox, &oy, &oz)->Type != 0) {
			AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
			return true;
		}
		const uint32 Target = Interaction().GetTradeInfo();
		TObject &UseObj = *Interaction().GetUseObject(&Index, &ox, &oy, &oz);
		if (Target != 0 && Target != PlayerState().GetPlayerID() && UseObj.Type != 0
			&& !ObjectTypes().GetObjectFlag(UseObj, FORCEUSE)) {
			Creatures().SetAttackID(Target != Creatures().GetAttackID() ? Target : 0);
			SendAttack(Creatures().GetAttackID());
			return true;
		}
		if (UseObj.Type == 0)
			return true;
		PATELLA_ASSERT(UseObj!=CREATURE);
		if (ObjectTypes().GetObjectFlag(UseObj, MULTIUSE)) {
			AddMessage(MESSAGE_USE_WITH, 0, 0);
			return true;
		}
		SendUseObject(ox, oy, oz, UseObj.Type, Index, Inventory().GetFreeContainer());
		return true;
	} else {
		if (Flags & KEY_ALT) {
			const uint32 Target = Interaction().GetTradeInfo();
			if (Target == 0 || Target == PlayerState().GetPlayerID())
				return true;
			Creatures().SetAttackID(Target != Creatures().GetAttackID() ? Target : 0);
			SendAttack(Creatures().GetAttackID());
			return true;
		}
		if (Flags & SHIFT_OR_BOTH_BUTTONS) {
			const TObject *LookObj = Interaction().GetTradeObject(&Index, &ox, &oy, &oz);
			if (LookObj->Type != 0)
				SendLook(ox, oy, oz, LookObj->Type, Index);
			return true;
		}
		if (Flags & KEY_CONTROL) {
			TObject &UseObj = *Interaction().GetUseObject(&Index, &ox, &oy, &oz);
			if (UseObj.Type == 0)
				return true;
			PATELLA_ASSERT(UseObj!=CREATURE);
			if (ObjectTypes().GetObjectFlag(UseObj, MULTIUSE)) {
				AddMessage(MESSAGE_USE_WITH, 0, 0);
				return true;
			}
			SendUseObject(ox, oy, oz, UseObj.Type, Index, Inventory().GetFreeContainer());
			return true;
		}
		if (Interaction().GetTradeObject(&Index, &ox, &oy, &oz)->Type != 0)
			AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
		return true;
	}
}

// Events 600 and 601 are the hotkeys of the constructor, 27 the hint button: it shows the next event help as a message.
// GCC's analyzer: the message queue owns the block it gets as an integer (AddMessage; the message's handler deletes
// it).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-malloc-leak\"")
void TMapWindow::HandleEvent(int Event)
{
	if (Event == EVENT_CREATURE_INFO)
		SetCreatureInfo((GetCreatureInfo() + 1) % 3);
	if (Event == EVENT_REMOVE_TALKBOX)
		Talk().RemoveOldestTalkbox();
	if (Event == EVENT_MAP_HINT) {
		DeleteChild(HintButton);
		HintButton = nullptr;
		const int Hint = Help().PopEventHelp();
		PATELLA_ASSERT_VALUES(In(Hint,1,EVENTHELPS), Hint);
		const std::size_t Size = strlen(Help().GetEventHelpText(Hint)) + 1;
		char *Text = new char[Size];
		TStringBuffer(Text, Size).Assign(Help().GetEventHelpText(Hint));
		AddMessage(MESSAGE_FORMATTED_DIALOG, reinterpret_cast<std::intptr_t>(Help().GetEventHelpTitle(Hint)), reinterpret_cast<std::intptr_t>(Text));
	}
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")

// Help mode: the help bubble for the creature or object under the mouse
// (1001 the player, 1000 another creature, the object's lens help, 1002 a
// container), placed over the field.
bool TMapWindow::HelpMouseMove(int x, int y)
{
	int FieldX, FieldY, FieldZ;
	GetField(x, y, &FieldX, &FieldY, &FieldZ);
	if ((!In(FieldX - 8, -1, 1) || !In(FieldY - 6, -1, 1)) && Light[FieldX][FieldY].R == 0)
		return false;
	int Position = 0;
	TObject Obj = Map().GetTopLookObject(FieldX, FieldY, FieldZ, Position);
	int Handle = HELP_NONE;
	if (Obj.Type == CREATURE && Obj.Data == (int)PlayerState().GetPlayerID())
		Handle = HELP_YOUR_CHARACTER;
	else if (Obj.Type == CREATURE)
		Handle = HELP_CREATURES;
	else if (ObjectTypes().GetObjectFlag(Obj, LENSHELP))
		Handle = ObjectTypes().GetObjectLensHelp(Obj.Type);
	else if (ObjectTypes().GetObjectFlag(Obj, CONTAINER))
		Handle = HELP_CONTAINERS;
	if (Handle == HELP_NONE)
		return false;
	const TCreature *Player = Creatures().GetCreature(PlayerState().GetPlayerID());
	PATELLA_ASSERT(Player!=nullptr);
	if (GetNoStretching()) {
		x = FieldX * 32 - (480 - Width) / 2 - Player->GetDeltaX() - 32;
		y = FieldY * 32 - (352 - Height) / 2 - Player->GetDeltaY() - 32;
		Help().MoveHelpPosition(x, y);
		Help().SetHelpSize(32, 32);
	} else {
		x = (FieldX * 32 - Player->GetDeltaX() - 32) * Width / 480;
		y = (FieldY * 32 - Player->GetDeltaY() - 32) * Height / 352;
		Help().MoveHelpPosition(x, y);
		Help().SetHelpSize(Width * 32 / 480, Height * 32 / 352);
	}
	Help().SetHelpHandle(Handle);
	return true;
}

// MSVC's analysis (C6262): the buffers on the stack.
PATELLA_MSVC_WARNING(suppress : 6262)
void TMapWindow::Draw(int Surface, int x, int y)
{
	const bool LightEffects = GetLightEffects() && GraphicsEngine().GetSurface()->CanShadeRectangles();
	const bool ShowTexts = GetTextualEffects();
	const int CreatureInfo = GetCreatureInfo();
	Map().ProcessMissileEffects();
	const TCreature *Player = Creatures().GetCreature(PlayerState().GetPlayerID());
	PATELLA_ASSERT(Player!=nullptr);
	ScrollX = Player->GetDeltaX() + 32;
	ScrollY = Player->GetDeltaY() + 32;
	const int Level = Map().GetMapLevel();
	MaxLevel = 7;

	// The levels drawn end below the first ground above the player's cross
	// of fields (and the diagonal lines from it up the levels), unless it is
	// DONTHIDE.
	{
		TObject Obj;
		for (int FieldX = 7; FieldX < 10; FieldX++) {
			for (int FieldY = 5; FieldY < 8; FieldY++) {
				if ((FieldX == 8 || FieldY == 6) && Map().IsFieldThrowable(FieldX, FieldY, Level)) {
					for (int z = Level + 1; z < 8 && FieldX - z + Level >= 0 && FieldY - z + Level >= 0; z++) {
						if (z - 1 < MaxLevel) {
							Obj = Map().GetFirstObject(FieldX - z + Level, FieldY - z + Level, z);
							if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BANK)
								&& !ObjectTypes().GetObjectFlag(Obj, DONTHIDE))
								MaxLevel = z - 1;
							Obj = Map().GetFirstObject(FieldX, FieldY, z);
							if (Obj.Type != 0 && z - 1 < MaxLevel
								&& (ObjectTypes().GetObjectFlag(Obj, BANK) || ObjectTypes().GetObjectFlag(Obj, BOTTOM))
								&& !ObjectTypes().GetObjectFlag(Obj, DONTHIDE))
								MaxLevel = z - 1;
						}
					}
				}
			}
		}
	}

	{
		TObject Obj;
		for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
			for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++) {
				CoverLevel[FieldX][FieldY] = 0;
				for (int z = MaxLevel; z > 0; z--) {
					Obj = Map().GetFirstObject(FieldX, FieldY, z);
					if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, FULLBANK)) {
						CoverLevel[FieldX][FieldY] = z;
						break;
					}
				}
			}
		}
	}

	// The levels bottom up. Each ground sets the light of its field to the
	// ambient light (none below the surface); a translucent one keeps 4/5 of
	// the light below it. The light below the player's level fades by 1/5.
	{
		TObject Obj;
		NumberOfNames = 0;
		NumberOfTextualEffects = 0;
		Map().GetMapPosition(&PositionX, &PositionY, &PositionZ);
		if (GraphicsEngine().GetSurface()->SurfaceRecreated(2)) {
			for (int i = 0; i < 16; i++) {
				for (int j = 0; j < 12; j++)
					Cache[i][j].CachedSprites = 0;
			}
		}
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 12; j++) {
				Cache[i][j].Sprites = 0;
				Cache[i][j].Copied = false;
			}
		}
		for (int z = 0; z <= MaxLevel; z++) {
			if (z != 0 && z == Level) {
				for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
					for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++) {
						TRGBColor &Color = Light[FieldX][FieldY];
						Color.R = Color.R * 4 / 5;
						Color.G = Color.G * 4 / 5;
						Color.B = Color.B * 4 / 5;
					}
				}
			}
			for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
				for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++) {
					Obj = Map().GetFirstObject(FieldX, FieldY, z);
					if (z == 0 || (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BANK))) {
						const TRGBColor Old = Light[FieldX][FieldY];
						SetLightColor(FieldX, FieldY, PositionZ <= 7 ? Map().GetAmbientBrightness() : 0);
						if (z > 0 && Map().IsFieldTranslucent(FieldX, FieldY, z)) {
							const int B = Old.B * 4 / 5;
							const int G = Old.G * 4 / 5;
							const int R = Old.R * 4 / 5;
							TRGBColor &Color = Light[FieldX][FieldY];
							Color.R = Color.R > R ? Color.R : R;
							Color.G = Color.G > G ? Color.G : G;
							Color.B = Color.B > B ? Color.B : B;
						}
					}
					if (z == PositionZ + Level - 8 && z < 7 && FieldX > 0 && FieldY > 0
						&& Map().IsFieldTranslucent(FieldX - 1, FieldY - 1, z + 1))
						SetLightColor(FieldX, FieldY, Map().GetAmbientBrightness() - 1);
				}
			}
			for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
				for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++)
					DrawField(FieldX * 32 - ScrollX, FieldY * 32 - ScrollY, FieldX, FieldY, z, 0);
			}
		}
		for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
			for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++)
				FlushField(FieldX, FieldY);
		}
	}

	// The light: rectangles shaded between the field centres, or one
	// darkness tile per field.
	if (LightEffects) {
		std::array<TRect, (MAPSIZE_X + 1) * (MAPSIZE_Y + 1)> Rects;
		std::array<TRGBColor, (MAPSIZE_X + 1) * (MAPSIZE_Y + 1) * 4> Colors;
		for (int FieldY = 0; FieldY <= MAPSIZE_Y; FieldY++) {
			for (int FieldX = 0; FieldX <= MAPSIZE_X; FieldX++) {
				const int i = FieldY * (MAPSIZE_X + 1) + FieldX;
				Rects[i] = TRect(FieldX * 32 - ScrollX - 16, FieldY * 32 - ScrollY - 16, 32, 32);
				int Left = FieldX - 1, Right = FieldX, Top = FieldY - 1, Bottom = FieldY;
				Limit(Left, 0, MAPSIZE_X - 1);
				Limit(Top, 0, MAPSIZE_Y - 1);
				Colors[i * 4] = Light[Left][Top];
				Limit(Right, 0, MAPSIZE_X - 1);
				Colors[i * 4 + 1] = Light[Right][Top];
				Limit(Bottom, 0, MAPSIZE_Y - 1);
				Colors[i * 4 + 2] = Light[Left][Bottom];
				Colors[i * 4 + 3] = Light[Right][Bottom];
			}
		}
		GraphicsEngine().GetSurface()->ShadeRectangles(2, (MAPSIZE_X + 1) * (MAPSIZE_Y + 1), Rects.data(),
			Colors.data());
	} else {
		for (int FieldX = 0; FieldX < MAPSIZE_X; FieldX++) {
			for (int FieldY = 0; FieldY < MAPSIZE_Y; FieldY++)
				DrawLightMask(FieldX * 32 - ScrollX, FieldY * 32 - ScrollY, Light[FieldX][FieldY].R);
		}
	}

	// The map surface onto the screen, then the names and health bars. A
	// creature hidden by a ground above it gets a grey bar; the colour fades
	// with the light on its field (the player's own light counts with at
	// least 2). The field index is not checked for creatures outside the map
	// (Light[][] is read out of range, as in the binary).
	const int ScreenWidth = Width;
	const int ScreenHeight = Height;
	int BorderX = 0, BorderY = 0;
	if (GetNoStretching()) {
		PATELLA_ASSERT_VALUES(GetWidth()<=MAP_WIDTH*32, GetWidth());
		PATELLA_ASSERT_VALUES(GetHeight()<=MAP_HEIGHT*32, GetHeight());
		if (Width >= 480 && Height >= 352) {
			GraphicsEngine().GetSurface()->BlitRect(Surface, x, y, 2, TRect(0, 0, 480, 352));
		} else {
			BorderX = (480 - Width) / 2;
			BorderY = (352 - Height) / 2;
			GraphicsEngine().GetSurface()->BlitRect(Surface, x, y, 2, TRect(BorderX, BorderY, Width, Height));
		}
	} else {
		GraphicsEngine().GetSurface()->StretchBlitRect(Surface, TRect(x, y, ScreenWidth, ScreenHeight), 2,
			TRect(0, 0, 480, 352));
	}
	if (CreatureInfo != 0) {
		for (int i = 0; i < NumberOfNames; i++) {
			TCreature *Creature = Creatures().GetCreature(Names[i].CreatureID);
			if (Creature == nullptr)
				continue;
			int cx = Creature->RealX, cy = Creature->RealY, cz = Creature->RealZ;
			Map().WorldToMap(&cx, &cy, &cz);
			bool Visible = false;
			if (In(cx, 0, MAPSIZE_X - 1) && In(cy, 0, MAPSIZE_Y - 1)) {
				Visible = true;
				while (++cz <= MaxLevel) {
					TObject Obj;
					Obj = Map().GetFirstObject(cx, cy, cz);
					if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BANK))
						Visible = false;
				}
			}
			const TRGBColor Color = Visible ? Creature->GetHealthColor() : TRGBColor(192, 192, 192);
			int Brightness = Light[cx][cy].R;
			if (Creature->CreatureID == PlayerState().GetPlayerID()) {
				int Strength = Creature->LightStrength;
				Limit(Strength, 2, 5);
				if (Brightness <= Strength * 51)
					Brightness = Strength * 51;
			}
			Brightness = Brightness / 5;
			const TRGBColor NameColor(Color.R * Brightness / 51, Color.G * Brightness / 51, Color.B * Brightness / 51);
			int nx = Names[i].x, ny = Names[i].y;
			if (GetNoStretching()) {
				nx -= BorderX;
				ny -= BorderY;
			} else {
				nx = Width * nx / 480;
				ny = Height * ny / 352;
			}
			int BarY = ny - 4;
			if (CreatureInfo == 2) {
				int BarX = nx - 14;
				const int TextWidth = Graphics().GetTextWidth(FONT_BORDERED, Creature->Name);
				Limit(BarX, 1, Width - 28);
				Limit(BarY, Graphics().GetFontHeight(FONT_BORDERED) + 1,
					Height - Graphics().GetFontHeight(FONT_BORDERED) - 5);
				int TextX = BarX - TextWidth / 2 + 14;
				const int TextY = BarY - Graphics().GetFontHeight(FONT_BORDERED);
				Limit(TextX, 1, Width - TextWidth - 1);
				GraphicsEngine().GetSurface()->FillRectangle(Surface, TRect(BarX + x, BarY + y, 27, 4),
					TRGBColor(0, 0, 0));
				GraphicsEngine().GetSurface()->FillRectangle(Surface,
					TRect(BarX + x + 1, BarY + y + 1, Creature->Health / 4, 2),
					NameColor);
				Graphics().PrintText(Surface, TextX + x, TextY + y, FONT_BORDERED, NameColor, Creature->Name, 0);
			} else {
				int BarX = nx - 14;
				Limit(BarX, 1, Width - 28);
				Limit(BarY, 1, Height - 5);
				GraphicsEngine().GetSurface()->FillRectangle(Surface, TRect(BarX + x, BarY + y, 27, 4),
					TRGBColor(0, 0, 0));
				GraphicsEngine().GetSurface()->FillRectangle(Surface,
					TRect(BarX + x + 1, BarY + y + 1, Creature->Health / 4, 2),
					NameColor);
			}
			// The party shield, then the skull (both blink when the skull is
			// about to go); a shield of type 1 shows only while the player is
			// in no party.
			int Icon1 = SKIN_NONE, Icon2 = SKIN_NONE;
			bool Blink = true;
			if (Creature->PARTYFlag > PARTY_NONE) {
				switch (Creature->PARTYFlag) {
				case PARTY_INVITER:
					if (!PlayerState().IsPlayerInParty())
						Icon1 = SKIN_MAP_SHIELD_1;
					break;
				case PARTY_INVITEE:
					Icon1 = SKIN_MAP_SHIELD_2;
					break;
				case PARTY_MEMBER:
					Icon1 = SKIN_MAP_SHIELD_3;
					break;
				case PARTY_LEADER:
					Icon1 = SKIN_MAP_SHIELD_4;
					break;
				default:
					TERROR1B("unknown creature party flag", Creature->PARTYFlag, "[bug0001106]");
				}
			}
			if (Creature->PKFlag > PK_NONE) {
				int Skull;
				switch (Creature->PKFlag) {
				case PK_YELLOW:
					Skull = SKIN_MAP_SKULL_1;
					break;
				case PK_GREEN:
					Skull = SKIN_MAP_SKULL_2;
					break;
				case PK_WHITE:
					Skull = SKIN_MAP_SKULL_3;
					break;
				case PK_RED:
					Skull = SKIN_MAP_SKULL_4;
					break;
				default:
					TERROR1("unknown creature PK flag", Creature->PKFlag);
				}
				Icon2 = Skull;
				if (Icon1 == SKIN_NONE) {
					Icon1 = Skull;
					Icon2 = SKIN_NONE;
				}
				if (Creature->SkullTime != 0) // a kept skull
					Blink = Interaction().GetBlinkState();
			}
			if (Icon1 != SKIN_NONE) {
				const TSkin *Skin = &Skins()[Icon1];
				int IconX = nx - Skin->width + 20;
				int IconY = ny + 1;
				if (Icon2 != SKIN_NONE) {
					Limit(IconX, 1, Width - Skin->width - 14);
					Limit(IconY, 1, Height - Skin->height - 1);
					DrawSkin(Surface, IconX + x, IconY + y, Skin->width, Skin->height, Icon1, 0, 0);
					if (Blink)
						DrawSkin(Surface, IconX + x + 13, IconY + y, Skins()[Icon2].width, Skins()[Icon2].height,
							Icon2, 0, 0);
				} else {
					Limit(IconX, 1, Width - Skin->width - 1);
					Limit(IconY, 1, Height - Skin->height - 1);
					if (Blink)
						DrawSkin(Surface, IconX + x, IconY + y, Skin->width, Skin->height, Icon1, 0, 0);
				}
			}
		}
	}

	// The talkboxes (their positions follow the walk unless they stick to
	// an edge) and the status message.
	if (Talk().TakeTalkChanged())
		ArrangeTalkboxes();
	const int Talkboxes = Talk().GetNumberOfTalkboxes();
	for (int Talkbox = 1; Talkbox <= Talkboxes; Talkbox++) {
		int tx, ty, Alignment, Fixed;
		Talk().GetTalkboxRectangle(Talkbox, &tx, &ty, &Alignment, &Fixed);
		if ((Fixed & 1) == 0)
			tx -= GetNoStretching() ? Player->GetDeltaX() : Player->GetDeltaX() * Width / 480;
		if ((Fixed & 2) == 0)
			ty -= GetNoStretching() ? Player->GetDeltaY() : Player->GetDeltaY() * Height / 352;
		const int Lines = Talk().GetTalkboxLines(Talkbox);
		TRGBColor Color;
		Talk().GetTalkboxColor(&Color, Talkbox);
		for (int Line = 0; Line < Lines; Line++)
			Graphics().PrintTextClipped(Surface, tx + x, Line * Graphics().GetFontLineHeight(FONT_BORDERED) + ty + y,
				FONT_BORDERED, Color, Talk().GetTalkboxLine(Talkbox, Line), Alignment,
				TRect(x, y, ScreenWidth, ScreenHeight));
	}
	if (Talk().GetStatusMessage() != nullptr) {
		const int MessageWidth = Width, MessageHeight = Height;
		const int Length = Graphics().GetFittingTextLength(FONT_BORDERED, Talk().GetStatusMessage(), MessageWidth);
		Graphics().PrintTextPart(Surface, MessageWidth / 2 + x,
			MessageHeight - Graphics().GetFontHeight(FONT_BORDERED) + y, FONT_BORDERED,
			TRGBColor(255, 255, 255), Talk().GetStatusMessage(), Length, 1);
	}

	// The animated texts of visible fields and of the player's level.
	{
		TObject Obj;
		if (ShowTexts) {
			for (int i = 0; i < NumberOfTextualEffects; i++) {
				const int FieldX = TextualEffects[i].FieldX, FieldY = TextualEffects[i].FieldY;
				int z = TextualEffects[i].FieldZ;
				bool Visible = false;
				if (In(FieldX, 0, MAPSIZE_X - 1) && In(FieldY, 0, MAPSIZE_Y - 1)) {
					Visible = true;
					while (++z <= MaxLevel) {
						Obj = Map().GetFirstObject(FieldX, FieldY, z);
						if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BANK))
							Visible = false;
					}
				}
				if (!Visible && TextualEffects[i].FieldZ != Map().GetMapLevel())
					continue;
				const int Color = Map().GetTextualEffectColor(EffectIdOf(TextualEffects[i].EffectNumber));
				const char *Text = Map().GetTextualEffectText(EffectIdOf(TextualEffects[i].EffectNumber));
				int tx = TextualEffects[i].x, ty = TextualEffects[i].y;
				if (GetNoStretching()) {
					tx -= BorderX;
					ty -= BorderY;
				} else {
					tx = tx * Width / 480;
					ty = ty * Height / 352;
				}
				Limit(tx, Graphics().GetTextWidth(FONT_BORDERED, Text) / 2,
					Width - Graphics().GetTextWidth(FONT_BORDERED, Text) / 2);
				Limit(ty, 0, Height - Graphics().GetFontHeight(FONT_BORDERED));
				Graphics().PrintText(Surface, tx + x, ty + y, FONT_BORDERED, TRGBColor(Color), Text, 1);
			}
		}
	}

	// The hint button while there is an event help to show.
	if (GetShowNewHints() && Help().GetEventHelp() != HINT_NONE && HintButton == nullptr) {
		HintButton = new TBlinkingButton(0, 0, SKIN_HINT_BUTTON_UP1, SKIN_HINT_BUTTON_DOWN1, SKIN_HINT_BUTTON_UP2, SKIN_HINT_BUTTON_DOWN2, TCallback(this, EVENT_MAP_HINT), TextReceiveHint);
		InsertChild(HintButton);
		Arrange();
		if (Help().GetEventHelp() == HINT_FIRST_LOGIN)
			HandleEvent(EVENT_MAP_HINT);
	} else if (!GetShowNewHints() && HintButton != nullptr) {
		DeleteChild(HintButton);
		HintButton = nullptr;
	}
	TWindow::Draw(Surface, x, y);
}

// One field at map surface position posx/posy. Mode 0 draws everything; Mode
// 1 (the fields left of and above a lying object, drawn again over it) skips
// the objects below the top ones, and both lights and animated texts are
// only collected in Mode 0. Objects are raised by the HEIGHT of the objects
// below them (at most 24 pixels). Creatures are drawn from the 3x3 fields
// around, those whose figure ends on this field, at most 10, sorted from the
// back; they get the highlight, attack and follow frames and their mark.
void TMapWindow::DrawField(int posx, int posy, int FieldX, int FieldY, int FieldZ, int Mode)
{
	int x = FieldX, y = FieldY, z = FieldZ;
	Map().MapToWorld(&x, &y, &z);
	TObject Obj = Map().GetFirstObject(FieldX, FieldY, FieldZ);
	if (Obj.Type == 0 && FieldZ == 0 && Mode == 0 && IsFieldVisible(FieldX, FieldY, 0))
		DrawFieldSprite(posx, posy, 0, false, 0, 0, nullptr);
	bool Lying = false;
	int Elevation = 0;
	TObject TopObject;
	for (Obj = Map().GetFirstObject(FieldX, FieldY, FieldZ); Obj.Type != 0; Obj = Map().GetNextObject()) {
		if (Mode == 0 && Obj.Type != CREATURE && ObjectTypes().GetObjectFlag(Obj, LIGHT)) {
			int ShiftX = 0, ShiftY = 0;
			if (ObjectTypes().GetObjectFlag(Obj, SHIFT)) {
				ShiftX = ObjectTypes().GetObjectShiftX(Obj.Type);
				ShiftY = ObjectTypes().GetObjectShiftY(Obj.Type);
			}
			AddLightSource(posx - ShiftX - Elevation + 16, posy - ShiftY - Elevation + 16, FieldZ,
				ObjectTypes().GetObjectBrightness(Obj.Type), TRGBColor(ObjectTypes().GetObjectLightColor(Obj.Type)));
		}
		if (ObjectTypes().GetObjectFlag(Obj, TOP)) {
			TopObject = Obj;
		} else if (Obj.Type != CREATURE && Mode != 1) {
			DrawObject(posx - Elevation, posy - Elevation, FieldZ, Obj, x, y, z);
			if (ObjectTypes().GetObjectFlag(Obj, LYINGOBJECT))
				Lying = true;
		}
		Elevation += ObjectTypes().GetObjectFlag(Obj, HEIGHT) ? ObjectTypes().GetObjectElevation(Obj.Type) : 0;
		if (Elevation > 24)
			Elevation = 24;
	}
	if (Lying) {
		if (FieldX > 0) {
			if (FieldY > 0)
				DrawField(posx - 32, posy - 32, FieldX - 1, FieldY - 1, FieldZ, 1);
			DrawField(posx - 32, posy, FieldX - 1, FieldY, FieldZ, 1);
		}
		if (FieldY > 0)
			DrawField(posx, posy - 32, FieldX, FieldY - 1, FieldZ, 1);
	}
	int NumberOfCreatures = 0;
	std::array<TCreature *, 10> Creatures;
	std::array<int, 10> OffsetX;
	std::array<int, 10> OffsetY;
	std::array<int, 10> ShiftX;
	std::array<int, 10> ShiftY;
	for (int i = FieldX - 1; i <= FieldX + 1; i++) {
		for (int j = FieldY - 1; j <= FieldY + 1; j++) {
			if (i < 0 || j < 0 || i >= MAPSIZE_X || j >= MAPSIZE_Y)
				continue;
			for (Obj = Map().GetFirstObject(i, j, FieldZ); Obj.Type != 0; Obj = Map().GetNextObject()) {
				if (Obj.Type != CREATURE || NumberOfCreatures == 10)
					continue;
				TCreature *Creature = ::Creatures().GetCreature(Obj.Data);
				if (Creature == nullptr)
					continue;
				int sx = 0, sy = 0;
				if (Creature->OutfitType != 0) {
					if (ObjectTypes().GetOutfitFlag(Creature->OutfitType, SHIFT)) {
						sx = ObjectTypes().GetOutfitShiftX(Creature->OutfitType);
						sy = ObjectTypes().GetOutfitShiftY(Creature->OutfitType);
					}
				} else if (Creature->Colors[0] != 0) {
					if (ObjectTypes().GetObjectFlag(Creature->Colors[0], SHIFT)) {
						sx = ObjectTypes().GetObjectShiftX(Creature->Colors[0]);
						sy = ObjectTypes().GetObjectShiftY(Creature->Colors[0]);
					}
				} else if (::Creatures().IsPlayer(Creature->CreatureID)) {
					sx = 8;
					sy = 8;
				} else {
					continue;
				}
				const int dx = (i - FieldX) * 32 - sx + Creature->GetDeltaX();
				const int dy = (j - FieldY) * 32 - sy + Creature->GetDeltaY();
				if (dx >= -31 && dx <= 0 && dy >= -31 && dy <= 0) {
					Creatures[NumberOfCreatures] = Creature;
					OffsetY[NumberOfCreatures] = dy;
					OffsetX[NumberOfCreatures] = dx;
					ShiftX[NumberOfCreatures] = sx;
					ShiftY[NumberOfCreatures] = sy;
					NumberOfCreatures++;
				}
			}
		}
	}
	for (int i = 0; i < NumberOfCreatures - 1; i++) {
		for (int j = i + 1; j < NumberOfCreatures; j++) {
			if (OffsetY[i] > OffsetY[j] || (OffsetY[i] == OffsetY[j] && OffsetX[i] >= OffsetX[j])) {
				TCreature *Creature = Creatures[i];
				Creatures[i] = Creatures[j];
				Creatures[j] = Creature;
				int Swap = OffsetX[i];
				OffsetX[i] = OffsetX[j];
				OffsetX[j] = Swap;
				Swap = OffsetY[i];
				OffsetY[i] = OffsetY[j];
				OffsetY[j] = Swap;
				Swap = ShiftX[i];
				ShiftX[i] = ShiftX[j];
				ShiftX[j] = Swap;
				Swap = ShiftY[i];
				ShiftY[i] = ShiftY[j];
				ShiftY[j] = Swap;
			}
		}
	}
	int i;
	for (i = 0; i < NumberOfCreatures; i++) {
		TRGBColor Color;
		if (Creatures[i]->CreatureID == ::Creatures().GetHighlightID())
			Color = SkinProperties().HighlightColor;
		if (Creatures[i]->CreatureID == ::Creatures().GetAttackID())
			Color = Creatures[i]->CreatureID == ::Creatures().GetHighlightID() ? SkinProperties().HighlightedAttackColor
																			   : SkinProperties().AttackColor;
		if (Creatures[i]->CreatureID == ::Creatures().GetFollowID())
			Color = Creatures[i]->CreatureID == ::Creatures().GetHighlightID() ? SkinProperties().HighlightedFollowColor
																			   : SkinProperties().FollowColor;
		const int ox = OffsetX[i], oy = OffsetY[i];
		if (Color != TRGBColor(0, 0, 0)) {
			const int cx = (ScrollX - Elevation + ox + posx) / 32, cy = (ScrollY - Elevation + oy + posy) / 32;
			FlushField(cx, cy);
			FlushField(cx + 1, cy);
			FlushField(cx, cy + 1);
			FlushField(cx + 1, cy + 1);
			GraphicsEngine().GetSurface()->DrawFrameClipped(2,
				TRect(ox - Elevation + posx, oy - Elevation + posy, 32, 32), 2,
				Color, TRect(0, 0, 480, 352));
		}
		if (Creatures[i]->IsMarked()) {
			const int cx = (ScrollX - Elevation + ox + posx) / 32, cy = (ScrollY - Elevation + oy + posy) / 32;
			FlushField(cx, cy);
			FlushField(cx + 1, cy);
			FlushField(cx, cy + 1);
			FlushField(cx + 1, cy + 1);
			const int Margin = Color == TRGBColor(0, 0, 0) ? 0 : 2;
			GraphicsEngine().GetSurface()->DrawFrameClipped(2,
				TRect(Margin + ox - Elevation + posx, Margin - Elevation + oy + posy,
					(16 - Margin) * 2, (16 - Margin) * 2),
				2, TRGBColor(Creatures[i]->Mark), TRect(0, 0, 480, 352));
		}
		const int OutfitType = Creatures[i]->OutfitType;
		if (OutfitType == 0) {
			if (Creatures[i]->Colors[0] == 0) {
				// An invisible creature: the sparkles of effect 13.
				const int SpriteNumber = ObjectTypes().GetEffectSpriteNumber(13, 1, 1, 1,
					GetActionNumber() / 3 % 3 + 2);
				const int sx = ox + ShiftX[i], sy = oy + ShiftY[i];
				const int cx = (ScrollX - Elevation + sx + posx - 8) / 32, cy = (ScrollY - Elevation + sy + posy - 8) / 32;
				FlushField(cx, cy);
				FlushField(cx + 1, cy);
				FlushField(cx, cy + 1);
				FlushField(cx + 1, cy + 1);
				Graphics().DrawSprite(2, sx - Elevation + posx - 8, sy - Elevation + posy - 8, SpriteNumber);
			} else {
				// A creature shown as an object.
				const TObject Shape = {Creatures[i]->Colors[0], 0, 0};
				DrawObject(ShiftX[i] - Elevation + ox + posx, ShiftY[i] - Elevation + oy + posy, FieldZ, Shape, 0, 0, 0);
			}
		} else {
			DrawCreature(ShiftX[i] - Elevation + ox + posx, ShiftY[i] - Elevation + oy + posy, FieldZ, Creatures[i]);
		}
		if (Mode == 0) {
			if ((::Creatures().IsOpponent(Creatures[i]) || Creatures[i]->CreatureID == PlayerState().GetPlayerID())
				&& FieldZ == Map().GetMapLevel()) {
				Names[NumberOfNames].CreatureID = Creatures[i]->CreatureID;
				Names[NumberOfNames].x = ox - Elevation + posx + 16;
				Names[NumberOfNames].y = oy - Elevation + posy;
				NumberOfNames++;
			}
			const int lx = ox - Elevation + posx + 16, ly = oy - Elevation + posy + 16;
			AddLightSource(lx, ly, FieldZ, Creatures[i]->LightStrength, TRGBColor(Creatures[i]->LightColor));
			if (OutfitType != 0 && ObjectTypes().GetOutfitFlag(OutfitType, LIGHT))
				AddLightSource(lx, ly, FieldZ, ObjectTypes().GetOutfitBrightness(OutfitType), TRGBColor(ObjectTypes().GetOutfitLightColor(OutfitType)));
			if (Creatures[i]->CreatureID == PlayerState().GetPlayerID() && Creatures[i]->LightStrength < 2)
				AddLightSource(lx, ly, FieldZ, 2, TRGBColor(255, 255, 255));
		}
	}
	for (const int EffectNumber : TMapEffects(FieldX, FieldY, FieldZ)) {
		if (EffectNumber < 0 || EffectNumber >= EFFECTS) {
			std::stringstream s;
			s << "EffectNumber: " << EffectNumber << " X: " << FieldX << " Y: " << FieldY << " Z: " << FieldZ;
			const TCreature *Player = ::Creatures().GetCreature(PlayerState().GetPlayerID());
			if (Player != nullptr)
				s << " PlayerX: " << Player->RealX << " PlayerY: " << Player->RealY << " PlayerZ: " << Player->RealZ;
			throw TMYEXCEPTION("effect number out of range [bug0000913]", "", 0, "", 0, s.str());
		}
		if (Map().GetEffectStatus(EffectIdOf(EffectNumber)) == 1) {
			const int Type = Map().GetGraphicalEffectType(EffectIdOf(EffectNumber));
			if (Type != -1) {
				const int Phase = Map().GetGraphicalEffectPhase(EffectIdOf(EffectNumber));
				for (int X = 1; X <= ObjectTypes().GetEffectWidth(Type); X++) {
					for (int Y = 1; Y <= ObjectTypes().GetEffectHeight(Type); Y++) {
						for (int Layer = 1; Layer <= ObjectTypes().GetEffectLayers(Type); Layer++) {
							const int SpriteNumber = ObjectTypes().GetEffectSpriteNumber(Type, X, Y, Layer, Phase);
							if (SpriteNumber > 0)
								DrawFieldSprite(posx - X * 32 - Elevation + 32, posy - Y * 32 - Elevation + 32, FieldZ,
									false, SpriteNumber, 0, nullptr);
						}
					}
				}
				// The light grows and fades with the phases, up to the
				// effect's brightness. The binary passes the level and the
				// brightness the wrong way round (the missile light below
				// has them right): the level becomes the radius.
				if (Mode == 0 && ObjectTypes().GetEffectFlag(Type, LIGHT)) {
					const int Rest = ObjectTypes().GetEffectPhases(Type) - Phase + 1;
					int Brightness = Phase < Rest ? Phase : Rest;
					const int Maximum = ObjectTypes().GetEffectBrightness(Type);
					if (Brightness >= Maximum)
						Brightness = Maximum;
					AddLightSource(posx - Elevation + 16, posy - Elevation + 16, Brightness, FieldZ,
						TRGBColor(ObjectTypes().GetEffectLightColor(Type)));
				}
			}
		}
		if (Map().GetEffectStatus(EffectIdOf(EffectNumber)) == 2) {
			if (Mode != 0)
				continue;
			TTextualEffectPosition &Text = TextualEffects[NumberOfTextualEffects];
			Text.EffectNumber = EffectNumber;
			Text.x = posx - Elevation + 16;
			Text.y = posy - Map().GetTextualEffectPhase(EffectIdOf(EffectNumber)) * 2 - Elevation;
			Text.FieldX = FieldX;
			Text.FieldY = FieldY;
			Text.FieldZ = FieldZ;
			NumberOfTextualEffects++;
		}
		if (Map().GetEffectStatus(EffectIdOf(EffectNumber)) == 3) {
			const int Type = Map().GetMissileEffectType(EffectIdOf(EffectNumber));
			if (Type != -1) {
				for (int X = 1; X <= ObjectTypes().GetMissileWidth(Type); X++) {
					for (int Y = 1; Y <= ObjectTypes().GetMissileHeight(Type); Y++) {
						for (int Layer = 1; Layer <= ObjectTypes().GetMissileLayers(Type); Layer++) {
							const int SpriteNumber = ObjectTypes().GetMissileSpriteNumber(Type, X, Y, Layer,
								Map().GetMissileEffectDirection(EffectIdOf(EffectNumber)));
							if (SpriteNumber > 0)
								DrawFieldSprite(
									Map().GetMissileEffectX(EffectIdOf(EffectNumber)) - X * 32 - Elevation + posx + 32,
									Map().GetMissileEffectY(EffectIdOf(EffectNumber)) - Y * 32 - Elevation + posy + 32,
									FieldZ, true, SpriteNumber, 0, nullptr);
						}
					}
				}
				if (Mode == 0 && ObjectTypes().GetMissileFlag(Type, LIGHT))
					AddLightSource(Map().GetMissileEffectX(EffectIdOf(EffectNumber)) - Elevation + posx + 16,
						Map().GetMissileEffectY(EffectIdOf(EffectNumber)) - Elevation + posy + 16, FieldZ,
						ObjectTypes().GetMissileBrightness(Type), TRGBColor(ObjectTypes().GetMissileLightColor(Type)));
			}
		}
	}
	if (TopObject.Type != 0)
		DrawObject(posx, posy, FieldZ, TopObject, x, y, z);
}

// The pattern comes from the absolute position (x, y, z), the count, the
// liquid or the hook; objects with several phases are animated and bypass the
// field cache. A pattern size or phase count of 0 in Tibia.dat throws a
// TMyException (PATELLA_DIVISOR).
void TMapWindow::DrawObject(int posx, int posy, int Level, TObject Obj, int x, int y, int z)
{
	PATELLA_ASSERT(Obj.Type!=0);
	int PatternX = x % PATELLA_DIVISOR(ObjectTypes().GetObjectPatternWidth(Obj.Type)) + 1;
	int PatternY = y % PATELLA_DIVISOR(ObjectTypes().GetObjectPatternHeight(Obj.Type)) + 1;
	const int PatternZ = z % PATELLA_DIVISOR(ObjectTypes().GetObjectPatternDepth(Obj.Type)) + 1;
	const int Phase = GetActionNumber() / 5 % PATELLA_DIVISOR(ObjectTypes().GetObjectPhases(Obj.Type)) + 1;
	if (ObjectTypes().GetObjectFlag(Obj, CUMULATIVE))
		ObjectTypes().GetCumulativePattern(Obj, &PatternX, &PatternY);
	if (ObjectTypes().GetObjectFlag(Obj, LIQUIDPOOL) || ObjectTypes().GetObjectFlag(Obj, LIQUIDCONTAINER))
		ObjectTypes().GetLiquidPattern(Obj, &PatternX, &PatternY);
	if (ObjectTypes().GetObjectFlag(Obj, HANG))
		ObjectTypes().GetHangPattern(Obj, &PatternX, &PatternY);
	int ShiftX = 0, ShiftY = 0;
	if (ObjectTypes().GetObjectFlag(Obj, SHIFT)) {
		ShiftX = ObjectTypes().GetObjectShiftX(Obj.Type);
		ShiftY = ObjectTypes().GetObjectShiftY(Obj.Type);
	}
	for (int X = 1; X <= ObjectTypes().GetObjectWidth(Obj.Type); X++) {
		for (int Y = 1; Y <= ObjectTypes().GetObjectHeight(Obj.Type); Y++) {
			for (int Layer = 1; Layer <= ObjectTypes().GetObjectLayers(Obj.Type); Layer++) {
				const int SpriteNumber = ObjectTypes().GetObjectSpriteNumber(Obj.Type, X, Y, Layer, PatternX, PatternY, PatternZ, Phase);
				if (SpriteNumber > 0)
					DrawFieldSprite(posx - X * 32 - ShiftX + 32, posy - Y * 32 - ShiftY + 32, Level,
						ObjectTypes().GetObjectPhases(Obj.Type) > 1, SpriteNumber, 0, nullptr);
			}
		}
	}
}

// Outfits are always animated; the second layer is the colour mask (head,
// body, legs, feet). A phase count of 0 in Tibia.dat throws a TMyException
// (PATELLA_DIVISOR).
void TMapWindow::DrawCreature(int posx, int posy, int Level, TCreature *Creature)
{
	PATELLA_ASSERT(Creature!=nullptr);
	PATELLA_ASSERT(Creature->OutfitType!=0);
	const int OutfitType = Creature->OutfitType;
	std::array<TRGBColor, 4> Colors;
	int R, G, B;
	GetOutfitColor(Creature->Colors[0], &R, &G, &B);
	Colors[0] = TRGBColor(R, G, B);
	GetOutfitColor(Creature->Colors[1], &R, &G, &B);
	Colors[1] = TRGBColor(R, G, B);
	GetOutfitColor(Creature->Colors[2], &R, &G, &B);
	Colors[2] = TRGBColor(R, G, B);
	GetOutfitColor(Creature->Colors[3], &R, &G, &B);
	Colors[3] = TRGBColor(R, G, B);
	int ShiftX = 0, ShiftY = 0;
	if (ObjectTypes().GetOutfitFlag(OutfitType, SHIFT)) {
		ShiftX = ObjectTypes().GetOutfitShiftX(OutfitType);
		ShiftY = ObjectTypes().GetOutfitShiftY(OutfitType);
	}
	int Phase;
	if (ObjectTypes().GetOutfitFlag(OutfitType, ANIMATEALWAYS))
		Phase = GetActionNumber() / 3 % PATELLA_DIVISOR(ObjectTypes().GetOutfitPhases(OutfitType)) + 1;
	else
		Phase = Creature->GetAnimationPhase(ObjectTypes().GetOutfitPhases(OutfitType));
	for (int X = 1; X <= ObjectTypes().GetOutfitWidth(OutfitType); X++) {
		for (int Y = 1; Y <= ObjectTypes().GetOutfitHeight(OutfitType); Y++) {
			const int SpriteNumber = ObjectTypes().GetOutfitSpriteNumber(OutfitType, X, Y, 1,
				Creature->GetDrawDirection(), Phase);
			const int MaskNumber = ObjectTypes().GetOutfitLayers(OutfitType) == 2
				? ObjectTypes().GetOutfitSpriteNumber(OutfitType, X, Y, 2, Creature->GetDrawDirection(), Phase)
				: 0;
			if (SpriteNumber > 0)
				DrawFieldSprite(posx - X * 32 - ShiftX + 32, posy - Y * 32 - ShiftY + 32, Level, true, SpriteNumber,
					MaskNumber, Colors.data());
		}
	}
}

// One of the darkness tiles of picture 7 (0 darkest; brightness 1-27
// gets tile 1, from 224 on nothing is drawn).
void TMapWindow::DrawLightMask(int posx, int posy, int Brightness)
{
	PATELLA_ASSERT_VALUES(In(Brightness,0,255), Brightness);
	int Tile;
	if (In(Brightness, 1, 27))
		Tile = 1;
	else
		Tile = Brightness / 28;
	if (Tile <= 7)
		Graphics().DrawPicturePart(2, posx, posy, 7, Tile << 5, 0, 32, 32);
}

// A sprite at map surface position x/y on the fields it covers that Level
// shows on. Static sprites go into the cache tiles; an animated (or masked)
// sprite flushes those tiles and is drawn directly.
void TMapWindow::DrawFieldSprite(int x, int y, int Level, bool Animated, int SpriteNumber, int MaskNumber,
	TRGBColor *Colors)
{
	const int ax = ScrollX + x;
	const int ay = ScrollY + y;
	const int FieldX = (ax + 32) / 32 - 1;
	const int FieldY = (ay + 32) / 32 - 1;
	if (MaskNumber == 0 && !Animated) {
		const int dx = (ax + 32) % 32;
		const int dy = (ay + 32) % 32;
		if (IsFieldVisible(FieldX, FieldY, Level))
			CacheSprite(FieldX, FieldY, SpriteNumber, dx, dy);
		if (ax % 32 != 0 && IsFieldVisible(FieldX + 1, FieldY, Level))
			CacheSprite(FieldX + 1, FieldY, SpriteNumber, dx - 32, dy);
		if (ay % 32 != 0 && IsFieldVisible(FieldX, FieldY + 1, Level))
			CacheSprite(FieldX, FieldY + 1, SpriteNumber, dx, dy - 32);
		if (ax % 32 != 0 && ay % 32 != 0 && IsFieldVisible(FieldX + 1, FieldY + 1, Level))
			CacheSprite(FieldX + 1, FieldY + 1, SpriteNumber, dx - 32, dy - 32);
		return;
	}
	bool Flushed = false;
	if (IsFieldVisible(FieldX, FieldY, Level)) {
		FlushField(FieldX, FieldY);
		Flushed = true;
	}
	if (ax % 32 != 0 && IsFieldVisible(FieldX + 1, FieldY, Level)) {
		FlushField(FieldX + 1, FieldY);
		Flushed = true;
	}
	if (ay % 32 != 0 && IsFieldVisible(FieldX, FieldY + 1, Level)) {
		FlushField(FieldX, FieldY + 1);
		Flushed = true;
	}
	if (ax % 32 != 0 && ay % 32 != 0 && IsFieldVisible(FieldX + 1, FieldY + 1, Level)) {
		FlushField(FieldX + 1, FieldY + 1);
		Flushed = true;
	}
	if (Flushed) {
		if (MaskNumber == 0)
			Graphics().DrawSprite(2, x, y, SpriteNumber);
		else
			Graphics().DrawMaskedSprite(2, x, y, SpriteNumber, MaskNumber, Colors);
	}
}

// Adds a sprite at offset dx/dy to a field's cache tile. A tile that holds
// the same sprites as in the last frame is not drawn again; on the first
// difference its sprites are drawn again from the start (without clearing
// the tile). A full tile (10 sprites) or one already blitted to the map
// surface gets the sprite drawn directly onto the map surface, clipped to
// the field.
void TMapWindow::CacheSprite(int FieldX, int FieldY, int SpriteNumber, int dx, int dy)
{
	if (!In(FieldX, 0, MAPSIZE_X - 1) || !In(FieldY, 0, MAPSIZE_Y - 1))
		return;
	const int x = FieldX * 32 - ScrollX;
	if (x <= -32)
		return;
	const int y = FieldY * 32 - ScrollY;
	if (y <= -32 || x >= 480 || y >= 352)
		return;
	TCacheField *Field = GetCacheField(FieldX, FieldY);
	if (Field->Sprites == 10)
		FlushField(FieldX, FieldY);
	if (Field->Copied) {
		int ClipX = x, ClipY = y, ClipWidth = 32, ClipHeight = 32;
		if (x < 0) {
			ClipWidth = x + 32;
			ClipX = 0;
		}
		if (y < 0) {
			ClipHeight = y + 32;
			ClipY = 0;
		}
		if (ClipWidth + ClipX >= 480)
			ClipWidth = 480 - ClipX;
		if (ClipY + ClipHeight >= 352)
			ClipHeight = 352 - ClipY;
		if (SpriteNumber != 0)
			Graphics().DrawSpriteClipped(2, x + dx, y + dy, SpriteNumber, TRect(ClipX, ClipY, ClipWidth, ClipHeight));
		else
			Graphics().DrawPicturePartClipped(2, x + dx, y + dy, 7, 0, 0, 32, 32,
				TRect(ClipX, ClipY, ClipWidth, ClipHeight));
		return;
	}
	if (Field->Sprites == Field->CachedSprites) {
		if (SpriteNumber == 0)
			Graphics().DrawPicturePartClipped(4, Field->x + dx, Field->y + dy, 7, 0, 0, 32, 32,
				TRect(Field->x, Field->y, 32, 32));
		else
			Graphics().DrawSpriteClipped(4, Field->x + dx, Field->y + dy, SpriteNumber,
				TRect(Field->x, Field->y, 32, 32));
		Field->Sprite[Field->CachedSprites] = SpriteNumber;
		Field->SpriteX[Field->CachedSprites] = dx;
		Field->SpriteY[Field->CachedSprites] = dy;
		Field->CachedSprites++;
		// MSVC's analysis (C28020 on the next line) takes Sprites as any int: it is below CachedSprites there (they
		// differ, and Sprites only counts up to it), and a field with 10 was flushed above.
		PATELLA_MSVC_WARNING(suppress : 28020)
	} else if (Field->Sprite[Field->Sprites] != SpriteNumber || Field->SpriteX[Field->Sprites] != dx
		|| Field->SpriteY[Field->Sprites] != dy) {
		for (int i = 0; i < Field->Sprites; i++) {
			if (Field->Sprite[i] == 0)
				Graphics().DrawPicturePartClipped(4, Field->x + Field->SpriteX[i], Field->y + Field->SpriteY[i], 7, 0,
					0, 32, 32, TRect(Field->x, Field->y, 32, 32));
			else
				Graphics().DrawSpriteClipped(4, Field->x + Field->SpriteX[i], Field->y + Field->SpriteY[i],
					Field->Sprite[i], TRect(Field->x, Field->y, 32, 32));
		}
		if (SpriteNumber == 0)
			Graphics().DrawPicturePartClipped(4, Field->x + dx, Field->y + dy, 7, 0, 0, 32, 32,
				TRect(Field->x, Field->y, 32, 32));
		else
			Graphics().DrawSpriteClipped(4, Field->x + dx, Field->y + dy, SpriteNumber,
				TRect(Field->x, Field->y, 32, 32));
		Field->Sprite[Field->Sprites] = SpriteNumber;
		Field->SpriteX[Field->Sprites] = dx;
		Field->SpriteY[Field->Sprites] = dy;
		Field->CachedSprites = Field->Sprites + 1;
	}
	Field->Sprites++;
}

// Blits a field's cache tile to the map surface, once per frame. If the
// tile holds more sprites than this frame's, they are drawn again over it
// first (without clearing it).
void TMapWindow::FlushField(int FieldX, int FieldY)
{
	if (In(FieldX, 0, MAPSIZE_X - 1) && In(FieldY, 0, MAPSIZE_Y - 1)) {
		const int x = FieldX * 32 - ScrollX;
		if (x > -32) {
			const int y = FieldY * 32 - ScrollY;
			if (y > -32 && x < 480 && y < 352) {
				TCacheField *Field = GetCacheField(FieldX, FieldY);
				if (!Field->Copied) {
					if (Field->Sprites < Field->CachedSprites) {
						for (int i = 0; i < Field->Sprites; i++) {
							if (Field->Sprite[i] == 0)
								Graphics().DrawPicturePartClipped(4, Field->x + Field->SpriteX[i], Field->y + Field->SpriteY[i], 7, 0,
									0, 32, 32, TRect(Field->x, Field->y, 32, 32));
							else
								Graphics().DrawSpriteClipped(4, Field->x + Field->SpriteX[i],
									Field->y + Field->SpriteY[i], Field->Sprite[i], TRect(Field->x, Field->y, 32, 32));
						}
						Field->CachedSprites = Field->Sprites;
					}
					Field->Copied = true;
					GraphicsEngine().GetSurface()->BlitRect(2, FieldX * 32 - ScrollX, FieldY * 32 - ScrollY, 4,
						TRect(Field->x, Field->y, 32, 32));
				}
			}
		}
	}
}

// The tiles wrap with the map position, so a field keeps its tile
// while the map scrolls.
TCacheField *TMapWindow::GetCacheField(int FieldX, int FieldY)
{
	PATELLA_ASSERT_VALUES(In(FieldX,0,MAPSIZE_X-1), FieldX);
	PATELLA_ASSERT_VALUES(In(FieldY,0,MAPSIZE_Y-1), FieldY);
	return &Cache[(PositionX + FieldX) % 16][(PositionY + FieldY) % 12];
}

// Lights the fields within Brightness fields of map surface position
// posx/posy (which are moved to field pixels first): the colour falls off
// linearly over Brightness*32 pixels (scaled by 1/160), by a further 1/5 on
// fields without a ground, and each field keeps the brightest value of each
// component.
void TMapWindow::AddLightSource(int posx, int posy, int z, int Brightness, TRGBColor Color)
{
	if (Brightness <= 0)
		return;
	posx += ScrollX;
	posy += ScrollY;
	int x = posx / 32 - Brightness;
	while (++x < posx / 32 + Brightness) {
		if (x < 0 || x >= MAPSIZE_X)
			continue;
		for (int y = posy / 32 - Brightness + 1; y < posy / 32 + Brightness; y++) {
			if (y < 0 || y >= MAPSIZE_Y)
				continue;
			const int dx = x * 32 - posx + 16;
			const int dy = y * 32 - posy + 16;
			const int Intensity = Brightness * 32 - (int)sqrt((double)dx * dx + dy * dy);
			int B = Color.B * Intensity / 160;
			int G = Color.G * Intensity / 160;
			int R = Color.R * Intensity / 160;
			TObject Obj = Map().GetMapObject(x, y, z, 0);
			try {
				if (Obj.Type == 0 || !ObjectTypes().GetObjectFlag(Obj, BANK)) {
					B = B * 4 / 5;
					G = G * 4 / 5;
					R = R * 4 / 5;
				}
			} catch (TMyException &Exception) {
				std::stringstream s;
				s << "failed to get object flag from type " << Obj.Type << " (" << posx << "," << posy << ","
				  << z << "->" << x << "," << y << "," << z << ")";
				throw TMYEXCEPTION(s.str().c_str(), "", 0, "", 0, Exception);
			}
			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;
			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;
			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;
			TRGBColor &FieldLight = Light[x][y];
			FieldLight.R = FieldLight.R > R ? FieldLight.R : R;
			FieldLight.G = FieldLight.G > G ? FieldLight.G : G;
			FieldLight.B = FieldLight.B > B ? FieldLight.B : B;
		}
	}
}

// Places the talkboxes: a box of a speaking creature over its field
// (Alignment 0-2 tells which side the creature is on), the others in the
// middle by talk mode. Overlapping boxes are pushed apart pixel by pixel
// within their limits while the total overlap shrinks. Fixed bit 1/2: the
// box sticks to a left/right or top/bottom edge, or has no creature (then
// it does not follow the walk).
void TMapWindow::ArrangeTalkboxes()
{
	std::array<int, TALKBOXES> BoxWidth;
	std::array<int, TALKBOXES> BoxHeight;
	const int Talkboxes = Talk().GetNumberOfTalkboxes();
	for (int Talkbox = 1; Talkbox <= Talkboxes; Talkbox++) {
		BoxWidth[Talkbox - 1] = 0;
		const int Lines = Talk().GetTalkboxLines(Talkbox);
		for (int Line = 0; Line < Lines; Line++) {
			const int LineWidth = Graphics().GetTextWidth(FONT_BORDERED, Talk().GetTalkboxLine(Talkbox, Line));
			BoxWidth[Talkbox - 1] = BoxWidth[Talkbox - 1] > LineWidth ? BoxWidth[Talkbox - 1] : LineWidth;
		}
		BoxHeight[Talkbox - 1] = Graphics().GetFontLineHeight(FONT_BORDERED) * Lines;
	}
	int StatusHeight;
	if (Talk().GetStatusMessage() == nullptr)
		StatusHeight = 0;
	else
		StatusHeight = Graphics().GetFontHeight(FONT_BORDERED) + 10;
	const bool NoStretching = GetNoStretching();
	const int BorderX = (480 - Width) / 2;
	const int BorderY = (352 - Height) / 2;
	std::array<int, TALKBOXES> CentreX;
	std::array<int, TALKBOXES> PosX;
	std::array<int, TALKBOXES> PosY;
	std::array<int, TALKBOXES> MinX;
	std::array<int, TALKBOXES> MaxX;
	std::array<int, TALKBOXES> MinY;
	std::array<int, TALKBOXES> MaxY;
	std::array<int, TALKBOXES> Fixed;
	for (int Talkbox = 1; Talkbox <= Talkboxes; Talkbox++) {
		const int i = Talkbox - 1;
		int x, y, z;
		Talk().GetTalkboxPosition(Talkbox, &x, &y, &z);
		int CentreY;
		if (x != 0) {
			Map().WorldToMap(&x, &y, &z);
			int cx;
			if (NoStretching)
				cx = x * 32 - BorderX - 16;
			else
				cx = (x - 1) * Width / 15 + Width / 15 / 2;
			CentreX[i] = cx;
			int cy;
			if (NoStretching)
				cy = y * 32 - BorderY - 32;
			else
				cy = (y - 1) * Height / 11;
			cy -= Talk().GetTalkboxLines(Talkbox) * Graphics().GetFontLineHeight(FONT_BORDERED) / 2;
			CentreY = cy;
			const int HalfWidth = BoxWidth[i] / 2;
			MinX[i] = HalfWidth + 1 > cx - HalfWidth ? HalfWidth + 1 : cx - HalfWidth;
			MaxX[i] = Width - HalfWidth < cx + HalfWidth ? Width - HalfWidth : cx + HalfWidth;
			const int HalfHeight = BoxHeight[i] / 2;
			MinY[i] = HalfHeight + 1 > cy ? HalfHeight + 1 : cy;
			MaxY[i] = Height - HalfHeight - StatusHeight < cy + BoxHeight[i]
				? Height - HalfHeight - StatusHeight
				: cy + BoxHeight[i];
			Fixed[i] = 0;
		} else {
			const int cx = Width / 2;
			CentreX[i] = cx;
			int cy;
			switch (Talk().GetTalkboxMode(Talkbox)) {
			case TALK_INFO_MESSAGE:
				cy = Height / 2 - BoxHeight[i] / 2 - 5;
				break;
			case TALK_GAMEMASTER_BROADCAST:
			case TALK_GAMEMASTER_MESSAGE:
			case TALK_ADMIN_MESSAGE:
			case TALK_EVENT_MESSAGE:
				cy = BoxHeight[i] / 2 + Height / 2 + 5;
				break;
			case TALK_PRIVATE_MESSAGE:
				cy = Height / 4;
				break;
			default:
				TERROR1("unknown talk box mode", Talk().GetTalkboxMode(Talkbox));
			}
			CentreY = cy;
			const int HalfWidth = BoxWidth[i] / 2;
			MinX[i] = HalfWidth + 1 > cx - HalfWidth ? HalfWidth + 1 : cx - HalfWidth;
			MaxX[i] = Width - HalfWidth < cx + HalfWidth ? Width - HalfWidth : cx + HalfWidth;
			const int HalfHeight = BoxHeight[i] / 2;
			MinY[i] = HalfHeight + 1 > cy - BoxHeight[i] ? HalfHeight + 1 : cy - BoxHeight[i];
			MaxY[i] = Height - HalfHeight - StatusHeight < BoxHeight[i] + cy
				? Height - HalfHeight - StatusHeight
				: BoxHeight[i] + cy;
			Fixed[i] = 3;
		}
		PosX[i] = CentreX[i];
		PosY[i] = CentreY;
		Limit(PosX[i], MinX[i], MaxX[i]);
		Limit(PosY[i], MinY[i], MaxY[i]);
		Limit(PosX[i], BoxWidth[i] / 2 + 1, Width - BoxWidth[i] / 2);
		Limit(PosY[i], BoxHeight[i] / 2 + 1, Height - BoxHeight[i] / 2 - StatusHeight);
	}
	int Overlap = 0x7fffffff;
	int LastOverlap;
	do {
		LastOverlap = Overlap;
		Overlap = 0;
		std::array<int, TALKBOXES> PushX;
		std::array<int, TALKBOXES> PushY;
		for (int i = 0; i < Talkboxes; i++) {
			PushX[i] = 0;
			PushY[i] = 0;
			for (int j = 0; j < Talkboxes; j++) {
				if (i == j)
					continue;
				const int dx = PosX[j] - PosX[i];
				const int dy = PosY[j] - PosY[i];
				const int Distance = abs(dx);
				const int HalfWidth1 = BoxWidth[i] / 2, HalfWidth2 = BoxWidth[j] / 2;
				if (Distance >= HalfWidth1 + HalfWidth2 + 10)
					continue;
				const int DistanceY = abs(dy);
				const int HalfHeight1 = BoxHeight[i] / 2, HalfHeight2 = BoxHeight[j] / 2;
				if (DistanceY >= HalfHeight1 + HalfHeight2 + 10)
					continue;
				int OverlapX, OverlapY;
				if (HalfWidth2 >= HalfWidth1 + Distance)
					OverlapX = BoxWidth[i] + 20;
				else if (HalfWidth1 >= HalfWidth2 + Distance)
					OverlapX = BoxWidth[j] + 20;
				else
					OverlapX = HalfWidth1 + HalfWidth2 - Distance + 10;
				if (HalfHeight2 >= HalfHeight1 + DistanceY)
					OverlapY = BoxHeight[i] + 20;
				else if (HalfHeight1 >= HalfHeight2 + DistanceY)
					OverlapY = BoxHeight[j] + 20;
				else
					OverlapY = HalfHeight1 + HalfHeight2 - DistanceY + 10;
				Overlap += OverlapY * OverlapX;
				if (dx > 0)
					PushX[i] -= OverlapX;
				if (dx < 0)
					PushX[i] += OverlapX;
				if (dy > 0)
					PushY[i] -= OverlapY;
				if (dy < 0)
					PushY[i] += OverlapY;
				if (dx == 0 && dy == 0)
					PushX[i] += i - j;
			}
		}
		for (int i = 0; i < Talkboxes; i++) {
			if (PushX[i] < 0 && PosX[i] > MinX[i])
				PosX[i]--;
			if (PushX[i] > 0 && PosX[i] < MaxX[i])
				PosX[i]++;
			if (PushY[i] < 0 && PosY[i] > MinY[i])
				PosY[i]--;
			if (PushY[i] > 0 && PosY[i] < MaxY[i])
				PosY[i]++;
		}
	} while (Overlap < LastOverlap);
	for (int Talkbox = 1; Talkbox <= Talkboxes; Talkbox++) {
		const int i = Talkbox - 1;
		const int HalfWidth = BoxWidth[i] / 2;
		if (PosX[i] == HalfWidth + 1 || PosX[i] == Width - HalfWidth)
			Fixed[i] |= 1;
		const int HalfHeight = BoxHeight[i] / 2;
		if (PosY[i] == HalfHeight + 1 || PosY[i] == Height - HalfHeight - StatusHeight)
			Fixed[i] |= 2;
		const int Quarter = BoxWidth[i] / 4;
		const int Top = PosY[i] - HalfHeight;
		if (PosX[i] < CentreX[i] - Quarter)
			Talk().SetTalkboxRectangle(Talkbox, PosX[i] + HalfWidth, Top, 2, Fixed[i]);
		else if (PosX[i] > CentreX[i] + Quarter)
			Talk().SetTalkboxRectangle(Talkbox, PosX[i] - HalfWidth, Top, 0, Fixed[i]);
		else
			Talk().SetTalkboxRectangle(Talkbox, PosX[i], Top, 1, Fixed[i]);
	}
}

// The map field under window position x/y: the level is the highest one
// down from MaxLevel with a ground or a BOTTOM object (first or second)
// on the field; 0 if there is none.
void TMapWindow::GetField(int x, int y, int *FieldX, int *FieldY, int *FieldZ)
{
	const TCreature *Player = Creatures().GetCreature(PlayerState().GetPlayerID());
	PATELLA_ASSERT(Player!=nullptr);
	if (GetNoStretching()) {
		*FieldX = ((480 - Width) / 2 + Player->GetDeltaX() + x) / 32 + 1;
		*FieldY = ((352 - Height) / 2 + Player->GetDeltaY() + y) / 32 + 1;
	} else {
		*FieldX = (x * 480 / Width + Player->GetDeltaX()) / 32 + 1;
		*FieldY = (y * 352 / Height + Player->GetDeltaY()) / 32 + 1;
	}
	for (*FieldZ = MaxLevel; *FieldZ > 0; (*FieldZ)--) {
		TObject Obj = Map().GetFirstObject(*FieldX, *FieldY, *FieldZ);
		if (Obj.Type != 0) {
			if (ObjectTypes().GetObjectFlag(Obj, BANK) || ObjectTypes().GetObjectFlag(Obj, BOTTOM))
				return;
			Obj = Map().GetNextObject();
			if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BOTTOM))
				return;
		}
	}
}

// The creature whose figure (its name position, the top centre) is under
// window position x/y, among the creatures with a name that no ground
// hides; 0 if there is none.
uint32 TMapWindow::GetCreatureAt(int x, int y)
{
	const int FieldWidth = Width / 15;
	const int FieldHeight = Height / 11;
	for (int i = 0; i < NumberOfNames; i++) {
		const TCreature *Creature = Creatures().GetCreature(Names[i].CreatureID);
		if (Creature == nullptr)
			continue;
		int cx = Creature->RealX, cy = Creature->RealY, cz = Creature->RealZ;
		Map().WorldToMap(&cx, &cy, &cz);
		if (!In(cx, 0, MAPSIZE_X - 1) || !In(cy, 0, MAPSIZE_Y - 1))
			continue;
		bool Visible = true;
		while (++cz <= MaxLevel) {
			TObject Obj = Map().GetFirstObject(cx, cy, cz);
			if (Obj.Type != 0 && ObjectTypes().GetObjectFlag(Obj, BANK))
				Visible = false;
		}
		if (!Visible)
			continue;
		if (GetNoStretching()) {
			const int dx = (480 - Width) / 2 - Names[i].x + x;
			const int dy = y - (Names[i].y - (352 - Height) / 2);
			if (In(dx, -16, 16) && In(dy, 0, 32))
				return Creature->CreatureID;
		} else {
			const int dx = x - Names[i].x * Width / 480;
			const int dy = y - Names[i].y * Height / 352;
			if (In(dx, -(FieldWidth / 2), FieldWidth / 2) && In(dy, 0, FieldHeight))
				return Creature->CreatureID;
		}
	}
	return 0;
}

// --- TMapFrame -------------------------------------------------------------

// Eight drawing list entries: the four border sides and the background
// around them.
TMapFrame::TMapFrame()
{
	MapWindow = new TMapWindow;
	InsertChild(MapWindow);
	DrawingListLength = 8;
	DrawingList.reset(new TDrawing[8]);
}

// The map view keeps 15:11 (at most 480 pixels wide without stretching)
// and is centred horizontally; the frame's height follows it.
void TMapFrame::SetSize(int Width, int Height)
{
	const int Border = SkinProperties().MapFrameBorder;
	const int MaxWidth = (Height - Border * 2) * 15 / 11 + 2;
	ViewWidth = Width - Border * 2 < MaxWidth ? Width - Border * 2 : MaxWidth;
	if (GetNoStretching())
		ViewWidth = ViewWidth < 480 ? ViewWidth : 480;
	ViewHeight = (int)floor(ViewWidth * (11.0f / 15.0f));
	ViewX = (Width - SkinProperties().MapFrameBorder * 2 - ViewWidth) / 2 + SkinProperties().MapFrameBorder;
	ViewY = SkinProperties().MapFrameBorder;
	MapWindow->SetSize(ViewWidth, ViewHeight);
	MapWindow->SetPosition(ViewX, ViewY);
	TWindow::SetSize(Width, ViewHeight + SkinProperties().MapFrameBorder * 2);
	Arrange();
}

int TMapFrame::GetMinimumWidth()
{
	return MapWindow->GetMinimumWidth() + SkinProperties().MapFrameBorder * 2;
}

int TMapFrame::GetMinimumHeight()
{
	return MapWindow->GetMinimumHeight() + SkinProperties().MapFrameBorder * 2;
}

// The border (skins 8-11) and skin 7 tiled over the rest, its pattern
// aligned to the frame.
void TMapFrame::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_TOP, ViewX - Skins()[SKIN_FRAME_EDGE_LEFT].width, ViewY - Skins()[SKIN_FRAME_EDGE_TOP].height,
		Skins()[SKIN_FRAME_EDGE_RIGHT].width + Skins()[SKIN_FRAME_EDGE_LEFT].width + ViewWidth, Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_LEFT, ViewX - Skins()[SKIN_FRAME_EDGE_LEFT].width, ViewY, Skins()[SKIN_FRAME_EDGE_LEFT].width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height + ViewHeight, 0, 0);
	DrawingList[2] = TDrawing(SKIN_FRAME_EDGE_RIGHT, ViewX + ViewWidth, ViewY, Skins()[SKIN_FRAME_EDGE_RIGHT].width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height + ViewHeight, 0, 0);
	DrawingList[3] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, ViewX, ViewY + ViewHeight, ViewWidth, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
	const int Top = ViewY - Skins()[SKIN_FRAME_EDGE_TOP].height;
	const int Left = ViewX - Skins()[SKIN_FRAME_EDGE_LEFT].width;
	const int Right = Skins()[SKIN_FRAME_EDGE_RIGHT].width + ViewX + ViewWidth;
	const int RightWidth = Width - Right;
	const int Bottom = Skins()[SKIN_FRAME_EDGE_BOTTOM].height + ViewY + ViewHeight;
	DrawingList[4] = TDrawing(SKIN_BACKGROUND, 0, 0, Width, Top, 0, 0);
	DrawingList[5] = TDrawing(SKIN_BACKGROUND, 0, Top, Left, Height - Top, 0, Top % Skins()[SKIN_BACKGROUND].height);
	DrawingList[6] = TDrawing(SKIN_BACKGROUND, Right, Top, RightWidth, Height - Top, Right % Skins()[SKIN_BACKGROUND].width, Top % Skins()[SKIN_BACKGROUND].height);
	DrawingList[7] = TDrawing(SKIN_BACKGROUND, Left, Bottom, Width - RightWidth - Left, Height - Bottom, Left % Skins()[SKIN_BACKGROUND].width,
		Bottom % Skins()[SKIN_BACKGROUND].height);
}
// NOLINTEND(cppcoreguidelines-owning-memory)

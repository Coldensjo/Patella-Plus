// ContextMenus.h: the help bubble and the context menus, modal windows that
// Control.cpp creates (the help bubble in action state 9, help mode).
//
// THelpBubble shows a help text next to the window it explains (Help.cpp's
// help area), on the side where it fits. A TContextMenu is a column of
// entries (TContextMenuEntry: a text, an optional shortcut on the right and a
// callback) with separators between groups. Its subclasses fill it in their
// constructors and do the work of an entry in HandleEvent: the menus of a map
// object (TObjectContextMenu), of a creature in the battle list
// (TCreatureContextMenu), of a VIP (TBuddyContextMenu), of a console line
// (TTextContextMenu), of a line in the rule violation channel
// (TRequestContextMenu), of the console channel (TChannelContextMenu), of an
// edit line (TEditContextMenu), of an automap point (TMarkContextMenu) and of
// the skill window (TSkillContextMenu). Every click or Escape ends the action
// state, and Control.cpp deletes the menu.
//
// Names from the binary: the members LookedObject,
// LookedObjectX, UsedObjectX, UsedObjectY, LastEntry and the parameter names
// in the assertions and catch blocks (HelpText, Text, Shortcut, Event,
// Surface, Width, Height, x, y, Key, CreatureID, _CharacterID, _TextWindow,
// _LineNumber, _ConsoleWindow, _Edit, _Data, _SkillsWindow, TextBuffer). The
// class names (those of Control.cpp's earlier sketches) and the other names
// are ours; see notes/decisions.md, "ContextMenus.cpp".

#pragma once

#include "common.h"
#include "GUI.h"
#include "Map.h"     // TCoordinate, TMark
#include "Status.h"  // SetActionState
#include "Objects.h" // TObject
#include "types/FixedString.h"

class TConsoleWindow;    // ConsoleWindow.h
class TTextWindow;       // ConsoleWindow.h
class TEditLine;         // DialogElements.h
class TFormattedText;    // DialogElements.h
class TSkillsWindowBody; // ContentWindows.h
class TAutomapWindow;    // ContentWindows.h

// The help bubble: a formatted text in a frame of skins 52-60.
class THelpBubble : public TWindow {
public:
	THelpBubble(const char *HelpText);
	void SetSize(int Width, int Height) override;
	int getMinimumWidth() override { return SkinProperties().HelpBubbleMinimumWidth; }
	int getMinimumHeight() override { return SkinProperties().HelpBubbleMinimumHeight; }
	void Arrange() override;
	void Place(TWindow *Window);
	bool FitsIn(int &x, int &y, int &Width, int &Height, TWindow *Window);

	TFormattedText *Text; // +0x2c
};

// One entry of a context menu.
class TContextMenuEntry final : public TElement {
public:
	TContextMenuEntry(const char *_Text, const char *_Shortcut, TCallback _Callback);
	void Draw(int Surface, int x, int y) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void SetLast();

	TFixedString<50> Text;     // +0x24
	TFixedString<20> Shortcut; // +0x56
	TCallback Callback;        // +0x6c
};

// The base class of the menus.
class TContextMenu : public TWindow {
public:
	TContextMenu();
	void Arrange() override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void KeyPress(int Key) override;
	// The subclasses override it; an entry of the base class only closes the
	// menu.
	void HandleEvent([[maybe_unused]] int Event) override { SetActionState(ACTIONSTATE_NONE); }
	void Show(int x, int y, int ParentWidth, int ParentHeight);
	void AddEntry(int Event, const char *Text, const char *Shortcut);
	void AddSeparator();

	int EntryWidth; // +0x2c, the widest entry
	bool Separator; // +0x30, the next entry starts a new group
	bool Empty;     // +0x31, no entry yet
};

// The menu of a map or inventory object: the looked-at object and the
// object to use (Status.cpp's GetTradeObject, GetUseObject) and the creature
// on the field (GetTradeInfo).
class TObjectContextMenu : public TContextMenu {
public:
	TObjectContextMenu(int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	TObject LookedObject;  // +0x34
	int LookedObjectIndex; // +0x40, the position on the field or in the container
	int LookedObjectX;     // +0x44, 0xffff in a container or the inventory
	int LookedObjectY;     // +0x48, then 64 plus the container number
	int LookedObjectZ;     // +0x4c
	TObject UsedObject;    // +0x50
	int UsedObjectIndex;   // +0x5c
	int UsedObjectX;       // +0x60
	int UsedObjectY;       // +0x64
	int UsedObjectZ;       // +0x68
	uint32 CreatureID;     // +0x6c, 0 without a creature
};

// The menu of a creature in the battle list.
class TCreatureContextMenu : public TContextMenu {
public:
	TCreatureContextMenu(uint32 CreatureID, int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	uint32 CreatureID; // +0x34
};

// The menu of the VIP list, on a VIP or on the free space.
class TBuddyContextMenu : public TContextMenu {
public:
	TBuddyContextMenu(uint32 _CharacterID, int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	uint32 CharacterID; // +0x34
};

// The menu of a line in a console channel.
class TTextContextMenu : public TContextMenu {
public:
	TTextContextMenu(TTextWindow *_TextWindow, int _LineNumber, int x, int y, int ParentWidth,
		int ParentHeight);
	void HandleEvent(int Event) override;

	TTextWindow *TextWindow; // +0x34
	int LineNumber;          // +0x38
};

// The menu of a request in the rule violation channel.
class TRequestContextMenu : public TContextMenu {
public:
	TRequestContextMenu(TConsoleWindow *_ConsoleWindow, int _LineNumber, int x, int y, int ParentWidth,
		int ParentHeight);
	void HandleEvent(int Event) override;

	TConsoleWindow *ConsoleWindow; // +0x34
	int LineNumber;                // +0x38
};

// The menu of the console's current channel (its tab).
class TChannelContextMenu : public TContextMenu {
public:
	TChannelContextMenu(TConsoleWindow *_ConsoleWindow, int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	TConsoleWindow *ConsoleWindow; // +0x34
	bool ShowServerMessages;       // +0x38, the value "Show/Hide server messages" sets
};

// The menu of an edit line.
class TEditContextMenu : public TContextMenu {
public:
	TEditContextMenu(TEditLine *_Edit, int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	TEditLine *Edit; // +0x34
};

// The point of the automap a mark menu is opened for (0x80 bytes). The
// automap view (ContentWindows.cpp, TAutomapView::RightButtonUp) makes it with new and the
// menu's constructor deletes it; its constructor (inlined there and in the
// menu's) clears the coordinate and the mark.
struct TMarkMenuData {
	TMarkMenuData()
	{
		Coordinate.x = 0;
		Coordinate.y = 0;
		Coordinate.z = 0;
	}

	TCoordinate Coordinate; // +0x00, the field
	TMark Mark;             // +0x0c, the mark there; x and y 0 without one
};

// The menu of a point of the automap.
class TMarkContextMenu : public TContextMenu {
public:
	TMarkContextMenu(TMarkMenuData *_Data, int x, int y, int ParentWidth, int ParentHeight);
	void HandleEvent(int Event) override;

	TMarkMenuData Data; // +0x34
};

// The menu of the skill window: which bars it shows.
class TSkillContextMenu : public TContextMenu {
public:
	TSkillContextMenu(TSkillsWindowBody *_SkillsWindow, int x, int y, int ParentWidth, int ParentHeight);
	~TSkillContextMenu() override;
	void HandleEvent(int Event) override;

	TSkillsWindowBody *SkillsWindow; // +0x34, its ContextMenu points back
};

// The skill window's side of the link between it and its menu (issue #118), defined in ContentWindows.cpp where
// TSkillsWindowBody is complete, so that ContextMenus.cpp does not include ContentWindows.h (the content windows make
// the menus, the menus called back into them: the two modules were a cycle). The menu's constructor and destructor
// set and clear the window's ContextMenu with SetSkillsWindowMenu (the original's stores at +0x30), its HandleEvent
// lays the window out again with ArrangeSkillsWindow (the window's virtual Arrange, as the original's call).
void SetSkillsWindowMenu(TSkillsWindowBody *Window, TSkillContextMenu *Menu) noexcept;
void ArrangeSkillsWindow(TSkillsWindowBody *Window);

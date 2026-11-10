// Skins.cpp: rectangles of the GUI graphics (file name inferred), and SkinProperties, the window graphics' positions,
// sizes, colours and fonts (GUI.cpp's global 0x5ccfc8, which InitSkinProperties fills).
//
// TSkins (Patella-Modern issue #116, M7; notes/decisions.md, "#104: TClient and the subsystem objects" and "#116"):
// Skins.cpp's data and SkinProperties, a part of TGraphics (graphics.h), declared here and defined in Skins.cpp, as
// TSession is Status.cpp's inside TCommunication. SkinProperties' types moved here from GUI.h with it. Portable
// (patella_core): the bridge accessor and the original's free functions are the client's (graphics.cpp), since
// patella_core has no TClient.

#pragma once

#include "types/GraphicsTypes.h"
#include <array>
#include <memory>

// Number of skin entries; the terminator of the table in InitSkins.
inline constexpr int LASTSKIN = 262;

// One rectangle in the GUI picture. width and height are named in GUI.cpp's
// assertions; x and y are ours.
struct TSkin {
	int x;      // +0x0
	int y;      // +0x4
	int width;  // +0x8
	int height; // +0xc
};

// The skin numbers: the index into Skins (InitSkins' table, Skins.cpp), the skin of a TDrawing and of DrawSkin, the
// two skins of a TSkinButton. One name of the original is known, CONSOLETEXT_CORNER_TOPRIGHT, from the error text of
// TComponent::Draw (GUI.cpp); the frames' names follow its pattern (CORNER_ and EDGE_ with the side, BACKGROUND for
// the tiled middle). Every other name is taken from what the code that uses the skin draws with it: the pictures
// themselves were not looked at. A button's two skins are _UP (released) and _DOWN (pressed).
//
// Without a name, because no code uses them by number or shows what they are: 3 (the picture of mouse cursor 3, which
// nothing sets), 20, 21, 34-41 and 216. 152 has no entry in the table. The help texts (Help.cpp) keep their skin
// numbers: they are arguments of std::format, which does not take an enumerator.
//
// Enumerators for the reason GUI.h gives at the modifier flags.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// Nothing is drawn (DrawSkin, TComponent::Draw): an empty TDrawing, a TBorder without a corner or a background.
	SKIN_NONE = 0,

	// The mouse cursors: Control.cpp draws the skin of the mouse cursor's number (Status.cpp's GetMouseCursor).
	SKIN_CURSOR_DEFAULT = 1,
	SKIN_CURSOR_RESIZE = 2,   // over a TResizer and the main window's splitter
	SKIN_CURSOR_USE_WITH = 4, // ACTIONSTATE_USE_WITH and ACTIONSTATE_TRADE_WITH
	SKIN_CURSOR_DRAGGING = 5, // ACTIONSTATE_DRAGGING
	SKIN_CURSOR_HELP = 6,     // ACTIONSTATE_HELP

	SKIN_BACKGROUND = 7, // tiled: the map's frame, the side bar and its windows, the splitter

	// The thin frame of the edit lines, the list boxes, the map and the automap; a TSeparator is the top above the
	// bottom edge or the left beside the right one, the splitter the bottom above the top edge.
	SKIN_FRAME_EDGE_BOTTOM = 8,
	SKIN_FRAME_EDGE_TOP = 9,
	SKIN_FRAME_EDGE_RIGHT = 10,
	SKIN_FRAME_EDGE_LEFT = 11,

	// The text buttons, by their widths in the table: 43 (TTextButton's own), 34, 58 and 86 pixels.
	SKIN_BUTTON_UP = 12,
	SKIN_BUTTON_DOWN = 13,
	SKIN_SMALL_BUTTON_UP = 14,
	SKIN_SMALL_BUTTON_DOWN = 15,
	SKIN_MEDIUM_BUTTON_UP = 16,
	SKIN_MEDIUM_BUTTON_DOWN = 17,
	SKIN_LARGE_BUTTON_UP = 18,
	SKIN_LARGE_BUTTON_DOWN = 19,
	SKIN_COLOR_BUTTON_UP = 22, // TColorButton, the outfit dialog's colours
	SKIN_COLOR_BUTTON_DOWN = 23,
	SKIN_ICON_BUTTON_UP = 24, // TIconButton, under a buddy's icon or a mark
	SKIN_ICON_BUTTON_DOWN = 25,

	// TBorder's own frame: the side bar's, and of the free space below the side bar's windows (TDialogWindow).
	SKIN_BORDER_CORNER_TOPLEFT = 26,
	SKIN_BORDER_CORNER_TOPRIGHT = 27,
	SKIN_BORDER_CORNER_BOTTOMLEFT = 28,
	SKIN_BORDER_CORNER_BOTTOMRIGHT = 29,
	SKIN_BORDER_EDGE_LEFT = 30,
	SKIN_BORDER_EDGE_RIGHT = 31,
	SKIN_BORDER_EDGE_TOP = 32,
	SKIN_BORDER_EDGE_BOTTOM = 33,

	SKIN_RESIZER = 42, // TResizer, when it is visible

	// TDialog's frame; the top edge is the bar a dialog is dragged by.
	SKIN_DIALOG_CORNER_TOPLEFT = 43,
	SKIN_DIALOG_CORNER_TOPRIGHT = 44,
	SKIN_DIALOG_CORNER_BOTTOMLEFT = 45,
	SKIN_DIALOG_CORNER_BOTTOMRIGHT = 46,
	SKIN_DIALOG_EDGE_LEFT = 47,
	SKIN_DIALOG_EDGE_RIGHT = 48,
	SKIN_DIALOG_EDGE_TOP = 49,
	SKIN_DIALOG_EDGE_BOTTOM = 50,
	SKIN_DIALOG_BACKGROUND = 51,

	// The frame of the help bubble and of the start window's panel of buttons.
	SKIN_PANEL_CORNER_TOPLEFT = 52,
	SKIN_PANEL_CORNER_TOPRIGHT = 53,
	SKIN_PANEL_CORNER_BOTTOMLEFT = 54,
	SKIN_PANEL_CORNER_BOTTOMRIGHT = 55,
	SKIN_PANEL_EDGE_LEFT = 56,
	SKIN_PANEL_EDGE_RIGHT = 57,
	SKIN_PANEL_EDGE_TOP = 58,
	SKIN_PANEL_EDGE_BOTTOM = 59,
	SKIN_PANEL_BACKGROUND = 60,

	// The map window's hint button, a TBlinkingButton: its two pairs of skins.
	SKIN_HINT_BUTTON_UP1 = 61,
	SKIN_HINT_BUTTON_DOWN1 = 62,
	SKIN_HINT_BUTTON_UP2 = 63,
	SKIN_HINT_BUTTON_DOWN2 = 64,

	// The automap window.
	SKIN_COMPASS_ROSE = 65,
	SKIN_ZOOM_IN_BUTTON_UP = 66,
	SKIN_ZOOM_IN_BUTTON_DOWN = 67,
	SKIN_ZOOM_OUT_BUTTON_UP = 68,
	SKIN_ZOOM_OUT_BUTTON_DOWN = 69,
	SKIN_HIGHER_FLOOR_BUTTON_UP = 70,
	SKIN_HIGHER_FLOOR_BUTTON_DOWN = 71,
	SKIN_LOWER_FLOOR_BUTTON_UP = 72,
	SKIN_LOWER_FLOOR_BUTTON_DOWN = 73,
	// The automap's marks: SKIN_MARK_0 plus the mark's type.
	SKIN_MARK_0 = 74,
	SKIN_MARK_1 = 75,
	SKIN_MARK_2 = 76,
	SKIN_MARK_3 = 77,
	SKIN_MARK_4 = 78,
	SKIN_MARK_5 = 79,
	SKIN_MARK_6 = 80,
	SKIN_MARK_7 = 81,
	SKIN_MARK_8 = 82,
	SKIN_MARK_9 = 83,
	SKIN_MARK_10 = 84,
	SKIN_MARK_11 = 85,
	SKIN_MARK_12 = 86,
	SKIN_MARK_13 = 87,

	// The inventory. An empty body slot's picture, by the slot's number (SkinProperties' InventoryView.EmptySlotSkins).
	SKIN_EMPTY_SLOT_1 = 88,
	SKIN_EMPTY_SLOT_2 = 89,
	SKIN_EMPTY_SLOT_3 = 90,
	SKIN_EMPTY_SLOT_4 = 91,
	SKIN_EMPTY_SLOT_6 = 92,
	SKIN_EMPTY_SLOT_5 = 93,
	SKIN_EMPTY_SLOT_7 = 94,
	SKIN_EMPTY_SLOT_8 = 95,
	SKIN_EMPTY_SLOT_9 = 96,
	SKIN_EMPTY_SLOT_10 = 97,
	SKIN_SOUL_CAPACITY_BACKGROUND = 98,  // behind each of the two numbers
	SKIN_SMALL_INVENTORY_DROP_AREA = 99, // the small inventory's area for the body slots
	SKIN_PLAYER_STATES_BACKGROUND = 100,
	SKIN_PLAYER_STATES_EDGE_RIGHT = 101, // of the small inventory's row
	SKIN_SLOT_FRAME = 102,               // around a body slot and a framed TObjectIcon

	// The combat controls; the options and help buttons are the small inventory's.
	SKIN_OFFENSIVE_BUTTON_UP = 103,
	SKIN_OFFENSIVE_BUTTON_DOWN = 104,
	SKIN_BALANCED_BUTTON_UP = 105,
	SKIN_BALANCED_BUTTON_DOWN = 106,
	SKIN_DEFENSIVE_BUTTON_UP = 107,
	SKIN_DEFENSIVE_BUTTON_DOWN = 108,
	SKIN_STAND_BUTTON_UP = 109,
	SKIN_STAND_BUTTON_DOWN = 110,
	SKIN_CHASE_BUTTON_UP = 111,
	SKIN_CHASE_BUTTON_DOWN = 112,
	SKIN_SECURE_BUTTON_UP = 113,
	SKIN_SECURE_BUTTON_DOWN = 114,
	SKIN_OPTIONS_BUTTON_UP = 115,
	SKIN_OPTIONS_BUTTON_DOWN = 116,
	SKIN_HELP_BUTTON_UP = 117,
	SKIN_HELP_BUTTON_DOWN = 118,

	// The status window: the picture beside each bar, the bar's background and the bar.
	SKIN_HITPOINTS_ICON = 119,
	SKIN_HITPOINTS_BAR_BACKGROUND = 120,
	SKIN_HITPOINTS_BAR = 121,
	SKIN_MANA_ICON = 122,
	SKIN_MANA_BAR_BACKGROUND = 123,
	SKIN_MANA_BAR = 124,

	SKIN_CONTAINER_SLOT = 125, // a slot of a container and of a trade offer

	// TSubWindow, a window of the side bar: its buttons, its frame and the icons of its title.
	SKIN_HIGHER_CONTAINER_BUTTON_UP = 126,
	SKIN_HIGHER_CONTAINER_BUTTON_DOWN = 127,
	SKIN_SUBWINDOW_CORNER_TOPLEFT = 128,
	SKIN_SUBWINDOW_CORNER_TOPRIGHT = 129,
	SKIN_SUBWINDOW_CORNER_BOTTOMLEFT = 130,
	SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT = 131,
	SKIN_SUBWINDOW_EDGE_TOP = 132,
	SKIN_SUBWINDOW_EDGE_BOTTOM = 133,
	SKIN_SUBWINDOW_EDGE_LEFT = 134,
	SKIN_SUBWINDOW_EDGE_RIGHT = 135,
	SKIN_CLOSE_BUTTON_UP = 136,
	SKIN_CLOSE_BUTTON_DOWN = 137,
	SKIN_MINIMISE_BUTTON_UP = 138, // also the inventory's
	SKIN_MINIMISE_BUTTON_DOWN = 139,
	SKIN_MAXIMISE_BUTTON_UP = 140, // of a minimised window and of the small inventory
	SKIN_MAXIMISE_BUTTON_DOWN = 141,
	SKIN_SKILLS_ICON = 142,
	SKIN_VIP_ICON = 143,
	SKIN_BATTLE_ICON = 144,
	SKIN_TRADE_ICON = 145,

	// TCheckBox: its frame and its box.
	SKIN_CHECKBOX_EDGE_LEFT = 146,
	SKIN_CHECKBOX_EDGE_RIGHT = 147,
	SKIN_CHECKBOX_EDGE_TOP = 148,
	SKIN_CHECKBOX_EDGE_BOTTOM = 149,
	SKIN_CHECKBOX_CHECKED = 150,
	SKIN_CHECKBOX_UNCHECKED = 151, // also a disabled one

	// TScrollBar and THorizontalScrollBar: the track, the slider's three parts and the two buttons.
	SKIN_SCROLLBAR_TRACK = 153,
	SKIN_SCROLLBAR_SLIDER_TOP = 154,
	SKIN_SCROLLBAR_SLIDER_MIDDLE = 155,
	SKIN_SCROLLBAR_SLIDER_BOTTOM = 156,
	SKIN_SCROLL_UP_BUTTON_UP = 157,
	SKIN_SCROLL_UP_BUTTON_DOWN = 158,
	SKIN_SCROLL_DOWN_BUTTON_UP = 159,
	SKIN_SCROLL_DOWN_BUTTON_DOWN = 160,
	SKIN_HSCROLLBAR_TRACK = 161,
	SKIN_HSCROLLBAR_SLIDER_LEFT = 162,
	SKIN_HSCROLLBAR_SLIDER_MIDDLE = 163,
	SKIN_HSCROLLBAR_SLIDER_RIGHT = 164,
	SKIN_SCROLL_LEFT_BUTTON_UP = 165,
	SKIN_SCROLL_LEFT_BUTTON_DOWN = 166,
	SKIN_SCROLL_RIGHT_BUTTON_UP = 167,
	SKIN_SCROLL_RIGHT_BUTTON_DOWN = 168,

	// The console. TConsole's frame, around a channel's text.
	SKIN_CONSOLETEXT_CORNER_TOPLEFT = 169,
	SKIN_CONSOLETEXT_CORNER_TOPRIGHT = 170, // the original's name
	SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT = 171,
	SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT = 172,
	SKIN_CONSOLETEXT_EDGE_LEFT = 173,
	SKIN_CONSOLETEXT_EDGE_RIGHT = 174,
	SKIN_CONSOLETEXT_EDGE_TOP = 175,
	SKIN_CONSOLETEXT_EDGE_BOTTOM = 176,
	// TConsoleWindow's frame.
	SKIN_CONSOLE_CORNER_TOPLEFT = 177,
	SKIN_CONSOLE_CORNER_TOPRIGHT = 178,
	SKIN_CONSOLE_CORNER_BOTTOMLEFT = 179,
	SKIN_CONSOLE_CORNER_BOTTOMRIGHT = 180,
	SKIN_CONSOLE_EDGE_LEFT = 181,
	SKIN_CONSOLE_EDGE_RIGHT = 182,
	SKIN_CONSOLE_EDGE_TOP = 183,
	SKIN_CONSOLE_EDGE_BOTTOM = 184,
	SKIN_CONSOLE_BACKGROUND = 185,
	SKIN_TAB_ACTIVE = 186,
	SKIN_TAB_INACTIVE = 187,
	SKIN_CLOSE_CHANNEL_BUTTON_UP = 188,
	SKIN_CLOSE_CHANNEL_BUTTON_DOWN = 189,
	SKIN_SERVER_MESSAGES_BUTTON_UP = 190,
	SKIN_SERVER_MESSAGES_BUTTON_DOWN = 191,
	SKIN_IGNORE_BUTTON_UP = 192,
	SKIN_IGNORE_BUTTON_UP_THROTTLED = 193, // while IsTalkThrottled
	SKIN_IGNORE_BUTTON_DOWN = 194,
	SKIN_NEW_CHANNEL_BUTTON_UP = 195,
	SKIN_NEW_CHANNEL_BUTTON_DOWN = 196,
	// The volume button's three states: the volume the player talks with while it shows.
	SKIN_SAY_BUTTON_UP = 197,
	SKIN_SAY_BUTTON_DOWN = 198,
	SKIN_WHISPER_BUTTON_UP = 199,
	SKIN_WHISPER_BUTTON_DOWN = 200,
	SKIN_YELL_BUTTON_UP = 201,
	SKIN_YELL_BUTTON_DOWN = 202,
	SKIN_TABS_RIGHT_BUTTON_UP = 203, // "Move channels to the right"
	SKIN_TABS_RIGHT_BUTTON_DOWN = 204,
	SKIN_TABS_LEFT_BUTTON_UP = 205,
	SKIN_TABS_LEFT_BUTTON_DOWN = 206,

	// TContextMenu's frame.
	SKIN_MENU_BACKGROUND = 207,
	SKIN_MENU_CORNER_TOPLEFT = 208,
	SKIN_MENU_CORNER_TOPRIGHT = 209,
	SKIN_MENU_CORNER_BOTTOMLEFT = 210,
	SKIN_MENU_CORNER_BOTTOMRIGHT = 211,
	SKIN_MENU_EDGE_LEFT = 212,
	SKIN_MENU_EDGE_RIGHT = 213,
	SKIN_MENU_EDGE_TOP = 214,
	SKIN_MENU_EDGE_BOTTOM = 215,

	SKIN_EDIT_LIST_PICTURE = 217, // the picture in TEditListDialog
	// A buddy's icon: SKIN_BUDDY_ICON_BASE plus the icon's number, 1-10 (icon 0 is no icon).
	SKIN_BUDDY_ICON_BASE = 217,
	SKIN_BUDDY_ICON_1 = 218,
	SKIN_BUDDY_ICON_2 = 219,
	SKIN_BUDDY_ICON_3 = 220,
	SKIN_BUDDY_ICON_4 = 221,
	SKIN_BUDDY_ICON_5 = 222,
	SKIN_BUDDY_ICON_6 = 223,
	SKIN_BUDDY_ICON_7 = 224,
	SKIN_BUDDY_ICON_8 = 225,
	SKIN_BUDDY_ICON_9 = 226,
	SKIN_BUDDY_ICON_10 = 227,

	// The player's states in the inventory (TPlayerStates::GetIcon). A skull's and a shield's number is the
	// creature's PKFlag or PARTYFlag.
	SKIN_STATE_POISONED = 228,
	SKIN_STATE_BURNING = 229,
	SKIN_STATE_ELECTRIFIED = 230,
	SKIN_STATE_DRUNK = 231,
	SKIN_STATE_MANA_SHIELD = 232,
	SKIN_STATE_PARALYZED = 233,
	SKIN_STATE_HASTED = 234,
	SKIN_STATE_IN_FIGHT = 235,
	SKIN_STATE_SKULL_1 = 236,
	SKIN_STATE_SKULL_2 = 237,
	SKIN_STATE_SKULL_3 = 238,
	SKIN_STATE_SKULL_4 = 239,
	// A creature's skull and party shield, in the battle window and above the creature on the map.
	SKIN_BATTLE_SKULL_1 = 240,
	SKIN_BATTLE_SKULL_2 = 241,
	SKIN_BATTLE_SKULL_3 = 242,
	SKIN_BATTLE_SKULL_4 = 243,
	SKIN_MAP_SKULL_1 = 244,
	SKIN_MAP_SKULL_2 = 245,
	SKIN_MAP_SKULL_3 = 246,
	SKIN_MAP_SKULL_4 = 247,
	SKIN_BATTLE_SHIELD_1 = 248, // shown only while the player is in no party
	SKIN_BATTLE_SHIELD_2 = 249,
	SKIN_BATTLE_SHIELD_3 = 250,
	SKIN_BATTLE_SHIELD_4 = 251,
	SKIN_MAP_SHIELD_1 = 252, // shown only while the player is in no party
	SKIN_MAP_SHIELD_2 = 253,
	SKIN_MAP_SHIELD_3 = 254,
	SKIN_MAP_SHIELD_4 = 255,

	// TProgressBar: the empty bar's three parts and the filled bar's.
	SKIN_PROGRESS_LEFT = 256,
	SKIN_PROGRESS_MIDDLE = 257,
	SKIN_PROGRESS_RIGHT = 258,
	SKIN_PROGRESS_FILLED_LEFT = 259,
	SKIN_PROGRESS_FILLED_MIDDLE = 260,
	SKIN_PROGRESS_FILLED_RIGHT = 261,
};

// --- Skin properties (GUI.h's until issue #116) ---------------------------

struct TGUIPoint {
	TGUIPoint() : x(0), y(0) {}
	int x, y;
};

// SkinProperties (0x5ccfc8, 0x1528 bytes): positions, sizes, colours and
// fonts of the window graphics, filled in by InitSkinProperties (0x498160).
// Its constructor (0x496800) and those of the blocks below (the gap
// 0x495480-0x4964ff) are compiler-generated: they construct the members
// that have constructors (TGUIPoint, TRGBColor, TRect, TFont) and leave the
// ints alone. Names from the assertions where there are any. The other
// names are ours, after the window or element that reads the member and what
// it does with the value; a member or block that no code reads, or whose use
// does not show what it is, is named by its offset. Some TRects hold two
// positions (x, y and width, height): their comments say which. The block
// boundaries are those of the out-of-line block constructors; blocks whose
// constructor was inlined are
// not visible and their members are direct members here (for example
// those of the inventory, which ContentWindows.cpp reads as arrays). The member types of the
// zero-initialised members (TGUIPoint, TRGBColor or TRect) are guesses from
// the values InitSkinProperties stores.

// GUIStyle+0xd8 (0x5cd0a0); compiler-generated constructor 0x495480. The
// start window's buttons (MainWindow.cpp): the size of their panel, its
// position and the positions of "Enter Game", "Options", "Info", "Exit
// Game" and "Access Account" in it (the constructor zeroes +0x08-+0x47,
// which leaves the types open; MainWindow.cpp reads +0x08-+0x37 as points,
// +0x38-+0x47 is not used).
struct TStartWindowProperties {
	int PanelWidth;                // +0x00
	int PanelHeight;               // +0x04
	TGUIPoint FramePosition;       // +0x08, the position of the frame around the panel (SetChildPosition)
	TGUIPoint EnterGameButton;     // +0x10, "Enter Game"
	TGUIPoint OptionsButton;       // +0x18, "Options"
	TGUIPoint InfoButton;          // +0x20, "Info"
	TGUIPoint ExitGameButton;      // +0x28, "Exit Game"
	TGUIPoint AccessAccountButton; // +0x30, "Access Account"
	TRect Data38;                  // +0x38, zeroed: a type with a constructor (a guess)
	TFont Data48;                  // +0x48
};

// GUIStyle+0x194 (0x5cd15c); compiler-generated constructor 0x4954f0.
// The frame of a sub window (TSubWindow, DialogWindow.cpp).
struct TSubWindowProperties {
	TRect Icon;               // +0x00, the icon's area (TSubWindow::SetIcon)
	TGUIPoint CloseButton;    // +0x10
	TGUIPoint MinimizeButton; // +0x18
	TGUIPoint UpButton;       // +0x20
	TRect Title;              // +0x28, the title's area (TSubWindow::SetTitle, DialogWindow.cpp)
	TFont TitleFont;          // +0x38
	int MinimizedHeight;      // +0x4c, the height of a minimized window (TSubWindow::SetSize)
	TRect Body;               // +0x50, the body's area, also passed to FitToChild (0x4882c0)
	TRect Resizer1;           // +0x60, the area of the first resizer (TSubWindow::SetSize)
	TRect Resizer2;           // +0x70, the area of the second resizer
	TRGBColor IconHighlight;  // +0x80, the highlight colour of a container's icon (TObjectIcon)
};

// GUIStyle+0x288 (0x5cd250); compiler-generated constructor 0x496180.
// The inventory's view (TInventoryView, ContentWindows.cpp).
struct TInventoryViewProperties {
	int Width;                          // +0x00
	int Height;                         // +0x04
	TGUIPoint MinimiseButton;           // +0x08
	std::array<TGUIPoint, 10> Objects;  // +0x10, the objects of the ten body slots (ContentWindows.cpp)
	std::array<TRect, 10> Slots;        // +0x60, the ten slot areas
	std::array<int, 10> EmptySlotSkins; // +0x100, the skins of the empty slots
	TRect Capacity;                     // +0x128, the area of the capacity (TCapacityLabel)
	TRect Soul;                         // +0x138, the area of the soul points (TSoulLabel)
	TRect States;                       // +0x148, the area of the player states (TPlayerStates)
};

// GUIStyle+0x3e0, +0x428 (0x5cd3a8, 0x5cd3f0); compiler-generated constructor 0x495590.
// The capacity and the soul points of the inventory (TCapacityLabel and TSoulLabel, ContentWindows.cpp).
struct TSkinProperties3E0 {
	TRect Caption;     // +0x00, the caption ("Cap:", "Soul:"): only x and y are read
	TFont CaptionFont; // +0x10
	TRect Value;       // +0x24, the number: only x and y are read
	TFont ValueFont;   // +0x34
};

// GUIStyle+0x5a0 (0x5cd568); compiler-generated constructor 0x495610.
struct TStatusWindowProperties {
	int Height;           // +0x00
	TRect Pictures;       // +0x04, x, y: the position of skin 119; width, height: that of skin 122
	TRect HitpointsBar;   // +0x14
	TRect ManaBar;        // +0x24
	TRect HitpointsValue; // +0x34, the number of hitpoints: only x and y are read
	TRect ManaValue;      // +0x44, the number of mana points
	TFont ValueFont;      // +0x54
};

// GUIStyle+0x688 (0x5cd650); compiler-generated constructor 0x495680.
// A row of the battle window (TBattleWindowBody, ContentWindows.cpp).
struct TBattleWindowProperties {
	int RowHeight;                 // +0x00
	TRect Outfit;                  // +0x04, the creature's outfit: its position, and its width as the size
	TRect Name;                    // +0x14, the name: its position, and its width as the longest name
	int FlagOffset;                // +0x24, the name moves right by this for each picture before it (party shield, skull)
	TRect HealthBar;               // +0x28
	TRect Frame;                   // +0x38, the frame of the highlighted, attacked or followed creature
	TRGBColor HealthBarFrameColor; // +0x48
	TFont NameFont;                // +0x54
	TRGBColor OutfitHighlight;     // +0x68, the highlight colour passed to DrawOutfit
};

// GUIStyle+0x6fc (0x5cd6c4); compiler-generated constructor 0x495700.
// The trade window (TTradeWindowBody and its parts, ContentWindows.cpp).
struct TTradeWindowProperties {
	int MinimumHeight;            // +0x00
	TRect Names;                  // +0x04, the names' area in the trade window's body (TTradeWindowBody::SetSize, ContentWindows.cpp)
	TRect OwnName;                // +0x14
	TRect CounterName;            // +0x24
	TRect NamesSeparator;         // +0x34, the area of the names' separator (TTradeNames::SetSize)
	TFont NameFont;               // +0x44
	TRect Offers;                 // +0x58, the offers' area in the body
	TRect OwnOffer;               // +0x68, the own offer's area in the offers (TTradeOffers::Draw)
	TRect CounterOffer;           // +0x78, the partner's
	TRect ObjectOffsets;          // +0x88, an object's offset in its slot: x, y for the own offer; width, height for the partner's
	int ObjectSize;               // +0x98
	int RowHeight;                // +0x9c
	int ColumnWidth;              // +0xa0
	int SlotsPerRow;              // +0xa4
	TRect OffersSeparator;        // +0xa8, the area of the offers' separator (TTradeOffers::SetSize)
	TRect Buttons;                // +0xb8, the buttons' area in the body
	TRect AcceptAndCancelButtons; // +0xc8, x, y: the position of "Accept"; width, height: that of "Cancel" (TTradeButtons::SetSize)
	TRect WaitLabel;              // +0xd8
	TRect ButtonsSeparator;       // +0xe8, the area of the buttons' separator (TTradeButtons::SetSize)
	TFont WaitFont;               // +0xf8
};

// GUIStyle+0x808 (0x5cd7d0); compiler-generated constructor 0x495870.
struct TBuddylistWindowProperties {
	int Data00;             // +0x00
	int RowHeight;          // +0x04
	TRect Logo;             // +0x08
	TRect Name;             // +0x18, the name: its position, and its width as the longest name
	TFont NameFont;         // +0x28
	TRGBColor OnlineColor;  // +0x3c, an online buddy
	TRGBColor OfflineColor; // +0x48, an offline buddy
	TRGBColor NewColor;     // +0x54, a buddy who just came online
};

// GUIStyle+0x88c (0x5cd854); compiler-generated constructor 0x4958e0.
struct TButtonsProperties {
	TFont Font;         // +0x00, the text of a button (TTextButton::Draw, DialogElements.cpp)
	TFont DisabledFont; // +0x14
	int PressedOffset;  // +0x28, the text moves by this while the button is pressed
	TGUIPoint Texture;  // +0x2c
};

// GUIStyle+0x8c4 (0x5cd88c); compiler-generated constructor 0x495950.
struct TSkinProperties8C4 {
	TFont Data00; // +0x00
	TFont Data14; // +0x14
	TFont Data28; // +0x28
};

// GUIStyle+0x900 (0x5cd8c8); compiler-generated constructor 0x4959e0.
struct TCheckBoxProperties {
	TGUIPoint Item;     // +0x00
	TRect Label;        // +0x08, the text's area (DialogElements.cpp's TCheckBox)
	TFont Font;         // +0x18
	TFont DisabledFont; // +0x2c
};

// GUIStyle+0x944 (0x5cd90c); compiler-generated constructor 0x495a60.
// The edit lines (TEditLine and TEditText, DialogElements.cpp).
struct TEditLineProperties {
	TRGBColor Background;         // +0x00
	TRGBColor DisabledBackground; // +0x0c
	TRGBColor CursorColor;        // +0x18
	TFont Font;                   // +0x24
	TFont ReadOnlyFont;           // +0x38
	TRGBColor SelectionColor;     // +0x4c
};

// GUIStyle+0x9a0 (0x5cd968); compiler-generated constructor 0x495af0.
// The list boxes (TListBox and TFontListBox, DialogElements.cpp).
struct TListBoxProperties {
	TRGBColor Background;         // +0x00
	TFont Font;                   // +0x0c
	int TextMargin;               // +0x20
	TRGBColor SelectedBackground; // +0x24
	TFont SelectedFont;           // +0x30
};

// GUIStyle+0x9e8 (0x5cd9b0); compiler-generated constructor 0x495b70 (it
// zeroes +0x04-+0xa7, so the types are open; these are ConsoleWindow.cpp's).
// The console: its minimum height, the areas of the tab bar, the edit line
// and the text, the button positions, the text window's area in its TConsole,
// the selection colour and the font of the channels other than the default.
struct TConsoleWindowProperties {
	int MinimumHeight;              // +0x00, the console's minimum height
	TRect TabBar;                   // +0x04, the tab bar (SetChildArea)
	TRect Data14;                   // +0x14, not used by ConsoleWindow.cpp
	TRect Input;                    // +0x24, the edit line
	TRect Data34;                   // +0x34, not used by ConsoleWindow.cpp
	TRect CurrentConsole;           // +0x44, the current channel (TConsole)
	TGUIPoint MoveRightButton;      // +0x54, the button "Move channels to the right"
	TGUIPoint MoveLeftButton;       // +0x5c, "Move channels to the left"
	TGUIPoint VolumeButton;         // +0x64, "Adjust volume"
	TGUIPoint CloseButton;          // +0x6c, "Close this Channel"
	TGUIPoint ServerMessagesButton; // +0x74, the server messages button
	TGUIPoint IgnoreButton;         // +0x7c, the ignore button
	TGUIPoint NewChannelButton;     // +0x84, "Open new channel"
	TRect TextWindow;               // +0x8c, the text window in its TConsole
	TRGBColor SelectionColor;       // +0x9c, the selection
	TFont InputFont;                // +0xa8, the edit line's font in a channel that accepts no messages (TConsoleWindow, ConsoleWindow.cpp)
};

// GUIStyle+0xaa4 (0x5cda6c); compiler-generated constructor 0x495c60.
// A tab of the console (TTab, ConsoleWindow.cpp).
struct TTabProperties {
	int Width;          // +0x00
	int Height;         // +0x04
	TGUIPoint Caption;  // +0x08
	int CaptionMargin;  // +0x10, left and right of the caption, for shortening it (TTab's constructor)
	TFont ActiveFont;   // +0x14
	TFont InactiveFont; // +0x28
	TFont UnreadFont;   // +0x3c
	TFont BlinkingFont; // +0x50, an unread tab while it blinks
};

// GUIStyle+0xb08 (0x5cdad0); compiler-generated constructor 0x4962c0.
// The frame of a dialog (TDialog, Dialogs.cpp). Data64 and Data78 are the fonts of the dialogs' texts and labels;
// they hold the same values and the code does not show how they differ.
struct TDialogProperties {
	int MinimumWidth;                 // +0x00
	int MinimumHeight;                // +0x04
	TRect Caption;                    // +0x08
	TFont CaptionFont;                // +0x18
	std::array<TGUIPoint, 3> Buttons; // +0x2c, the buttons from the right
	TRect Separator;                  // +0x44
	TRect Body;                       // +0x54, FitToChild takes it by value
	TFont Data64;                     // +0x64
	TFont Data78;                     // +0x78
};

// GUIStyle+0xd3c (0x5cdd04); compiler-generated constructor 0x495d20.
// The ignore list dialog (TIgnoreListDialog, Dialogs.cpp).
struct TIgnoreListDialogProperties {
	int Width;                    // +0x00
	int Height;                   // +0x04
	TRect IgnoreListCheckBox;     // +0x08
	TRect IgnoreListLabel;        // +0x18
	TRect IgnoreList;             // +0x28
	TGUIPoint AddIgnoreButton;    // +0x38
	TGUIPoint DeleteIgnoreButton; // +0x40
	TRect IgnorePermanently;      // +0x48
	TRect GlobalIgnoreLabel;      // +0x58
	TRect IgnoreYelling;          // +0x68
	TRect IgnorePrivate;          // +0x78
	TRGBColor PermanentColor;     // +0x88, the lines of the permanent entries
	TRect WhiteListCheckBox;      // +0x94
	TRect WhiteListLabel;         // +0xa4
	TRect WhiteList;              // +0xb4
	TGUIPoint AddWhiteButton;     // +0xc4
	TGUIPoint DeleteWhiteButton;  // +0xcc
	TRect AllowPermanently;       // +0xd4
	TRect GlobalAllowLabel;       // +0xe4
	TRect AllowBuddies;           // +0xf4
};

// GUIStyle+0xe68 (0x5cde30); compiler-generated constructor 0x495e50.
// The info dialog (CreateInfoDialog, Dialogs.cpp).
struct TInfoDialogProperties {
	int Width;       // +0x00
	int Height;      // +0x04
	TFont LabelFont; // +0x08
	TFont LinkFont;  // +0x1c
	int Border;      // +0x30
	TRect Label;     // +0x34
	TRect Link;      // +0x44
	TRect Separator; // +0x54
};

// GUIStyle+0xf8c (0x5cdf54); compiler-generated constructor 0x4963a0.
// The dialog that edits a VIP (TEditBuddyDialog, Dialogs.cpp).
struct TEditBuddyDialogProperties {
	int Width;                             // +0x00
	int Height;                            // +0x04
	TRect Name;                            // +0x08
	TRect Separator;                       // +0x18
	TRect TypeLabel;                       // +0x28
	std::array<TGUIPoint, 11> IconButtons; // +0x38
	TRect DescriptionLabel;                // +0x90
	TRect Description;                     // +0xa0
	TRect Notify;                          // +0xb0
};

// GUIStyle+0x1084 (0x5ce04c); compiler-generated constructor 0x496430.
// The help dialog (THelpDialog, Dialogs.cpp).
struct THelpDialogProperties {
	int Width;                     // +0x00
	int Height;                    // +0x04
	TGUIPoint ClientHelpButton;    // +0x08
	TGUIPoint HintsButton;         // +0x10
	TGUIPoint TutorHelpButton;     // +0x18
	TGUIPoint RuleViolationButton; // +0x20
	TGUIPoint ManualButton;        // +0x28
	TGUIPoint FAQButton;           // +0x30
	TGUIPoint ClientHelpLabel;     // +0x38
	TGUIPoint HintsLabel;          // +0x40
	TGUIPoint TutorHelpLabel;      // +0x48
	TGUIPoint RuleViolationLabel;  // +0x50
	TGUIPoint ManualLabel;         // +0x58
	TGUIPoint FAQLabel;            // +0x60
};

// GUIStyle+0x1114 (0x5ce0dc); compiler-generated constructor 0x495ee0.
// The rule violation dialog (TRuleViolationDialog, Dialogs.cpp).
struct TRuleViolationDialogProperties {
	int Width;            // +0x00
	int Height;           // +0x04
	TRect NameLabel;      // +0x08
	TRect Name;           // +0x18
	TRect ReasonLabel;    // +0x28
	TRect ReasonList;     // +0x38
	TRect ActionLabel;    // +0x48
	TRect ActionList;     // +0x58
	TRect CommentLabel;   // +0x68
	TRect Comment;        // +0x78
	TRect StatementLabel; // +0x88
	TRect Statement;      // +0x98
	TRect IPBanishment;   // +0xa8
};

// GUIStyle+0x1274 (0x5ce23c); compiler-generated constructor 0x4964a0.
// The options dialog (TOptionsDialog, Dialogs.cpp).
struct TOptionsDialogProperties {
	int Width;                // +0x00
	int Height;               // +0x04
	TGUIPoint GeneralButton;  // +0x08
	TGUIPoint GraphicsButton; // +0x10
	TGUIPoint ConsoleButton;  // +0x18
	TGUIPoint NetworkButton;  // +0x20
	TGUIPoint HotkeysButton;  // +0x28
	TGUIPoint MotdButton;     // +0x30
	TGUIPoint GeneralLabel;   // +0x38
	TGUIPoint GraphicsLabel;  // +0x40
	TGUIPoint ConsoleLabel;   // +0x48
	TGUIPoint NetworkLabel;   // +0x50
	TGUIPoint HotkeysLabel;   // +0x58
	TGUIPoint MotdLabel;      // +0x60
	TRect Separator;          // +0x68
};

// GUIStyle+0x1394 (0x5ce35c); compiler-generated constructor 0x495fa0.
// The advanced graphics dialog (TAdvancedGraphicsDialog, Dialogs.cpp).
struct TAdvancedGraphicsDialogProperties {
	int Width;                      // +0x00
	int Height;                     // +0x04
	TGUIPoint DeviceLabel;          // +0x08
	TGUIPoint DirectX5Button;       // +0x10
	TGUIPoint DirectX9Button;       // +0x18
	TGUIPoint OpenGLButton;         // +0x20
	TRect LightEffects;             // +0x28
	TRect NoStretching;             // +0x38
	TRect WindowsMousepointer;      // +0x48
	TRect Framerate;                // +0x58
	TGUIPoint FramerateLimitText;   // +0x68
	TGUIPoint CurrentFramerateText; // +0x70
};

// GUIStyle+0x470 (0x5cd438), PlayerStates in ContentWindows.cpp's assertion
// ("SkinProperties.PlayerStates.NumberOfIcons"); its constructor is inlined
// into SkinProperties' (the extent of the block is a guess: the members
// TPlayerStates reads). LargeRow and SmallRow hold the first icon's position
// in the large and the small row, IconDistance the distance between the icons.
struct TPlayerStatesProperties {
	TRect LargeRow;    // +0x00
	TRect SmallRow;    // +0x10
	int NumberOfIcons; // +0x20
	int IconDistance;  // +0x24
};

// GUIStyle+0x4ac and +0x4fc (0x5cd474, 0x5cd4c4): the layout of the large and
// the small combat controls (ContentWindows.cpp copies a whole block by
// value): their height and width and the positions of the offensive,
// balanced, defensive, stand, chase, secure, stop, options and help buttons.
// The type and member names are ours.
struct TCombatControlsProperties {
	int Height;                       // +0x00
	int Width;                        // +0x04
	std::array<TGUIPoint, 9> Buttons; // +0x08
};

struct TSkinProperties {
	TGUIPoint CursorHotspot1;                                 // +0x000
	TGUIPoint CursorHotspot2;                                 // +0x008
	TGUIPoint CursorHotspot3;                                 // +0x010
	TGUIPoint CursorHotspot4;                                 // +0x018
	TGUIPoint CursorHotspot5;                                 // +0x020
	TGUIPoint CursorHotspot6;                                 // +0x028
	TFont TooltipFont;                                        // +0x030
	int TooltipBorder;                                        // +0x044
	TRGBColor TooltipBackground;                              // +0x048
	TRGBColor TooltipFrame;                                   // +0x054
	TRGBColor HighlightColor;                                 // +0x060
	TRGBColor AttackColor;                                    // +0x06c
	TRGBColor FollowColor;                                    // +0x078
	TRGBColor HighlightedAttackColor;                         // +0x084
	TRGBColor HighlightedFollowColor;                         // +0x090
	int MenuShortcutGap;                                      // +0x09c, the gap between a menu entry's text and its shortcut (ContextMenus.cpp)
	TRect MenuMargins;                                        // +0x0a0, a context menu's margins around its entries (FitToChildren, ContextMenus.cpp)
	TFont MenuFont;                                           // +0x0b0, the font of the menu entries (number and colour; ContextMenus.cpp)
	int MenuSeparatorHeight;                                  // +0x0c4, the height of a menu separator
	TRGBColor MenuHighlightBackground;                        // +0x0c8, the background of the menu entry under the mouse
	int MenuTextMargin;                                       // +0x0d4, the space around a menu entry's text
	TStartWindowProperties StartWindow;                       // +0x0d8, 0x5c bytes
	int StartWindowBorder;                                    // +0x134, the border of the start window's frame
	int SplitterHeight;                                       // +0x138, the height of MainWindow.cpp's splitter
	int MapFrameBorder;                                       // +0x13c, the border of the map's frame (TMapFrame, MapWindow.cpp)
	TRect MapHintButton;                                      // +0x140, the area of the map window's hint button (TMapWindow::Arrange)
	int HelpBubbleMinimumWidth;                               // +0x150, the help bubble's minimum width and height (ContextMenus.cpp)
	int HelpBubbleMinimumHeight;                              // +0x154
	int HelpBubbleMaximumWidth;                               // +0x158, its maximum width and height
	int HelpBubbleMaximumHeight;                              // +0x15c
	TRect HelpBubbleTextMargins;                              // +0x160, the margins around its text (one 16-byte block passed by value)
	int HelpBubbleSizeStep;                                   // +0x170, its sizes are HelpBubbleSizeRemainder modulo HelpBubbleSizeStep
	int HelpBubbleSizeRemainder;                              // +0x174
	int HelpBubbleMargin;                                     // +0x178, its distance to the help area and the window edges
	int FormattedMessageWidth;                                // +0x17c, the text's size in a formatted message dialog (CreateFormattedMessageDialog, Dialogs.cpp)
	int FormattedMessageHeight;                               // +0x180
	int SideBarWidth;                                         // +0x184, the width of the side bar (MainWindow.cpp)
	TRGBColor DraggedWindowPlaceColor;                        // +0x188, the place of the sub window being dragged (TDialogWindow::Draw)
	TSubWindowProperties SubWindow;                           // +0x194, 0x8c bytes
	int AutomapWindowHeight;                                  // +0x220
	TRect AutomapFrame;                                       // +0x224, the area whose edges the automap's frame follows (TAutomapWindow::Arrange, ContentWindows.cpp)
	TRect AutomapView;                                        // +0x234
	TRect AutomapZoomButtons;                                 // +0x244, x, y: the position of zoom in; width, height: that of zoom out
	TRect AutomapFloorButtons;                                // +0x254, x, y: the position of floor up; width, height: that of floor down
	TRect AutomapCentreButtonAndCompassRose;                  // +0x264, x, y: the position of "Centre"; width, height: that of the compass rose
	int InventoryWindowHeight;                                // +0x274
	TRect InventoryControlsAndView;                           // +0x278, x, y: the position of the combat controls; width, height: that of the inventory view
	TInventoryViewProperties InventoryView;                   // +0x288, 0x158 bytes
	TSkinProperties3E0 CapacityLabel;                         // +0x3e0, 0x48 bytes, the capacity (TCapacityLabel, ContentWindows.cpp)
	TSkinProperties3E0 SoulLabel;                             // +0x428, 0x48 bytes, the soul points (TSoulLabel)
	TPlayerStatesProperties PlayerStates;                     // +0x470, 0x28 bytes
	TFont ObjectCountFont;                                    // +0x498, the count of an object (DrawObject)
	TCombatControlsProperties LargeCombatControls;            // +0x4ac, the large combat controls (ContentWindows.cpp)
	TCombatControlsProperties SmallCombatControls;            // +0x4fc, the small ones
	int SmallInventoryWindowHeight;                           // +0x54c
	TRect SmallInventoryControlsAndMaximise;                  // +0x550, x, y: the position of the combat controls; width, height: that of the maximise button
	TRect SmallInventoryCapacity;                             // +0x560
	TRect SmallInventorySoul;                                 // +0x570
	TRect SmallInventoryStates;                               // +0x580
	TRect SmallInventoryDropArea;                             // +0x590, where a dragged object is dropped on the small inventory (skin 99)
	TStatusWindowProperties StatusWindow;                     // +0x5a0, 0x68 bytes
	int ButtonsWindowHeight;                                  // +0x608
	TRect ButtonsWindowSkillsAndBattle;                       // +0x60c, x, y: the position of "Skills"; width, height: that of "Battle"
	TRect ButtonsWindowBuddyAndLogout;                        // +0x61c, x, y: the position of "VIP"; width, height: that of "Logout"
	int ContainerSlotsPerRow;                                 // +0x62c
	TGUIPoint ContainerFirstSlot;                             // +0x630
	TRect ContainerSlotSizeAndObject;                         // +0x638, x, y: a slot's width and height; width, height: the object's offset in the row and column
	int ContainerColumnWidth;                                 // +0x648
	int ContainerRowHeight;                                   // +0x64c
	int ContainerMinimumHeight;                               // +0x650
	TFont SkillsFont;                                         // +0x654
	int SkillsHorizontalMargin;                               // +0x668
	int SkillsVerticalMargin;                                 // +0x66c
	TRGBColor SkillsFirstBarsColor;                           // +0x670, the bars of the first seven skill lines (ContentWindows.cpp)
	TRGBColor SkillsOtherBarsColor;                           // +0x67c, the bars of the skills
	TBattleWindowProperties BattleWindow;                     // +0x688, 0x74 bytes
	TTradeWindowProperties TradeWindow;                       // +0x6fc, 0x10c bytes
	TBuddylistWindowProperties BuddylistWindow;               // +0x808, 0x60 bytes
	int Data868;                                              // +0x868
	int ScrollBarRangeMinimumHeight;                          // +0x86c, the vertical scroll bar (DialogElements.h): its range's minimum height
	int ScrollBarMinimumWidth;                                // +0x870, its and its range's minimum width
	int ScrollBarMinimumHeight;                               // +0x874
	int ScrollBarWidth;                                       // +0x878
	int HorizontalScrollBarRangeMinimumWidth;                 // +0x87c, the horizontal scroll bar: its range's minimum width
	int HorizontalScrollBarHeight;                            // +0x880
	int HorizontalScrollBarMinimumWidth;                      // +0x884
	int Data888;                                              // +0x888
	TButtonsProperties Buttons;                               // +0x88c, 0x34 bytes
	int BlinkingButtonInterval;                               // +0x8c0, milliseconds between a blinking button's changes (TBlinkingButton)
	TSkinProperties8C4 Style8C4;                              // +0x8c4, 0x3c bytes
	TCheckBoxProperties CheckBox;                             // +0x900, 0x40 bytes
	int ObjectIconFrameWidth;                                 // +0x940, the frame of a framed object icon (TObjectIcon::Draw)
	TEditLineProperties EditLine;                             // +0x944, 0x58 bytes
	int EditLineTextMargin;                                   // +0x99c
	TListBoxProperties ListBox;                               // +0x9a0, 0x44 bytes
	int ListBoxSelectionMargin;                               // +0x9e4, the selected line's background is larger than its text by this
	TConsoleWindowProperties ConsoleWindow;                   // +0x9e8, 0xbc bytes
	TTabProperties Tab;                                       // +0xaa4, 0x64 bytes
	TDialogProperties Dialog;                                 // +0xb08, 0x8c bytes
	int LoginDialogWidth;                                     // +0xb94
	int LoginDialogHeight;                                    // +0xb98
	TRect LoginDialogAccountLabel;                            // +0xb9c
	TRect LoginDialogAccount;                                 // +0xbac, the login dialog's account number
	TRect LoginDialogPasswordLabel;                           // +0xbbc
	TRect LoginDialogPassword;                                // +0xbcc, its password
	TRect LoginDialogLink;                                    // +0xbdc, its link
	int ReconnectWindowSpacing;                               // +0xbec, between the reconnect window's text, bar and time (TReconnectWindow)
	int CharacterListDialogWidth;                             // +0xbf0
	int CharacterListDialogHeight;                            // +0xbf4
	TRect CharacterListDialogLabel;                           // +0xbf8, the character list's label
	TRect CharacterListDialogList;                            // +0xc08, its list
	TRect CharacterListDialogStatus;                          // +0xc18, its account status
	int MoveObjectsDialogWidth;                               // +0xc28
	int MoveObjectsDialogHeight;                              // +0xc2c
	TRect MoveObjectsDialogScrollBar;                         // +0xc30, the move dialog's scroll bar
	TRect MoveObjectsDialogLabel;                             // +0xc40, its label
	TGUIPoint MoveObjectsDialogObject;                        // +0xc50
	int OutfitDialogWidth;                                    // +0xc58
	int OutfitDialogHeight;                                   // +0xc5c
	TRect OutfitDialogIcon;                                   // +0xc60, the area of the outfit with its border
	int OutfitDialogIconBorder;                               // +0xc70
	TRGBColor OutfitDialogIconHighlight;                      // +0xc74
	TGUIPoint OutfitDialogNextButton;                         // +0xc80
	std::array<TGUIPoint, 4> OutfitDialogPartButtons;         // +0xc88, the outfit dialog's part buttons
	TGUIPoint OutfitDialogColorButtons;                       // +0xca8, the position of its first colour button
	int OutfitDialogColorColumnWidth;                         // +0xcb0
	int OutfitDialogColorRowHeight;                           // +0xcb4
	TGUIPoint OutfitDialogLabel;                              // +0xcb8
	int TextDialogWidth;                                      // +0xcc0
	int TextDialogHeight;                                     // +0xcc4
	TRect TextDialogObjectIcon;                               // +0xcc8
	TRGBColor TextDialogObjectHighlight;                      // +0xcd8
	TRect TextDialogDescription;                              // +0xce4, the text dialog's description
	TRect TextDialogText;                                     // +0xcf4, its text
	int EditListDialogWidth;                                  // +0xd04
	int EditListDialogHeight;                                 // +0xd08
	TRect EditListDialogPicture;                              // +0xd0c, x, y: the position of the edit list's picture (skin 217)
	TRect EditListDialogDescription;                          // +0xd1c, the edit list's description
	TRect EditListDialogText;                                 // +0xd2c, its text
	TIgnoreListDialogProperties IgnoreListDialog;             // +0xd3c, 0x104 bytes
	int BugReportDialogWidth;                                 // +0xe40
	int BugReportDialogHeight;                                // +0xe44
	TRect BugReportDialogText;                                // +0xe48, the bug report's text
	TRect BugReportDialogLabel;                               // +0xe58, its label
	TInfoDialogProperties InfoDialog;                         // +0xe68, 0x64 bytes
	int NewChannelDialogWidth;                                // +0xecc
	int NewChannelDialogHeight;                               // +0xed0
	TRect NewChannelDialogListLabel;                          // +0xed4, the new channel dialog's labels and fields
	TRect NewChannelDialogList;                               // +0xee4
	TRect NewChannelDialogNameLabel;                          // +0xef4
	TRect NewChannelDialogName;                               // +0xf04
	int PrivateChannelDialogWidth;                            // +0xf14
	int PrivateChannelDialogHeight;                           // +0xf18
	TRect PrivateChannelDialogName;                           // +0xf1c, the private channel dialog's name
	TRect PrivateChannelDialogLabel;                          // +0xf2c, its label
	int ReportDialogWidth;                                    // +0xf3c
	int ReportDialogHeight;                                   // +0xf40
	TRect ReportDialogText;                                   // +0xf44, the report dialog's text
	TRect ReportDialogLabel;                                  // +0xf54, its label
	int AddBuddyDialogWidth;                                  // +0xf64
	int AddBuddyDialogHeight;                                 // +0xf68
	TRect AddBuddyDialogName;                                 // +0xf6c, the VIP dialog's name
	TRect AddBuddyDialogLabel;                                // +0xf7c, its label
	TEditBuddyDialogProperties EditBuddyDialog;               // +0xf8c, 0xc0 bytes
	int Data104C;                                             // +0x104c
	int Data1050;                                             // +0x1050
	TRect Data1054;                                           // +0x1054, the update dialog's label
	TRect Data1064;                                           // +0x1064, zeroed, not used
	TRect Data1074;                                           // +0x1074, its progress bar
	THelpDialogProperties HelpDialog;                         // +0x1084, 0x68 bytes
	int HintsDialogWidth;                                     // +0x10ec
	int HintsDialogHeight;                                    // +0x10f0
	TRect HintsDialogLabel;                                   // +0x10f4, the hints dialog's label
	TRect HintsDialogList;                                    // +0x1104, its list
	TRuleViolationDialogProperties RuleViolationDialog;       // +0x1114, 0xb8 bytes
	int EditMarkDialogWidth;                                  // +0x11cc
	int EditMarkDialogHeight;                                 // +0x11d0
	TRect EditMarkDialogDescriptionLabel;                     // +0x11d4, the mark dialog's labels and text
	TRect EditMarkDialogDescription;                          // +0x11e4
	TRect EditMarkDialogTypeLabel;                            // +0x11f4
	std::array<TGUIPoint, 14> EditMarkDialogTypeButtons;      // +0x1204
	TOptionsDialogProperties OptionsDialog;                   // +0x1274, 0x78 bytes
	int GeneralOptionsDialogWidth;                            // +0x12ec
	int GeneralOptionsDialogHeight;                           // +0x12f0
	TRect GeneralOptionsDialogClassicControl;                 // +0x12f4, the general options' check boxes
	TRect GeneralOptionsDialogAutoChaseOff;                   // +0x1304
	TRect GeneralOptionsDialogShowHints;                      // +0x1314
	TRect GeneralOptionsDialogTextualEffects;                 // +0x1324
	TRect GeneralOptionsDialogCreatureNames;                  // +0x1334
	int GraphicsOptionsDialogWidth;                           // +0x1344
	int GraphicsOptionsDialogHeight;                          // +0x1348
	TRect GraphicsOptionsDialogFullscreen;                    // +0x134c, the graphics options' controls
	TRect GraphicsOptionsDialogResolutionList;                // +0x135c
	TRect GraphicsOptionsDialogResolutionLabel;               // +0x136c
	TGUIPoint GraphicsOptionsDialogAdvancedButton;            // +0x137c
	TRect GraphicsOptionsDialogAdvancedLabel;                 // +0x1384
	TAdvancedGraphicsDialogProperties AdvancedGraphicsDialog; // +0x1394, 0x78 bytes
	int ConsoleOptionsDialogWidth;                            // +0x140c
	int ConsoleOptionsDialogHeight;                           // +0x1410
	TRect ConsoleOptionsDialogInfoMessages;                   // +0x1414, the console options' check boxes
	TRect ConsoleOptionsDialogEventMessages;                  // +0x1424
	TRect ConsoleOptionsDialogStatusMessages;                 // +0x1434
	TRect ConsoleOptionsDialogTimeStamps;                     // +0x1444
	TRect ConsoleOptionsDialogPrivateMessages;                // +0x1454
	int NetworkOptionsDialogWidth;                            // +0x1464
	int NetworkOptionsDialogHeight;                           // +0x1468
	TRect NetworkOptionsDialogTestServerLogin;                // +0x146c, the network options' check box
	int HotkeyDialogWidth;                                    // +0x147c
	int HotkeyDialogHeight;                                   // +0x1480
	TRect HotkeyDialogList;                                   // +0x1484, the hotkey dialog's list
	TRect HotkeyDialogListLabel;                              // +0x1494, its labels and fields
	TRect HotkeyDialogText;                                   // +0x14a4
	TRect HotkeyDialogTextLabel;                              // +0x14b4
	TRect HotkeyDialogAutoSend;                               // +0x14c4
	TRGBColor HotkeyDialogAutoSendColor;                      // +0x14d4, the lines of hotkeys sent automatically
	int Data14E0;                                             // +0x14e0
	int Data14E4;                                             // +0x14e4
	TRect Data14E8;                                           // +0x14e8, the error report's labels and fields (set, unread since #74)
	TRect Data14F8;                                           // +0x14f8
	TRect Data1508;                                           // +0x1508
	TRect Data1518;                                           // +0x1518
};

namespace patella {

// Skins.cpp's data and GUI.cpp's SkinProperties (issue #116), TGraphics' member Skins. Public, as the globals were:
// the window files read them (through the accessors below), the golden tests set them (gui_tests.cpp constructs
// SkinProperties in place and compares it byte for byte; the window tests pass Skins to the original's side).
class TSkins {
public:
	TSkins() = default;
	TSkins(const TSkins &) = delete;
	TSkins &operator=(const TSkins &) = delete;
	TSkins(TSkins &&) = delete;
	TSkins &operator=(TSkins &&) = delete;
	~TSkins() = default;

	void InitSkins(); // 0x4e3cb0
	void ExitSkins(); // 0x4e6da0

	// Data, in .bss order (Skins.cpp's), then GUI.cpp's.
	int NumberOfSkins{};              // 0x71c450
	std::unique_ptr<TSkin[]> Skins;   // 0x71c454 (issue #58: the table's only owner, a pointer as the original's)
	TSkinProperties SkinProperties{}; // 0x5ccfc8, GUI.cpp's; filled by InitSkinProperties
};

// The transition's access path (notes/decisions.md, "#104: TClient and the subsystem objects"): the one TClient's
// TSkins (Client.cpp). It goes when no free function needs it.
namespace bridge {
TSkins &Skins();
} // namespace bridge

} // namespace patella

// The original's globals' names for their about 2,400 readers in the window files (issue #116): SkinProperties() and
// Skins() are the one TClient's, so a use reads `SkinProperties().TooltipBorder` where it read
// `SkinProperties.TooltipBorder`. Not the design's `bridge::Skins().SkinProperties` at each use: the same object,
// without rewriting every expression of the windows (notes/decisions.md, "#116").
inline TSkinProperties &SkinProperties()
{
	return patella::bridge::Skins().SkinProperties;
}

inline std::unique_ptr<TSkin[]> &Skins()
{
	return patella::bridge::Skins().Skins;
}

// The original's free functions (graphics.cpp's since issue #116: they call the bridge, which patella_core does not
// have), forwarding to TSkins' members.
void InitSkins();
void ExitSkins();

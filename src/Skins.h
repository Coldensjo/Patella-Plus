// Skins.h: the skins, rectangles of the GUI graphics (their table and the names of their numbers), and SkinProperties,
// the window graphics' positions, sizes, colours and fonts, which GUI.cpp's InitSkinProperties fills.
//
// TSkins holds both: a part of TGraphics (graphics.h), declared here and defined in Skins.cpp, as TSession is
// Status.cpp's inside TCommunication. Portable (patella_core): the bridge accessor and the free functions are the
// client's (graphics.cpp), since patella_core has no TClient.

#pragma once

#include "types/GraphicsTypes.h"
#include <array>
#include <memory>

// Number of skin entries; the terminator of the table in InitSkins.
inline constexpr int LASTSKIN = 262;

// One rectangle in the GUI picture.
struct TSkin {
	int x;
	int y;
	int width;
	int height;
};

// The skin numbers: the index into Skins (InitSkins' table, Skins.cpp), the skin of a TDrawing and of DrawSkin, the
// two skins of a TSkinButton. One name of the original is known, CONSOLETEXT_CORNER_TOPRIGHT, from the error text of
// TComponent::Draw (GUI.cpp); the frames' names follow its pattern (CORNER_ and EDGE_ with the side, BACKGROUND for
// the tiled middle). Every other name is taken from what the code that uses the skin draws with it: the pictures
// themselves were not looked at. A button's two skins are _UP (released) and _DOWN (pressed).
//
// Without a name, because no code uses them by number or shows what they are: 3 (the picture of mouse cursor 3, which
// nothing sets), 20, 21, 34-41 and 216. 152 has no entry in the table. The help texts (Help.cpp) write theirs as
// int{SKIN_...}: they are arguments of std::format, which does not take an enumerator.
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

// --- Skin properties ------------------------------------------------------

struct TGUIPoint {
	TGUIPoint() : x(0), y(0) {}
	int x, y;
};

// SkinProperties: positions, sizes, colours and fonts of the window graphics, filled in by
// InitSkinProperties. The members are named after the window or element that reads them and
// what it does with the value. Some TRects hold two positions (x, y and width, height):
// their comments say which.

// The start window's buttons (MainWindow.cpp): the size of
// their panel, its position and the positions of "Enter Game", "Options",
// "Info", "Exit Game" and "Access Account" in it.
struct TStartWindowProperties {
	int PanelWidth;
	int PanelHeight;
	TGUIPoint FramePosition;       // the position of the frame around the panel (SetChildPosition)
	TGUIPoint EnterGameButton;     // "Enter Game"
	TGUIPoint OptionsButton;       // "Options"
	TGUIPoint InfoButton;          // "Info"
	TGUIPoint ExitGameButton;      // "Exit Game"
	TGUIPoint AccessAccountButton; // "Access Account"
};

// The frame of a sub window (TSubWindow,
// DialogWindow.cpp).
struct TSubWindowProperties {
	TRect Icon; // the icon's area (TSubWindow::SetIcon)
	TGUIPoint CloseButton;
	TGUIPoint MinimizeButton;
	TGUIPoint UpButton;
	TRect Title; // the title's area (TSubWindow::SetTitle, DialogWindow.cpp)
	TFont TitleFont;
	int MinimizedHeight;     // the height of a minimized window (TSubWindow::SetSize)
	TRect Body;              // the body's area, also passed to FitToChild
	TRect Resizer1;          // the area of the first resizer (TSubWindow::SetSize)
	TRect Resizer2;          // the area of the second resizer
	TRGBColor IconHighlight; // the highlight colour of a container's icon (TObjectIcon)
};

// The inventory's view (TInventoryView,
// ContentWindows.cpp).
struct TInventoryViewProperties {
	int Width;
	int Height;
	TGUIPoint MinimiseButton;
	std::array<TGUIPoint, 10> Objects;  // the objects of the ten body slots (ContentWindows.cpp)
	std::array<TRect, 10> Slots;        // the ten slot areas
	std::array<int, 10> EmptySlotSkins; // the skins of the empty slots
	TRect Capacity;                     // the area of the capacity (TCapacityLabel)
	TRect Soul;                         // the area of the soul points (TSoulLabel)
	TRect States;                       // the area of the player states (TPlayerStates)
};

// The capacity and the soul points of the inventory (TCapacityLabel and
// TSoulLabel, ContentWindows.cpp).
struct TInventoryLabelProperties {
	TRect Caption; // the caption ("Cap:", "Soul:"): only x and y are read
	TFont CaptionFont;
	TRect Value; // the number: only x and y are read
	TFont ValueFont;
};

struct TStatusWindowProperties {
	int Height;
	TRect Pictures; // x, y: the position of skin 119; width, height: that of skin 122
	TRect HitpointsBar;
	TRect ManaBar;
	TRect HitpointsValue; // the number of hitpoints: only x and y are read
	TRect ManaValue;      // the number of mana points
	TFont ValueFont;
};

// A row of the battle window (TBattleWindowBody,
// ContentWindows.cpp).
struct TBattleWindowProperties {
	int RowHeight;
	TRect Outfit;   // the creature's outfit: its position, and its width as the size
	TRect Name;     // the name: its position, and its width as the longest name
	int FlagOffset; // the name moves right by this for each picture before it (party shield, skull)
	TRect HealthBar;
	TRect Frame; // the frame of the highlighted, attacked or followed creature
	TRGBColor HealthBarFrameColor;
	TFont NameFont;
	TRGBColor OutfitHighlight; // the highlight colour passed to DrawOutfit
};

// The trade window (TTradeWindowBody and its parts,
// ContentWindows.cpp).
struct TTradeWindowProperties {
	int MinimumHeight;
	TRect Names; // the names' area in the trade window's body (TTradeWindowBody::SetSize, ContentWindows.cpp)
	TRect OwnName;
	TRect CounterName;
	TRect NamesSeparator; // the area of the names' separator (TTradeNames::SetSize)
	TFont NameFont;
	TRect Offers;        // the offers' area in the body
	TRect OwnOffer;      // the own offer's area in the offers (TTradeOffers::Draw)
	TRect CounterOffer;  // the partner's
	TRect ObjectOffsets; // an object's offset in its slot: x, y for the own offer; width, height for the partner's
	int ObjectSize;
	int RowHeight;
	int ColumnWidth;
	int SlotsPerRow;
	TRect OffersSeparator;        // the area of the offers' separator (TTradeOffers::SetSize)
	TRect Buttons;                // the buttons' area in the body
	TRect AcceptAndCancelButtons; // x, y: the position of "Accept"; width, height: that of "Cancel" (TTradeButtons::SetSize)
	TRect WaitLabel;
	TRect ButtonsSeparator; // the area of the buttons' separator (TTradeButtons::SetSize)
	TFont WaitFont;
};

struct TBuddylistWindowProperties {
	int RowHeight;
	TRect Logo;
	TRect Name; // the name: its position, and its width as the longest name
	TFont NameFont;
	TRGBColor OnlineColor;  // an online buddy
	TRGBColor OfflineColor; // an offline buddy
	TRGBColor NewColor;     // a buddy who just came online
};

struct TButtonsProperties {
	TFont Font; // the text of a button (TTextButton::Draw, DialogElements.cpp)
	TFont DisabledFont;
	int PressedOffset; // the text moves by this while the button is pressed
	TGUIPoint Texture;
};

struct TCheckBoxProperties {
	TGUIPoint Item;
	TRect Label; // the text's area (DialogElements.cpp's TCheckBox)
	TFont Font;
	TFont DisabledFont;
};

// The edit lines (TEditLine and TEditText,
// DialogElements.cpp).
struct TEditLineProperties {
	TRGBColor Background;
	TRGBColor DisabledBackground;
	TRGBColor CursorColor;
	TFont Font;
	TFont ReadOnlyFont;
	TRGBColor SelectionColor;
};

// The list boxes (TListBox and TFontListBox,
// DialogElements.cpp).
struct TListBoxProperties {
	TRGBColor Background;
	TFont Font;
	int TextMargin;
	TRGBColor SelectedBackground;
	TFont SelectedFont;
};

// The console: its minimum height, the areas
// of the tab bar, the edit line and the text, the button positions, the text
// window's area in its TConsole, the selection colour and the font of the
// channels other than the default.
struct TConsoleWindowProperties {
	int MinimumHeight;              // the console's minimum height
	TRect TabBar;                   // the tab bar (SetChildArea)
	TRect Input;                    // the edit line
	TRect CurrentConsole;           // the current channel (TConsole)
	TGUIPoint MoveRightButton;      // the button "Move channels to the right"
	TGUIPoint MoveLeftButton;       // "Move channels to the left"
	TGUIPoint VolumeButton;         // "Adjust volume"
	TGUIPoint CloseButton;          // "Close this Channel"
	TGUIPoint ServerMessagesButton; // the server messages button
	TGUIPoint IgnoreButton;         // the ignore button
	TGUIPoint NewChannelButton;     // "Open new channel"
	TRect TextWindow;               // the text window in its TConsole
	TRGBColor SelectionColor;       // the selection
	TFont InputFont;                // the edit line's font in a channel that accepts no messages (TConsoleWindow, ConsoleWindow.cpp)
};

// A tab of the console (TTab, ConsoleWindow.cpp).
struct TTabProperties {
	int Width;
	int Height;
	TGUIPoint Caption;
	int CaptionMargin; // left and right of the caption, for shortening it (TTab's constructor)
	TFont ActiveFont;
	TFont InactiveFont;
	TFont UnreadFont;
	TFont BlinkingFont; // an unread tab while it blinks
};

// The frame of a dialog (TDialog, Dialogs.cpp), and the font of the texts and labels in the dialogs.
struct TDialogProperties {
	int MinimumWidth;
	int MinimumHeight;
	TRect Caption;
	TFont CaptionFont;
	std::array<TGUIPoint, 3> Buttons; // the buttons from the right
	TRect Separator;
	TRect Body; // FitToChild takes it by value
	TFont TextFont;
};

// The ignore list dialog (TIgnoreListDialog,
// Dialogs.cpp).
struct TIgnoreListDialogProperties {
	int Width;
	int Height;
	TRect IgnoreListCheckBox;
	TRect IgnoreListLabel;
	TRect IgnoreList;
	TGUIPoint AddIgnoreButton;
	TGUIPoint DeleteIgnoreButton;
	TRect IgnorePermanently;
	TRect GlobalIgnoreLabel;
	TRect IgnoreYelling;
	TRect IgnorePrivate;
	TRGBColor PermanentColor; // the lines of the permanent entries
	TRect WhiteListCheckBox;
	TRect WhiteListLabel;
	TRect WhiteList;
	TGUIPoint AddWhiteButton;
	TGUIPoint DeleteWhiteButton;
	TRect AllowPermanently;
	TRect GlobalAllowLabel;
	TRect AllowBuddies;
};

// The info dialog (CreateInfoDialog, Dialogs.cpp).
struct TInfoDialogProperties {
	int Width;
	int Height;
	TFont LabelFont;
	TFont LinkFont;
	int Border;
	TRect Label;
	TRect Link;
	TRect Separator;
};

// The dialog that edits a VIP (TEditBuddyDialog,
// Dialogs.cpp).
struct TEditBuddyDialogProperties {
	int Width;
	int Height;
	TRect Name;
	TRect Separator;
	TRect TypeLabel;
	std::array<TGUIPoint, 11> IconButtons;
	TRect DescriptionLabel;
	TRect Description;
	TRect Notify;
};

// The help dialog (THelpDialog, Dialogs.cpp).
struct THelpDialogProperties {
	int Width;
	int Height;
	TGUIPoint ClientHelpButton;
	TGUIPoint HintsButton;
	TGUIPoint TutorHelpButton;
	TGUIPoint RuleViolationButton;
	TGUIPoint ManualButton;
	TGUIPoint FAQButton;
	TGUIPoint ClientHelpLabel;
	TGUIPoint HintsLabel;
	TGUIPoint TutorHelpLabel;
	TGUIPoint RuleViolationLabel;
	TGUIPoint ManualLabel;
	TGUIPoint FAQLabel;
};

// The rule violation dialog (TRuleViolationDialog,
// Dialogs.cpp).
struct TRuleViolationDialogProperties {
	int Width;
	int Height;
	TRect NameLabel;
	TRect Name;
	TRect ReasonLabel;
	TRect ReasonList;
	TRect ActionLabel;
	TRect ActionList;
	TRect CommentLabel;
	TRect Comment;
	TRect StatementLabel;
	TRect Statement;
	TRect IPBanishment;
};

// The options dialog (TOptionsDialog, Dialogs.cpp).
struct TOptionsDialogProperties {
	int Width;
	int Height;
	TGUIPoint GeneralButton;
	TGUIPoint GraphicsButton;
	TGUIPoint ConsoleButton;
	TGUIPoint NetworkButton;
	TGUIPoint HotkeysButton;
	TGUIPoint MotdButton;
	TGUIPoint GeneralLabel;
	TGUIPoint GraphicsLabel;
	TGUIPoint ConsoleLabel;
	TGUIPoint NetworkLabel;
	TGUIPoint HotkeysLabel;
	TGUIPoint MotdLabel;
	TRect Separator;
};

// The advanced graphics dialog
// (TAdvancedGraphicsDialog, Dialogs.cpp).
struct TAdvancedGraphicsDialogProperties {
	int Width;
	int Height;
	TGUIPoint DeviceLabel;
	TGUIPoint DirectX5Button;
	TGUIPoint DirectX9Button;
	TGUIPoint OpenGLButton;
	TRect LightEffects;
	TRect NoStretching;
	TRect WindowsMousepointer;
	TRect Framerate;
	TGUIPoint FramerateLimitText;
	TGUIPoint CurrentFramerateText;
};

// The player's states (TPlayerStates, ContentWindows.cpp): LargeRow and
// SmallRow hold the first icon's position in the large and the small row,
// IconDistance the distance between the icons.
struct TPlayerStatesProperties {
	TRect LargeRow;
	TRect SmallRow;
	int NumberOfIcons;
	int IconDistance;
};

// The layout of the large and the small combat controls (ContentWindows.cpp
// copies a whole block by value): their height and width and the positions of
// the offensive, balanced, defensive, stand, chase, secure, stop, options and
// help buttons.
struct TCombatControlsProperties {
	int Height;
	int Width;
	std::array<TGUIPoint, 9> Buttons;
};

struct TSkinProperties {
	TGUIPoint CursorHotspot1;
	TGUIPoint CursorHotspot2;
	TGUIPoint CursorHotspot3;
	TGUIPoint CursorHotspot4;
	TGUIPoint CursorHotspot5;
	TGUIPoint CursorHotspot6;
	TFont TooltipFont;
	int TooltipBorder;
	TRGBColor TooltipBackground;
	TRGBColor TooltipFrame;
	TRGBColor HighlightColor;
	TRGBColor AttackColor;
	TRGBColor FollowColor;
	TRGBColor HighlightedAttackColor;
	TRGBColor HighlightedFollowColor;
	int MenuShortcutGap;               // the gap between a menu entry's text and its shortcut (ContextMenus.cpp)
	TRect MenuMargins;                 // a context menu's margins around its entries (FitToChildren, ContextMenus.cpp)
	TFont MenuFont;                    // the font of the menu entries (number and colour; ContextMenus.cpp)
	int MenuSeparatorHeight;           // the height of a menu separator
	TRGBColor MenuHighlightBackground; // the background of the menu entry under the mouse
	int MenuTextMargin;                // the space around a menu entry's text
	TStartWindowProperties StartWindow;
	int StartWindowBorder;      // the border of the start window's frame
	int SplitterHeight;         // the height of MainWindow.cpp's splitter
	int MapFrameBorder;         // the border of the map's frame (TMapFrame, MapWindow.cpp)
	TRect MapHintButton;        // the area of the map window's hint button (TMapWindow::Arrange)
	int HelpBubbleMinimumWidth; // the help bubble's minimum width and height (ContextMenus.cpp)
	int HelpBubbleMinimumHeight;
	int HelpBubbleMaximumWidth; // its maximum width and height
	int HelpBubbleMaximumHeight;
	TRect HelpBubbleTextMargins; // the margins around its text
	int HelpBubbleSizeStep;      // its sizes are HelpBubbleSizeRemainder modulo HelpBubbleSizeStep
	int HelpBubbleSizeRemainder;
	int HelpBubbleMargin;      // its distance to the help area and the window edges
	int FormattedMessageWidth; // the text's size in a formatted message dialog (CreateFormattedMessageDialog, Dialogs.cpp)
	int FormattedMessageHeight;
	int SideBarWidth;                  // the width of the side bar (MainWindow.cpp)
	TRGBColor DraggedWindowPlaceColor; // the place of the sub window being dragged (TDialogWindow::Draw)
	TSubWindowProperties SubWindow;
	int AutomapWindowHeight;
	TRect AutomapFrame; // the area whose edges the automap's frame follows (TAutomapWindow::Arrange, ContentWindows.cpp)
	TRect AutomapView;
	TRect AutomapZoomButtons;                // x, y: the position of zoom in; width, height: that of zoom out
	TRect AutomapFloorButtons;               // x, y: the position of floor up; width, height: that of floor down
	TRect AutomapCentreButtonAndCompassRose; // x, y: the position of "Centre"; width, height: that of the compass rose
	int InventoryWindowHeight;
	TRect InventoryControlsAndView; // x, y: the position of the combat controls; width, height: that of the inventory view
	TInventoryViewProperties InventoryView;
	TInventoryLabelProperties CapacityLabel; // the capacity (TCapacityLabel, ContentWindows.cpp)
	TInventoryLabelProperties SoulLabel;     // the soul points (TSoulLabel)
	TPlayerStatesProperties PlayerStates;
	TFont ObjectCountFont;                         // the count of an object (DrawObject)
	TCombatControlsProperties LargeCombatControls; // the large combat controls (ContentWindows.cpp)
	TCombatControlsProperties SmallCombatControls; // the small ones
	int SmallInventoryWindowHeight;
	TRect SmallInventoryControlsAndMaximise; // x, y: the position of the combat controls; width, height: that of the maximise button
	TRect SmallInventoryCapacity;
	TRect SmallInventorySoul;
	TRect SmallInventoryStates;
	TRect SmallInventoryDropArea; // where a dragged object is dropped on the small inventory (skin 99)
	TStatusWindowProperties StatusWindow;
	int ButtonsWindowHeight;
	TRect ButtonsWindowSkillsAndBattle; // x, y: the position of "Skills"; width, height: that of "Battle"
	TRect ButtonsWindowBuddyAndLogout;  // x, y: the position of "VIP"; width, height: that of "Logout"
	int ContainerSlotsPerRow;
	TGUIPoint ContainerFirstSlot;
	TRect ContainerSlotSizeAndObject; // x, y: a slot's width and height; width, height: the object's offset in the row and column
	int ContainerColumnWidth;
	int ContainerRowHeight;
	int ContainerMinimumHeight;
	TFont SkillsFont;
	int SkillsHorizontalMargin;
	int SkillsVerticalMargin;
	TRGBColor SkillsFirstBarsColor; // the bars of the first seven skill lines (ContentWindows.cpp)
	TRGBColor SkillsOtherBarsColor; // the bars of the skills
	TBattleWindowProperties BattleWindow;
	TTradeWindowProperties TradeWindow;
	TBuddylistWindowProperties BuddylistWindow;
	int BuddylistWindowExtraHeight;  // added to the height of the buddies' rows (TBuddylistWindowBody::GetContentHeight)
	int ScrollBarRangeMinimumHeight; // the vertical scroll bar (DialogElements.h): its range's minimum height
	int ScrollBarMinimumWidth;       // its and its range's minimum width
	int ScrollBarMinimumHeight;
	int ScrollBarWidth;
	int HorizontalScrollBarRangeMinimumWidth; // the horizontal scroll bar: its range's minimum width
	int HorizontalScrollBarHeight;
	int HorizontalScrollBarMinimumWidth;
	TButtonsProperties Buttons;
	int BlinkingButtonInterval; // milliseconds between a blinking button's changes (TBlinkingButton)
	TCheckBoxProperties CheckBox;
	int ObjectIconFrameWidth; // the frame of a framed object icon (TObjectIcon::Draw)
	TEditLineProperties EditLine;
	int EditLineTextMargin;
	TListBoxProperties ListBox;
	int ListBoxSelectionMargin; // the selected line's background is larger than its text by this
	TConsoleWindowProperties ConsoleWindow;
	TTabProperties Tab;
	TDialogProperties Dialog;
	int LoginDialogWidth;
	int LoginDialogHeight;
	TRect LoginDialogAccountLabel;
	TRect LoginDialogAccount; // the login dialog's account number
	TRect LoginDialogPasswordLabel;
	TRect LoginDialogPassword;  // its password
	TRect LoginDialogLink;      // its link
	int ReconnectWindowSpacing; // between the reconnect window's text, bar and time (TReconnectWindow)
	int CharacterListDialogWidth;
	int CharacterListDialogHeight;
	TRect CharacterListDialogLabel;  // the character list's label
	TRect CharacterListDialogList;   // its list
	TRect CharacterListDialogStatus; // its account status
	int MoveObjectsDialogWidth;
	int MoveObjectsDialogHeight;
	TRect MoveObjectsDialogScrollBar; // the move dialog's scroll bar
	TRect MoveObjectsDialogLabel;     // its label
	TGUIPoint MoveObjectsDialogObject;
	int OutfitDialogWidth;
	int OutfitDialogHeight;
	TRect OutfitDialogIcon; // the area of the outfit with its border
	int OutfitDialogIconBorder;
	TRGBColor OutfitDialogIconHighlight;
	TGUIPoint OutfitDialogNextButton;
	std::array<TGUIPoint, 4> OutfitDialogPartButtons; // the outfit dialog's part buttons
	TGUIPoint OutfitDialogColorButtons;               // the position of its first colour button
	int OutfitDialogColorColumnWidth;
	int OutfitDialogColorRowHeight;
	TGUIPoint OutfitDialogLabel;
	int TextDialogWidth;
	int TextDialogHeight;
	TRect TextDialogObjectIcon;
	TRGBColor TextDialogObjectHighlight;
	TRect TextDialogDescription; // the text dialog's description
	TRect TextDialogText;        // its text
	int EditListDialogWidth;
	int EditListDialogHeight;
	TRect EditListDialogPicture;     // x, y: the position of the edit list's picture (skin 217)
	TRect EditListDialogDescription; // the edit list's description
	TRect EditListDialogText;        // its text
	TIgnoreListDialogProperties IgnoreListDialog;
	int BugReportDialogWidth;
	int BugReportDialogHeight;
	TRect BugReportDialogText;  // the bug report's text
	TRect BugReportDialogLabel; // its label
	TInfoDialogProperties InfoDialog;
	int NewChannelDialogWidth;
	int NewChannelDialogHeight;
	TRect NewChannelDialogListLabel; // the new channel dialog's labels and fields
	TRect NewChannelDialogList;
	TRect NewChannelDialogNameLabel;
	TRect NewChannelDialogName;
	int PrivateChannelDialogWidth;
	int PrivateChannelDialogHeight;
	TRect PrivateChannelDialogName;  // the private channel dialog's name
	TRect PrivateChannelDialogLabel; // its label
	int ReportDialogWidth;
	int ReportDialogHeight;
	TRect ReportDialogText;  // the report dialog's text
	TRect ReportDialogLabel; // its label
	int AddBuddyDialogWidth;
	int AddBuddyDialogHeight;
	TRect AddBuddyDialogName;  // the VIP dialog's name
	TRect AddBuddyDialogLabel; // its label
	TEditBuddyDialogProperties EditBuddyDialog;
	THelpDialogProperties HelpDialog;
	int HintsDialogWidth;
	int HintsDialogHeight;
	TRect HintsDialogLabel; // the hints dialog's label
	TRect HintsDialogList;  // its list
	TRuleViolationDialogProperties RuleViolationDialog;
	int EditMarkDialogWidth;
	int EditMarkDialogHeight;
	TRect EditMarkDialogDescriptionLabel; // the mark dialog's labels and text
	TRect EditMarkDialogDescription;
	TRect EditMarkDialogTypeLabel;
	std::array<TGUIPoint, 14> EditMarkDialogTypeButtons;
	TOptionsDialogProperties OptionsDialog;
	int GeneralOptionsDialogWidth;
	int GeneralOptionsDialogHeight;
	TRect GeneralOptionsDialogClassicControl; // the general options' check boxes
	TRect GeneralOptionsDialogAutoChaseOff;
	TRect GeneralOptionsDialogShowHints;
	TRect GeneralOptionsDialogTextualEffects;
	TRect GeneralOptionsDialogCreatureNames;
	int GraphicsOptionsDialogWidth;
	int GraphicsOptionsDialogHeight;
	TRect GraphicsOptionsDialogFullscreen; // the graphics options' controls
	TRect GraphicsOptionsDialogResolutionList;
	TRect GraphicsOptionsDialogResolutionLabel;
	TGUIPoint GraphicsOptionsDialogAdvancedButton;
	TRect GraphicsOptionsDialogAdvancedLabel;
	TAdvancedGraphicsDialogProperties AdvancedGraphicsDialog;
	int ConsoleOptionsDialogWidth;
	int ConsoleOptionsDialogHeight;
	TRect ConsoleOptionsDialogInfoMessages; // the console options' check boxes
	TRect ConsoleOptionsDialogEventMessages;
	TRect ConsoleOptionsDialogStatusMessages;
	TRect ConsoleOptionsDialogTimeStamps;
	TRect ConsoleOptionsDialogPrivateMessages;
	int NetworkOptionsDialogWidth;
	int NetworkOptionsDialogHeight;
	TRect NetworkOptionsDialogTestServerLogin; // the network options' check box
	int HotkeyDialogWidth;
	int HotkeyDialogHeight;
	TRect HotkeyDialogList;      // the hotkey dialog's list
	TRect HotkeyDialogListLabel; // its labels and fields
	TRect HotkeyDialogText;
	TRect HotkeyDialogTextLabel;
	TRect HotkeyDialogAutoSend;
	TRGBColor HotkeyDialogAutoSendColor; // the lines of hotkeys sent automatically
};

namespace patella {

// Skins.cpp's data and GUI.cpp's SkinProperties, TGraphics' member Skins. Public, as the globals were: the window files
// read them through the accessors below.
class TSkins {
public:
	TSkins() = default;
	TSkins(const TSkins &) = delete;
	TSkins &operator=(const TSkins &) = delete;
	TSkins(TSkins &&) = delete;
	TSkins &operator=(TSkins &&) = delete;
	~TSkins() = default;

	void InitSkins();
	void ExitSkins();

	// Skins.cpp's data, then GUI.cpp's.
	int NumberOfSkins{};
	std::unique_ptr<TSkin[]> Skins;   // the table's only owner
	TSkinProperties SkinProperties{}; // GUI.cpp's; filled by InitSkinProperties
};

// The transition's access path: the one TClient's TSkins (Client.cpp). It goes when no free function needs it.
namespace bridge {
TSkins &Skins();
} // namespace bridge

} // namespace patella

// The original's globals' names for their about 2,400 readers in the window files: SkinProperties() and Skins() are the
// one TClient's, so a use reads `SkinProperties().TooltipBorder` where it read `SkinProperties.TooltipBorder`. Not the
// design's `bridge::Skins().SkinProperties` at each use: the same object, without rewriting every expression of the
// windows.
inline TSkinProperties &SkinProperties()
{
	return patella::bridge::Skins().SkinProperties;
}

inline std::unique_ptr<TSkin[]> &Skins()
{
	return patella::bridge::Skins().Skins;
}

// The original's free functions (graphics.cpp's: they call the bridge, which patella_core does not have), forwarding to
// TSkins' members.
void InitSkins();
void ExitSkins();

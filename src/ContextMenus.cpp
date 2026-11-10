// ContextMenus.cpp: reconstructed from Tibia.exe 7.72 (0x43d6f0-0x4458ff).
//
// The help bubble and the context menus; see ContextMenus.h. The object
// opens with the skill menu's destructor (0x43d6f0, no frame, no line); the
// ContentWindows/ContextMenus gap 0x43d6f0-0x43d86b is this file's (the
// destructor and its deleting form use the vtable 0x55f5f0 of the skill menu,
// whose constructor is here; 0x43d730 is line 142). The functions follow in
// line order. The binary moves the help bubble's constructor and Place, and
// TContextMenu's AddEntry and Show, behind the event handlers, followed by
// the menus' constructors: each calls a function defined further down (the
// pattern noted for Creatures.cpp and DialogWindow.cpp). The inline functions
// of ContextMenus.h are emitted with the vtables after the constructors.
//
// A menu is built by its constructor: AddEntry for every entry (Separator =
// true starts a new group, AddSeparator draws the line), then Show, which
// gives every entry the width of the widest, stacks them, fits the menu
// around them and keeps it on the screen. A click on an entry sends its
// event to the menu (TCallback), which acts in HandleEvent; any button
// release or Escape ends the action state, and Control.cpp deletes the menu.
//
// The try blocks and catch lines the comments name are the original's (its
// catch wrappers, common.h); since issue #78 the functions here have none.

#include "ContextMenus.h"
#include "Communication.h"
#include "ConsoleWindow.h"
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
#include "Text.h" // IWYU pragma: keep (the object's own copy of the text table, see Text.h)
#include "Utils.h"
#include "graphics.h"
#include "sysinc.h"
#include "GUI.h"
#include "common.h"
#include "types/FixedString.h"
#include <cstdint>
#include <cstring>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries; the original's varargs (TMultiStateButton's constructor, _snprintf)
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)

namespace {

// At 0x5c4508, constructed by the static initialiser 0x559de0 as
// TCallback(NULL, 0) and never used (the other window files have one too);
// ContextMenus.obj's only datum, right after ContentWindows.obj's
// NoCallback (0x5c4500, initialiser 0x559dd0) and before Control.obj's .bss
// (0x5c4510).
// NOLINTNEXTLINE(bugprone-throwing-static-initialization): the binary's global; TCallback's constructor (GUI.cpp) stores two values
TCallback NoCallback(nullptr, 0);

} // namespace


// --- Functions without line evidence at the start ------------------------------

// 0x43d6f0 (deleting destructor 0x43d710). The skill window forgets the menu.
TSkillContextMenu::~TSkillContextMenu()
{
	if (SkillsWindow != nullptr)
		SetSkillsWindowMenu(SkillsWindow, nullptr);
}

// --- THelpBubble ---------------------------------------------------------------

// 0x441d60, lines 106-121. The assertion comes before the try block. The text
// is fitted twice: before and after it has laid itself out.
THelpBubble::THelpBubble(const char *HelpText)
{
	PATELLA_ASSERT(HelpText!=nullptr);
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	Text = new TFormattedText(HelpText);
	InsertChild(Text);
	SetSize(SkinProperties().HelpBubbleMaximumWidth, SkinProperties().HelpBubbleMaximumHeight);
	FitToChild(Text, SkinProperties().HelpBubbleTextMargins, true);
	Text->FitToChildren(TRect(0, 0, 0, 0), true);
	FitToChild(Text, SkinProperties().HelpBubbleTextMargins, true);
}

// 0x43d730, line 142. The size is rounded up to HelpBubbleSizeRemainder modulo HelpBubbleSizeStep and
// kept between the minimum and HelpBubbleMaximumWidth/HelpBubbleMaximumHeight (the larger of the two wins
// when the minimum is above the maximum).
void THelpBubble::SetSize(int Width, int Height)
{
	const int MaxWidth = SkinProperties().HelpBubbleMaximumWidth;
	if (Width % SkinProperties().HelpBubbleSizeStep != SkinProperties().HelpBubbleSizeRemainder)
		Width += SkinProperties().HelpBubbleSizeRemainder - Width % SkinProperties().HelpBubbleSizeStep;
	if (Height % SkinProperties().HelpBubbleSizeStep != SkinProperties().HelpBubbleSizeRemainder)
		Height += SkinProperties().HelpBubbleSizeRemainder - Height % SkinProperties().HelpBubbleSizeStep;
	const int MinWidth = getMinimumWidth();
	const int MaxHeight = SkinProperties().HelpBubbleMaximumHeight;
	if (MaxWidth < MinWidth) {
		if (Width < MaxWidth)
			Width = MaxWidth;
		if (Width > MinWidth)
			Width = MinWidth;
	} else {
		if (Width < MinWidth)
			Width = MinWidth;
		if (Width > MaxWidth)
			Width = MaxWidth;
	}
	const int MinHeight = getMinimumHeight();
	if (MaxHeight < MinHeight) {
		if (Height < MaxHeight)
			Height = MaxHeight;
		if (Height > MinHeight)
			Height = MinHeight;
	} else {
		if (Height < MinHeight)
			Height = MinHeight;
		if (Height > MaxHeight)
			Height = MaxHeight;
	}
	TWindow::SetSize(Width, Height);
	SetChildArea(Text, SkinProperties().HelpBubbleTextMargins);
	Arrange();
}

// 0x43d9b0. The frame: corners 52-55, edges 56-59, the middle 60.
void THelpBubble::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_PANEL_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_PANEL_EDGE_TOP, Skins()[SKIN_PANEL_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_PANEL_CORNER_TOPRIGHT].width - Skins()[SKIN_PANEL_CORNER_TOPLEFT].width, Skins()[SKIN_PANEL_EDGE_TOP].height,
		0, 0);
	DrawingList[2] = TDrawing(SKIN_PANEL_CORNER_TOPRIGHT, Width - Skins()[SKIN_PANEL_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_PANEL_EDGE_LEFT, 0, Skins()[SKIN_PANEL_CORNER_TOPLEFT].height, Skins()[SKIN_PANEL_EDGE_LEFT].width, Height - Skins()[SKIN_PANEL_CORNER_BOTTOMLEFT].height - Skins()[SKIN_PANEL_CORNER_TOPLEFT].height,
		0, 0);
	DrawingList[4] = TDrawing(SKIN_PANEL_EDGE_RIGHT, Width - Skins()[SKIN_PANEL_EDGE_RIGHT].width, Skins()[SKIN_PANEL_CORNER_TOPRIGHT].height, Skins()[SKIN_PANEL_EDGE_RIGHT].width,
		Height - Skins()[SKIN_PANEL_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_PANEL_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_PANEL_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_PANEL_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_PANEL_EDGE_BOTTOM, Skins()[SKIN_PANEL_CORNER_BOTTOMLEFT].width, Height - Skins()[SKIN_PANEL_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_PANEL_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_PANEL_CORNER_BOTTOMLEFT].width, Skins()[SKIN_PANEL_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_PANEL_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_PANEL_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_PANEL_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
	DrawingList[8] = TDrawing(SKIN_PANEL_BACKGROUND, Skins()[SKIN_PANEL_EDGE_LEFT].width, Skins()[SKIN_PANEL_EDGE_TOP].height, Width - Skins()[SKIN_PANEL_EDGE_RIGHT].width - Skins()[SKIN_PANEL_EDGE_LEFT].width,
		Height - Skins()[SKIN_PANEL_EDGE_BOTTOM].height - Skins()[SKIN_PANEL_EDGE_TOP].height, 0, 0);
}

// 0x442110, lines 257-272. Tries eight places next to the help area (left
// and right of it at its top, at its bottom and at its bottom moved up by
// the margin and kept in the window; then below and above it) and takes the
// first that fits; FitsIn moves a coordinate of 0 to the margin. Without one
// the bubble goes to the top right corner while the mouse is in the left
// half of the window, else to the top left one. The second assertion of
// that case (line 263, newY) is folded into the first by the compiler, and
// that path skips lines 267-268.
void THelpBubble::Place(TWindow *Window)
{
	const int Margin = SkinProperties().HelpBubbleMargin;
	int HelpX, HelpY, HelpWidth, HelpHeight;
	GetHelpArea(&HelpX, &HelpY, &HelpWidth, &HelpHeight);
	int w = Width;
	int h = Height;
	int newX = HelpX - w - Margin;
	int newY = HelpY;
	bool Found = FitsIn(newX, newY, w, h, Window);
	if (!Found) {
		newX = HelpX + HelpWidth + Margin;
		newY = HelpY;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX - w - Margin;
		newY = HelpY - h + HelpHeight;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX + HelpWidth + Margin;
		newY = HelpY - h + HelpHeight;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX - w - Margin;
		newY = HelpY - h - Margin + HelpHeight;
		if (newY < Margin)
			newY = Margin;
		if (newY + h > Window->Height - Margin)
			newY = Window->Height - h - Margin;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX + HelpWidth + Margin;
		newY = HelpY - h - Margin + HelpHeight;
		if (newY < Margin)
			newY = Margin;
		if (newY + h > Window->Height - Margin)
			newY = Window->Height - h - Margin;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX;
		newY = HelpY + HelpHeight + Margin;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		newX = HelpX;
		newY = HelpY - h - Margin;
		Found = FitsIn(newX, newY, w, h, Window);
	}
	if (!Found) {
		if (GetMouseX() < Window->Width / 2) {
			newX = Window->Width - w - Margin;
			newY = Margin;
			PATELLA_ASSERT_VALUES(newX>=0, newX);
			PATELLA_ASSERT_VALUES(newY>=0, newY);
		} else {
			newX = Margin;
			newY = Margin;
			PATELLA_ASSERT_VALUES(newX>=0, newX);
			PATELLA_ASSERT_VALUES(newY>=0, newY);
		}
	}
	PATELLA_ASSERT_VALUES(newX>=0, newX);
	PATELLA_ASSERT_VALUES(newY>=0, newY);
	SetPosition(newX, newY);
	SetSize(w, h);
}

// 0x43dca0, lines 289-293. The bubble fits at x, y if it keeps the margin to
// every edge of the window; a coordinate of 0 becomes the margin first.
bool THelpBubble::FitsIn(int &x, int &y, int &Width, int &Height, TWindow *Window)
{
	const int Margin = SkinProperties().HelpBubbleMargin;
	if (x == 0)
		x = Margin;
	if (y == 0)
		y = Margin;
	if (x - Margin < 0 || y - Margin < 0)
		return false;
	if (x + Width > Window->Width - Margin || y + Height > Window->Height - Margin)
		return false;
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	return true;
}

// --- TContextMenuEntry ---------------------------------------------------------

// 0x43dfe0, line 316. As wide as the text, the gap and the shortcut, and
// as high as a line of the font, plus the space around both.
TContextMenuEntry::TContextMenuEntry(const char *_Text, const char *_Shortcut, TCallback _Callback)
	: Callback(nullptr, 0)
{
	CopyString(Text.Chars, _Text);
	if (_Shortcut == nullptr)
		Shortcut[0] = 0;
	else
		CopyString(Shortcut.Chars, _Shortcut);
	Callback = _Callback;
	const int h = GetFontLineHeight(SkinProperties().MenuFont.FontNumber) + SkinProperties().MenuTextMargin * 2;
	int w = GetTextWidth(SkinProperties().MenuFont.FontNumber, Text) + SkinProperties().MenuTextMargin * 2
		+ SkinProperties().MenuShortcutGap;
	if (Shortcut[0] != 0)
		w += GetTextWidth(SkinProperties().MenuFont.FontNumber, Shortcut);
	SetSize(w, h);
}

// 0x43e210, line 338. The entry under the mouse gets a background as high
// as the font (not the line); the shortcut is right-aligned.
void TContextMenuEntry::Draw(int Surface, int x, int y)
{
	if (In(GetMouseX() - x, 0, Width - 1) && In(GetMouseY() - y, 0, Height - 1)) {
		GetSurface()->FillRectangle(Surface,
			TRect(x, y, Width,
				GetFontHeight(SkinProperties().MenuFont.FontNumber) + SkinProperties().MenuTextMargin * 2),
			SkinProperties().MenuHighlightBackground);
	}
	y += SkinProperties().MenuTextMargin;
	PrintText(Surface, x + SkinProperties().MenuTextMargin, y, SkinProperties().MenuFont.FontNumber, SkinProperties().MenuFont.Color,
		Text, LEFT_ALIGN);
	if (Shortcut[0] != 0) {
		PrintText(Surface, x + Width - SkinProperties().MenuTextMargin - 1, y, SkinProperties().MenuFont.FontNumber,
			SkinProperties().MenuFont.Color, Shortcut, RIGHT_ALIGN);
	}
}

// 0x43e4d0, line 349
bool TContextMenuEntry::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	Callback.Send();
	return true;
}

// 0x43e660, line 361. The last entry loses the space below the font.
void TContextMenuEntry::SetLast()
{
	// The original's order: the left operand first (MSVC; GCC and Clang may differ, issue #91).
	const int LineHeight = GetFontLineHeight(SkinProperties().MenuFont.FontNumber);
	SetSize(Width, Height - (LineHeight - GetFontHeight(SkinProperties().MenuFont.FontNumber)));
}

// --- TContextMenu --------------------------------------------------------------

// 0x43e820, line 381. Nine frame skins (Arrange).
TContextMenu::TContextMenu()
{
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	EntryWidth = 0;
	Separator = false;
	Empty = true;
}

// 0x43ea00. The frame: corners 208-211, edges 212-215, the middle 207.
void TContextMenu::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_MENU_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_MENU_EDGE_TOP, Skins()[SKIN_MENU_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_MENU_CORNER_TOPRIGHT].width - Skins()[SKIN_MENU_CORNER_TOPLEFT].width,
		Skins()[SKIN_MENU_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_MENU_CORNER_TOPRIGHT, Width - Skins()[SKIN_MENU_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_MENU_EDGE_LEFT, 0, Skins()[SKIN_MENU_CORNER_TOPLEFT].height, Skins()[SKIN_MENU_EDGE_LEFT].width,
		Height - Skins()[SKIN_MENU_CORNER_BOTTOMLEFT].height - Skins()[SKIN_MENU_CORNER_TOPLEFT].height, 0, 0);
	DrawingList[4] = TDrawing(SKIN_MENU_EDGE_RIGHT, Width - Skins()[SKIN_MENU_EDGE_RIGHT].width, Skins()[SKIN_MENU_CORNER_TOPRIGHT].height, Skins()[SKIN_MENU_EDGE_RIGHT].width,
		Height - Skins()[SKIN_MENU_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_MENU_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_MENU_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_MENU_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_MENU_EDGE_BOTTOM, Skins()[SKIN_MENU_CORNER_BOTTOMLEFT].width, Height - Skins()[SKIN_MENU_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_MENU_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_MENU_CORNER_BOTTOMLEFT].width, Skins()[SKIN_MENU_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_MENU_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_MENU_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_MENU_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
	DrawingList[8] = TDrawing(SKIN_MENU_BACKGROUND, Skins()[SKIN_MENU_EDGE_LEFT].width, Skins()[SKIN_MENU_EDGE_TOP].height, Width - Skins()[SKIN_MENU_EDGE_RIGHT].width - Skins()[SKIN_MENU_EDGE_LEFT].width,
		Height - Skins()[SKIN_MENU_EDGE_BOTTOM].height - Skins()[SKIN_MENU_EDGE_TOP].height, 0, 0);
}

// 0x43ecf0, line 437. The entry under the mouse sends its event first.
bool TContextMenu::LeftButtonUp(int x, int y, int Flags)
{
	TWindow::LeftButtonUp(x, y, Flags);
	SetActionState(ACTIONSTATE_NONE);
	return true;
}

// 0x43eea0, line 449
bool TContextMenu::RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	SetActionState(ACTIONSTATE_NONE);
	return true;
}

// 0x43f030, line 460. Escape closes the menu.
void TContextMenu::KeyPress(int Key)
{
	if (Key == 27)
		SetActionState(ACTIONSTATE_NONE);
}

// 0x442de0, lines 486-498. Opens the menu at x, y, or left of and above it
// where it would reach the right or bottom edge. A menu without entries gets
// an empty one; that one is added after the entries were laid out, so it
// keeps its own width and the position 0,0.
void TContextMenu::Show(int x, int y, int ParentWidth, int ParentHeight)
{
	int Position = 0;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		Child->SetSize(EntryWidth, Child->Height);
		Child->SetPosition(0, Position);
		Position += Child->Height;
	}
	if (GetLastChild() == nullptr)
		AddEntry(0, "", nullptr);
	TContextMenuEntry *LastEntry = static_cast<TContextMenuEntry *>(GetLastChild());
	PATELLA_ASSERT(LastEntry!=nullptr);
	LastEntry->SetLast();
	FitToChildren(SkinProperties().MenuMargins, true);
	Arrange();
	if (x + Width >= ParentWidth)
		x -= Width;
	if (x < 0)
		x = 0;
	if (y + Height >= ParentHeight)
		y -= Height;
	if (y < 0)
		y = 0;
	SetPosition(x, y);
}

// 0x442910, lines 506-521. The assertions come before the try block. A
// pending separator is drawn only between entries, and it stays pending
// while the menu is empty.
void TContextMenu::AddEntry(int Event, const char *Text, const char *Shortcut)
{
	PATELLA_ASSERT(Text!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Text)<50, strlen(Text));
	PATELLA_ASSERT_VALUES(Shortcut==nullptr || strlen(Shortcut)<20, strlen(Shortcut));
	if (Separator && !Empty) {
		AddSeparator();
		Separator = false;
	}
	TContextMenuEntry *Entry = new TContextMenuEntry(Text, Shortcut, TCallback(this, Event));
	if (EntryWidth < Entry->Width)
		EntryWidth = Entry->Width;
	AppendChild(Entry);
	Empty = false;
}

// 0x43f1d0, line 542. Show widens the line to the entries.
void TContextMenu::AddSeparator()
{
	TSeparator *Line = new TSeparator(true);
	Line->SetSize(50, SkinProperties().MenuSeparatorHeight);
	AppendChild(Line);
}

// --- TObjectContextMenu --------------------------------------------------------

// 0x443130, lines 560-670. The shortcuts are shown with the modern controls
// only. The rotate test reads the flags of the used object even when there
// is none (type 0).
TObjectContextMenu::TObjectContextMenu(int x, int y, int ParentWidth, int ParentHeight)
{
	LookedObject = *GetTradeObject(&LookedObjectIndex, &LookedObjectX, &LookedObjectY, &LookedObjectZ);
	UsedObject = *GetUseObject(&UsedObjectIndex, &UsedObjectX, &UsedObjectY, &UsedObjectZ);
	CreatureID = GetTradeInfo();
	PATELLA_ASSERT(LookedObject!=0);
	const bool ClassicControl = GetClassicControl();
	AddEntry(EVENT_MENU_LOOK, "Look", ClassicControl ? nullptr : "(Shift)");
	if (UsedObject != 0) {
		if (GetObjectFlag(UsedObject, CONTAINER)) {
			if (UsedObjectX == POSITION_NOT_ON_MAP && UsedObjectY >= POSITION_CONTAINER_BASE) {
				AddEntry(EVENT_MENU_OPEN, "Open", ClassicControl ? nullptr : "(Ctrl)");
				AddEntry(EVENT_MENU_OPEN_NEW_WINDOW, "Open in new window", nullptr);
			} else {
				AddEntry(EVENT_MENU_OPEN_NEW_WINDOW, "Open", ClassicControl ? nullptr : "(Ctrl)");
			}
		} else if (GetObjectFlag(UsedObject, MULTIUSE)) {
			AddEntry(EVENT_MENU_USE_WITH, "Use with ...", ClassicControl ? nullptr : "(Ctrl)");
		} else {
			AddEntry(EVENT_MENU_USE, "Use", ClassicControl ? nullptr : "(Ctrl)");
		}
	}
	if (GetObjectFlag(UsedObject, ROTATE))
		AddEntry(EVENT_MENU_ROTATE, "Rotate", nullptr);
	Separator = true;
	if (LookedObject != CREATURE && !GetObjectFlag(LookedObject, UNMOVE) && GetObjectFlag(LookedObject, TAKE))
		AddEntry(EVENT_MENU_TRADE_WITH, "Trade with ...", nullptr);
	if (LookedObjectX == POSITION_NOT_ON_MAP && LookedObjectY >= POSITION_CONTAINER_BASE && GetContainerHasParent(LookedObjectY - POSITION_CONTAINER_BASE))
		AddEntry(EVENT_MENU_MOVE_UP, "Move up", nullptr);
	if (CreatureID != 0) {
		if (CreatureID == GetPlayerID()) {
			AddEntry(EVENT_MENU_SET_OUTFIT, "Set Outfit", nullptr);
			if (IsPlayerInParty() && !IsPlayerInFight())
				AddEntry(EVENT_MENU_LEAVE_PARTY, "Leave Party", nullptr);
		} else {
			if (CreatureID == GetAttackID())
				AddEntry(EVENT_MENU_STOP_ATTACK, "Stop Attack", ClassicControl ? nullptr : "(Alt)");
			else
				AddEntry(EVENT_MENU_ATTACK, "Attack", ClassicControl ? nullptr : "(Alt)");
			if (CreatureID == GetFollowID())
				AddEntry(EVENT_MENU_STOP_FOLLOW, "Stop Follow", nullptr);
			else
				AddEntry(EVENT_MENU_FOLLOW, "Follow", nullptr);
			Separator = true;
			TCreature *Creature = GetCreature(CreatureID);
			if (Creature != nullptr && IsPlayer(CreatureID)) {
				TFixedString<50> Text;
				Text.Format("Message to {}", Creature->Name);
				AddEntry(EVENT_MENU_PRIVATE_MESSAGE, Text, nullptr);
				if (HasOwnPrivateChannel())
					AddEntry(EVENT_MENU_INVITE_PRIVATE, "Invite to private chat", nullptr);
				if (!IsBuddy(Creature->Name))
					AddEntry(EVENT_MENU_ADD_VIP, "Add to VIP list", nullptr);
				Text.Format("{} {}", IsTalkIgnored(0, Creature->Name, -1) ? "Unignore" : "Ignore", Creature->Name);
				AddEntry(EVENT_MENU_IGNORE, Text, nullptr);
				// The binary passes the name to "Invite to Party" as well
				// (unused; left out here, the compiler warns about it).
				switch (GetPlayerPartyFlag()) {
				case PARTY_NONE:
				case PARTY_INVITEE:
					if (Creature->PARTYFlag == PARTY_INVITER) {
						Text.Format("Join {}'s Party", Creature->Name);
						AddEntry(EVENT_MENU_JOIN_PARTY, Text, nullptr);
					} else {
						Text.Assign("Invite to Party");
						AddEntry(EVENT_MENU_INVITE_PARTY, Text, nullptr);
					}
					break;
				case PARTY_INVITER:
					if (Creature->PARTYFlag == PARTY_INVITEE) {
						Text.Format("Revoke {}'s Invitation", Creature->Name);
						AddEntry(EVENT_MENU_REVOKE_PARTY_INVITE, Text, nullptr);
					}
					break;
				case PARTY_MEMBER:
					break;
				case PARTY_LEADER:
					if (Creature->PARTYFlag == PARTY_INVITEE) {
						Text.Format("Revoke {}'s Invitation", Creature->Name);
						AddEntry(EVENT_MENU_REVOKE_PARTY_INVITE, Text, nullptr);
					} else if (Creature->PARTYFlag == PARTY_MEMBER) {
						Text.Format("Pass Leadership to {}", Creature->Name);
						AddEntry(EVENT_MENU_PASS_PARTY_LEADER, Text, nullptr);
					} else {
						Text.Assign("Invite to Party");
						AddEntry(EVENT_MENU_INVITE_PARTY, Text, nullptr);
					}
					break;
				default:
					TERROR1("unknown player party flag", GetPlayerPartyFlag());
				}
			}
		}
		Separator = true;
		AddEntry(EVENT_MENU_COPY, "Copy Name", nullptr);
		if (IsPlayer(CreatureID) && CreatureID != GetPlayerID() && CheckAnyRight()) {
			Separator = true;
			AddEntry(EVENT_MENU_RULE_VIOLATION, "Rule Violation", nullptr);
			Separator = true;
		}
	}
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x43f3a0, lines 712-780; jump table 0x43fd98 with the byte table 0x43fdfc
// (Event-10001, 0-202). "Move up" moves the object to z 0xfe of its
// container's position, the parent container; "Open" opens a container in
// the window of the one it is in.
void TObjectContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_LOOK:
		SendLook(LookedObjectX, LookedObjectY, LookedObjectZ, LookedObject.Type, LookedObjectIndex);
		break;
	case EVENT_MENU_USE:
		SendUseObject(UsedObjectX, UsedObjectY, UsedObjectZ, UsedObject.Type, UsedObjectIndex, 0);
		break;
	case EVENT_MENU_USE_WITH:
		AddMessage(MESSAGE_USE_WITH, 0, 0);
		break;
	case EVENT_MENU_ROTATE:
		SendRotateObject(UsedObjectX, UsedObjectY, UsedObjectZ, UsedObject.Type, UsedObjectIndex);
		break;
	case EVENT_MENU_ATTACK:
		SetAttackID(CreatureID);
		SendAttack(CreatureID);
		break;
	case EVENT_MENU_STOP_ATTACK:
		SetAttackID(0);
		SendAttack(0);
		break;
	case EVENT_MENU_FOLLOW:
		SetFollowID(CreatureID);
		SendFollow(CreatureID);
		break;
	case EVENT_MENU_STOP_FOLLOW:
		SetFollowID(0);
		SendFollow(0);
		break;
	case EVENT_MENU_SET_OUTFIT:
		SendRequestOutfit();
		break;
	case EVENT_MENU_MOVE_UP:
		PATELLA_ASSERT_VALUES(LookedObjectX==0xFFFF, LookedObjectX);
		PATELLA_ASSERT(LookedObject.Type!=CREATURE);
		SendMoveObject(LookedObjectX, LookedObjectY, LookedObjectZ, LookedObject.Type, LookedObjectIndex,
			LookedObjectX, LookedObjectY, POSITION_PARENT_CONTAINER, GetObjectFlag(LookedObject, CUMULATIVE) ? LookedObject.Data : 1);
		break;
	case EVENT_MENU_OPEN:
		PATELLA_ASSERT_VALUES(UsedObjectX==0xFFFF, UsedObjectX);
		PATELLA_ASSERT_VALUES(UsedObjectY>=64, UsedObjectY);
		SendUseObject(UsedObjectX, UsedObjectY, UsedObjectZ, UsedObject.Type, UsedObjectIndex, UsedObjectY - POSITION_CONTAINER_BASE);
		break;
	case EVENT_MENU_OPEN_NEW_WINDOW:
		SendUseObject(UsedObjectX, UsedObjectY, UsedObjectZ, UsedObject.Type, UsedObjectIndex, GetFreeContainer());
		break;
	case EVENT_MENU_TRADE_WITH:
		AddMessage(MESSAGE_TRADE_WITH, 0, 0);
		break;
	case EVENT_MENU_ADD_VIP: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendAddBuddy(Creature->Name);
		break;
	}
	case EVENT_MENU_INVITE_PRIVATE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendInviteToPrivateChannel(Creature->Name);
		break;
	}
	case EVENT_MENU_PRIVATE_MESSAGE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendOpenPrivateConversation(Creature->Name);
		break;
	}
	case EVENT_MENU_IGNORE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr) {
			if (IsTalkIgnored(0, Creature->Name, -1))
				UnignoreName(Creature->Name);
			else
				IgnoreName(Creature->Name, false);
		}
		break;
	}
	case EVENT_MENU_COPY: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			CopyToClipboard(Creature->Name);
		break;
	}
	case EVENT_MENU_INVITE_PARTY: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendInviteToParty(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_JOIN_PARTY: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendJoinParty(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_REVOKE_PARTY_INVITE: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendRevokePartyInvitation(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_PASS_PARTY_LEADER: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendPassPartyLeadership(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_LEAVE_PARTY:
		SendLeaveParty();
		break;
	case EVENT_MENU_RULE_VIOLATION: {
		// The rule violation dialog gets a copy of the name (Control.cpp
		// deletes it).
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr && CheckAnyRight()) {
			char *Name = new char[30];
			TStringBuffer(Name, 30).Assign(Creature->Name);
			AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, reinterpret_cast<std::intptr_t>(Name), 0);
		}
		break;
	}
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TCreatureContextMenu ------------------------------------------------------

// 0x443960, lines 845-862. Like the creature part of the object menu,
// without the shortcuts and the test for the own character.
TCreatureContextMenu::TCreatureContextMenu(uint32 CreatureID, int x, int y, int ParentWidth, int ParentHeight)
{
	this->CreatureID = CreatureID;
	if (this->CreatureID == GetAttackID())
		AddEntry(EVENT_MENU_STOP_ATTACK, "Stop Attack", nullptr);
	else
		AddEntry(EVENT_MENU_ATTACK, "Attack", nullptr);
	if (this->CreatureID == GetFollowID())
		AddEntry(EVENT_MENU_STOP_FOLLOW, "Stop Follow", nullptr);
	else
		AddEntry(EVENT_MENU_FOLLOW, "Follow", nullptr);
	Separator = true;
	TCreature *Creature = GetCreature(this->CreatureID);
	if (Creature != nullptr && IsPlayer(this->CreatureID)) {
		TFixedString<50> Text;
		Text.Format("Message to {}", Creature->Name);
		AddEntry(EVENT_MENU_PRIVATE_MESSAGE, Text, nullptr);
		if (HasOwnPrivateChannel())
			AddEntry(EVENT_MENU_INVITE_PRIVATE, "Invite to private chat", nullptr);
		if (!IsBuddy(Creature->Name))
			AddEntry(EVENT_MENU_ADD_VIP, "Add to VIP list", nullptr);
		Text.Format("{} {}", IsTalkIgnored(0, Creature->Name, -1) ? "Unignore" : "Ignore", Creature->Name);
		AddEntry(EVENT_MENU_IGNORE, Text, nullptr);
		switch (GetPlayerPartyFlag()) {
		case PARTY_NONE:
		case PARTY_INVITEE:
			if (Creature->PARTYFlag == PARTY_INVITER) {
				Text.Format("Join {}'s Party", Creature->Name);
				AddEntry(EVENT_MENU_JOIN_PARTY, Text, nullptr);
			} else {
				Text.Assign("Invite to Party");
				AddEntry(EVENT_MENU_INVITE_PARTY, Text, nullptr);
			}
			break;
		case PARTY_INVITER:
			if (Creature->PARTYFlag == PARTY_INVITEE) {
				Text.Format("Revoke {}'s Invitation", Creature->Name);
				AddEntry(EVENT_MENU_REVOKE_PARTY_INVITE, Text, nullptr);
			}
			break;
		case PARTY_MEMBER:
			break;
		case PARTY_LEADER:
			if (Creature->PARTYFlag == PARTY_INVITEE) {
				Text.Format("Revoke {}'s Invitation", Creature->Name);
				AddEntry(EVENT_MENU_REVOKE_PARTY_INVITE, Text, nullptr);
			} else if (Creature->PARTYFlag == PARTY_MEMBER) {
				Text.Format("Pass Leadership to {}", Creature->Name);
				AddEntry(EVENT_MENU_PASS_PARTY_LEADER, Text, nullptr);
			} else {
				Text.Assign("Invite to Party");
				AddEntry(EVENT_MENU_INVITE_PARTY, Text, nullptr);
			}
			break;
		default:
			TERROR1("unknown player party flag", GetPlayerPartyFlag());
		}
		Separator = true;
	}
	AddEntry(EVENT_MENU_COPY, "Copy Name", nullptr);
	if (Creature != nullptr && IsPlayer(this->CreatureID) && CheckAnyRight()) {
		Separator = true;
		AddEntry(EVENT_MENU_RULE_VIOLATION, "Rule Violation", nullptr);
		Separator = true;
	}
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x43fed0, lines 934-937; jump table 0x440350 with the byte table 0x44038c
// (Event-10004, 0-199).
void TCreatureContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_ATTACK:
		SetAttackID(CreatureID);
		SendAttack(CreatureID);
		break;
	case EVENT_MENU_STOP_ATTACK:
		SetAttackID(0);
		SendAttack(0);
		break;
	case EVENT_MENU_FOLLOW:
		SetFollowID(CreatureID);
		SendFollow(CreatureID);
		break;
	case EVENT_MENU_STOP_FOLLOW:
		SetFollowID(0);
		SendFollow(0);
		break;
	case EVENT_MENU_ADD_VIP: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendAddBuddy(Creature->Name);
		break;
	}
	case EVENT_MENU_INVITE_PRIVATE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendInviteToPrivateChannel(Creature->Name);
		break;
	}
	case EVENT_MENU_PRIVATE_MESSAGE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendOpenPrivateConversation(Creature->Name);
		break;
	}
	case EVENT_MENU_IGNORE: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr) {
			if (!IsTalkIgnored(0, Creature->Name, -1))
				IgnoreName(Creature->Name, false);
			else
				UnignoreName(Creature->Name);
		}
		break;
	}
	case EVENT_MENU_COPY: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			CopyToClipboard(Creature->Name);
		break;
	}
	case EVENT_MENU_INVITE_PARTY: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendInviteToParty(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_JOIN_PARTY: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendJoinParty(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_REVOKE_PARTY_INVITE: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendRevokePartyInvitation(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_PASS_PARTY_LEADER: {
		const TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr)
			SendPassPartyLeadership(Creature->CreatureID);
		break;
	}
	case EVENT_MENU_RULE_VIOLATION: {
		TCreature *Creature = GetCreature(CreatureID);
		if (Creature != nullptr && CheckAnyRight()) {
			char *Name = new char[30];
			TStringBuffer(Name, 30).Assign(Creature->Name);
			AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, reinterpret_cast<std::intptr_t>(Name), 0);
		}
		break;
	}
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TBuddyContextMenu ---------------------------------------------------------

// 0x443e20, line 987. _CharacterID is not in the list when the menu is
// opened on the free space. "Sort by status" is offered only while the
// offline VIPs are shown.
TBuddyContextMenu::TBuddyContextMenu(uint32 _CharacterID, int x, int y, int ParentWidth, int ParentHeight)
{
	CharacterID = _CharacterID;
	const int Position = GetBuddyPosition(_CharacterID);
	if (Position != -1) {
		TFixedString<50> Text;
		Text.Format("Edit {}", GetBuddyListName(Position));
		AddEntry(EVENT_MENU_EDIT_VIP, Text, nullptr);
		Text.Format("Remove {}", GetBuddyListName(Position));
		AddEntry(EVENT_MENU_REMOVE_VIP, Text, nullptr);
		if (CharacterID != GetPlayerID() && GetBuddyListOnline(Position)) {
			Text.Format("Message to {}", GetBuddyListName(Position));
			AddEntry(EVENT_MENU_MESSAGE_VIP, Text, nullptr);
		}
		if (HasOwnPrivateChannel())
			AddEntry(EVENT_MENU_INVITE_PRIVATE, "Invite to private chat", nullptr);
		Separator = true;
	}
	AddEntry(EVENT_MENU_NEW_VIP, "Add new VIP", nullptr);
	AddEntry(EVENT_MENU_SORT_BY_NAME, "Sort by name", nullptr);
	AddEntry(EVENT_MENU_SORT_BY_TYPE, "Sort by type", nullptr);
	if (!GetBuddyHideOffline())
		AddEntry(EVENT_MENU_SORT_BY_STATUS, "Sort by status", nullptr);
	AddEntry(EVENT_MENU_HIDE_OFFLINE, GetBuddyHideOffline() ? "Show offline VIPs" : "Hide offline VIPs", nullptr);
	Separator = true;
	if (Position != -1) {
		AddEntry(EVENT_MENU_COPY_NAME, "Copy Name", nullptr);
		if (CheckAnyRight()) {
			Separator = true;
			AddEntry(EVENT_MENU_RULE_VIOLATION, "Rule Violation", nullptr);
			Separator = true;
		}
	}
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x440460, lines 1040-1043; a compare tree with the jump table 0x4408d8
// (Event-10303, 0-4). The cases are written in the order of their code.
void TBuddyContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_RULE_VIOLATION: {
		const int Position = GetBuddyPosition(CharacterID);
		if (Position != -1 && CheckAnyRight()) {
			char *Name = new char[30];
			TStringBuffer(Name, 30).Assign(GetBuddyListName(Position));
			AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, reinterpret_cast<std::intptr_t>(Name), 0);
		}
		break;
	}
	case EVENT_MENU_INVITE_PRIVATE: {
		const int Position = GetBuddyPosition(CharacterID);
		if (Position != -1)
			SendInviteToPrivateChannel(GetBuddyListName(Position));
		break;
	}
	case EVENT_MENU_COPY_NAME: {
		const int Position = GetBuddyPosition(CharacterID);
		if (Position != -1)
			CopyToClipboard(GetBuddyListName(Position));
		break;
	}
	case EVENT_MENU_REMOVE_VIP:
		RemoveBuddy(CharacterID);
		SendRemoveBuddy(CharacterID);
		break;
	case EVENT_MENU_NEW_VIP:
		AddMessage(MESSAGE_ADD_BUDDY_DIALOG, 0, 0);
		break;
	case EVENT_MENU_EDIT_VIP:
		AddMessage(MESSAGE_EDIT_BUDDY_DIALOG, CharacterID, 0);
		break;
	case EVENT_MENU_MESSAGE_VIP: {
		const int Position = GetBuddyPosition(CharacterID);
		if (Position != -1)
			SendOpenPrivateConversation(GetBuddyListName(Position));
		break;
	}
	case EVENT_MENU_SORT_BY_NAME:
		SetBuddySortMethod(0);
		break;
	case EVENT_MENU_SORT_BY_TYPE:
		SetBuddySortMethod(1);
		break;
	case EVENT_MENU_SORT_BY_STATUS:
		SetBuddySortMethod(2);
		break;
	case EVENT_MENU_HIDE_OFFLINE:
		HideOfflineBuddies(!GetBuddyHideOffline());
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TTextContextMenu ----------------------------------------------------------

// 0x444160, lines 1056-1120. The assertions come before the try block. The
// entries for the speaker are offered for lines of other characters only,
// and none for a game master's answer in the rule violation channel. The
// test of the addressee array against NULL is the binary's.
TTextContextMenu::TTextContextMenu(TTextWindow *_TextWindow, int _LineNumber, int x, int y, int ParentWidth,
	int ParentHeight)
{
	PATELLA_ASSERT(_TextWindow!=nullptr);
	PATELLA_ASSERT_VALUES(GetCreature(GetPlayerID()), GetPlayerID());
	TextWindow = _TextWindow;
	LineNumber = _LineNumber;
	TTalkLine *Line = TextWindow->GetTalkLine(_LineNumber);
	bool OtherSpeaker = false;
	bool HasSpeaker = false;
	if (Line != nullptr) {
		const char *Speaker = Line->Addressee;
		if (Speaker != nullptr && strlen(Speaker) != 0) {
			HasSpeaker = true;
			if (strcmp(GetCreature(GetPlayerID())->Name, Speaker) != 0)
				OtherSpeaker = true;
		}
		if (TextWindow->TextBuffer->IsRuleViolation() && Line->Mode == TALK_GAMEMASTER_ANSWER) {
			HasSpeaker = false;
		} else if (OtherSpeaker) {
			TFixedString<50> Text;
			Text.Format("Message to {}", Speaker);
			AddEntry(EVENT_MENU_PRIVATE_MESSAGE, Text, nullptr);
			if (TextWindow->TextBuffer->GetSelected() && TextWindow->TextBuffer->IsPrivateChannel()) {
				if (HasOwnPrivateChannel())
					AddEntry(EVENT_MENU_EXCLUDE_PRIVATE, "Exclude from private chat", nullptr);
			} else if (HasOwnPrivateChannel()) {
				AddEntry(EVENT_MENU_INVITE_PRIVATE, "Invite to private chat", nullptr);
			}
			if (!IsBuddy(Speaker))
				AddEntry(EVENT_MENU_ADD_VIP, "Add to VIP list", nullptr);
			Text.Format("{} {}", IsTalkIgnored(0, Speaker, -1) ? "Unignore" : "Ignore", Speaker);
			AddEntry(EVENT_MENU_IGNORE, Text, nullptr);
			Separator = true;
		}
	}
	if (TextWindow->HasSelection())
		AddEntry(EVENT_MENU_COPY, "Copy", nullptr);
	if (HasSpeaker)
		AddEntry(EVENT_MENU_COPY_NAME, "Copy Name", nullptr);
	if (Line != nullptr)
		AddEntry(EVENT_MENU_COPY_MESSAGE, "Copy Message", nullptr);
	Separator = true;
	AddEntry(EVENT_MENU_SELECT_ALL, "Select all", nullptr);
	if (HasSpeaker && CheckAnyRight()) {
		Separator = true;
		AddEntry(EVENT_MENU_RULE_VIOLATION, "Rule Violation", nullptr);
		Separator = true;
	}
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x4408f0, line 1173. Unknown events do nothing. "Rule Violation" passes a
// copy of the line, or nothing without one.
void TTextContextMenu::HandleEvent(int Event)
{
	TTalkLine *Line = TextWindow->GetTalkLine(LineNumber);
	const char *Speaker = nullptr;
	if (Line != nullptr)
		Speaker = Line->Addressee;
	switch (Event) {
	case EVENT_MENU_PRIVATE_MESSAGE:
		if (Speaker != nullptr)
			SendOpenPrivateConversation(Speaker);
		break;
	case EVENT_MENU_IGNORE:
		if (Speaker != nullptr) {
			TFixedString<200> Text;
			const char *Format;
			if (!IsTalkIgnored(0, Speaker, -1)) {
				IgnoreName(Speaker, false);
				Format = "You are now ignoring player %s.";
			} else {
				UnignoreName(Speaker);
				Format = "You are no longer ignoring player %s.";
			}
			MsvcSnprintf(Text, 200, Format, Speaker);
			AddConsoleText(0, Text, TALK_STATUS_MESSAGE, nullptr, 0, 0);
		}
		break;
	case EVENT_MENU_INVITE_PRIVATE:
		if (Speaker != nullptr)
			SendInviteToPrivateChannel(Speaker);
		break;
	case EVENT_MENU_EXCLUDE_PRIVATE:
		if (Speaker != nullptr)
			SendExcludeFromPrivateChannel(Speaker);
		break;
	case EVENT_MENU_ADD_VIP:
		if (Speaker != nullptr)
			SendAddBuddy(Speaker);
		break;
	case EVENT_MENU_COPY_NAME:
		if (Speaker != nullptr)
			CopyToClipboard(Speaker);
		break;
	case EVENT_MENU_COPY_MESSAGE:
		if (Line != nullptr)
			CopyToClipboard(Line->GetText());
		break;
	case EVENT_MENU_COPY: {
		const char *Selection = TextWindow->GetSelectedText();
		if (Selection != nullptr)
			CopyToClipboard(Selection);
		break;
	}
	case EVENT_MENU_SELECT_ALL:
		TextWindow->SelectAll();
		break;
	case EVENT_MENU_RULE_VIOLATION:
		if (CheckAnyRight()) {
			if (Line == nullptr) {
				AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, 0, 0);
			} else {
				AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, 0,
					reinterpret_cast<std::intptr_t>(new TTalkLine(Line->StatementID, Line->Mode, Line->Addressee,
						Line->Text, 0)));
			}
		}
		break;
	default:
		break;
	}
}

// --- TRequestContextMenu -------------------------------------------------------

// 0x444720, lines 1181-1203. The assertion comes before the try block.
TRequestContextMenu::TRequestContextMenu(TConsoleWindow *_ConsoleWindow, int _LineNumber, int x, int y,
	int ParentWidth, int ParentHeight)
{
	PATELLA_ASSERT(_ConsoleWindow!=nullptr);
	ConsoleWindow = _ConsoleWindow;
	LineNumber = _LineNumber;
	TTalkLine *Line = ConsoleWindow->CurrentConsole->TextWindow->GetTalkLine(_LineNumber);
	if (Line != nullptr && Line->Addressee != nullptr && strlen(Line->Addressee) != 0) {
		TFixedString<50> Text;
		Text.Format("Process {}", Line->Addressee);
		AddEntry(EVENT_MENU_PROCESS_REPORT, Text, nullptr);
		Text.Format("Remove {}", Line->Addressee);
		AddEntry(EVENT_MENU_REMOVE_REPORT, Text, nullptr);
		Separator = true;
		AddEntry(EVENT_MENU_COPY_NAME, "Copy Name", nullptr);
		AddEntry(EVENT_MENU_COPY_MESSAGE, "Copy Message", nullptr);
	}
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x440c60, lines 1221-1241. "Process" opens the reporter's channel with the
// request as its first line. Unknown events do nothing.
void TRequestContextMenu::HandleEvent(int Event)
{
	TTalkLine *Line = ConsoleWindow->CurrentConsole->TextWindow->GetTalkLine(LineNumber);
	if (Line != nullptr && Line->Addressee != nullptr && strlen(Line->Addressee) != 0) {
		const char *Name = Line->Addressee;
		switch (Event) {
		case EVENT_MENU_PROCESS_REPORT: {
			TFixedString<220> Title;
			Title.Format("{}'s Report", Name);
			ConsoleWindow->OpenChannel(TALK_PLAYER_ANSWER, TALK_GAMEMASTER_ANSWER, Name, -1, Title);
			TTalkBuffer *TextBuffer = GetTextBuffer(TALK_PLAYER_ANSWER, Name, -1);
			PATELLA_ASSERT(TextBuffer!=nullptr);
			TextBuffer->Clear();
			AddTextBufferLine(TextBuffer, Line->StatementID, Line->Text, TALK_PLAYER_ANSWER, Name, 0);
			TextBuffer->SetWriteEnabled(true);
			SendProcessRuleViolation(Name);
			break;
		}
		case EVENT_MENU_REMOVE_REPORT:
			SendCloseRuleViolation(Name);
			break;
		case EVENT_MENU_COPY_NAME:
			CopyToClipboard(Name);
			break;
		case EVENT_MENU_COPY_MESSAGE:
			CopyToClipboard(Line->GetText());
			break;
		case EVENT_MENU_RULE_VIOLATION:
			if (CheckAnyRight()) {
				AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, 0, reinterpret_cast<std::intptr_t>(new TTalkLine(Line->StatementID, Line->Mode, Name, Line->Text, 0)));
			}
			break;
		default:
			break;
		}
	}
}

// --- TChannelContextMenu -------------------------------------------------------

// 0x444a70, lines 1248-1275. The first assertion comes before the try
// block. The default channel cannot be closed; a private channel of the
// player's own gets "Invite player" and "Exclude player".
TChannelContextMenu::TChannelContextMenu(TConsoleWindow *_ConsoleWindow, int x, int y, int ParentWidth,
	int ParentHeight)
{
	PATELLA_ASSERT(_ConsoleWindow!=nullptr);
	ConsoleWindow = _ConsoleWindow;
	TTalkBuffer *TextBuffer = ConsoleWindow->GetTextBuffer();
	PATELLA_ASSERT(TextBuffer!=nullptr);
	if (!TextBuffer->isDefault()) {
		if (TextBuffer->GetSelected() && TextBuffer->IsPrivateChannel()) {
			AddEntry(EVENT_MENU_INVITE_PRIVATE, "Invite player", nullptr);
			AddEntry(EVENT_MENU_EXCLUDE_PRIVATE, "Exclude player", nullptr);
			Separator = true;
		}
		AddEntry(EVENT_MENU_CLOSE_CHANNEL, "Close", nullptr);
		const bool Shown = ConsoleWindow->GetServerMessages();
		ShowServerMessages = !Shown;
		AddEntry(EVENT_MENU_SERVER_MESSAGES, Shown ? "Hide server messages" : "Show server messages", nullptr);
		Separator = true;
	}
	AddEntry(EVENT_MENU_SAVE_WINDOW, "Save Window", nullptr);
	if (!TextBuffer->IsRuleViolationChannel())
		AddEntry(EVENT_MENU_CLEAR_WINDOW, "Clear Window", nullptr);
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x440fe0, lines 1307-1312. The invitation and exclusion go through dialogs
// (Control.cpp's events 0x13 and 0x14).
void TChannelContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_CLOSE_CHANNEL:
		ConsoleWindow->CloseChannel();
		break;
	case EVENT_MENU_INVITE_PRIVATE:
		AddMessage(MESSAGE_INVITE_DIALOG, 0, 0);
		break;
	case EVENT_MENU_EXCLUDE_PRIVATE:
		AddMessage(MESSAGE_EXCLUDE_DIALOG, 0, 0);
		break;
	case EVENT_MENU_SERVER_MESSAGES:
		ConsoleWindow->SetServerMessages(ShowServerMessages);
		break;
	case EVENT_MENU_SAVE_WINDOW:
		static_cast<void>(ConsoleWindow->SaveChannel()); // the original ignores whether it was saved
		break;
	case EVENT_MENU_CLEAR_WINDOW: {
		TTalkBuffer *TextBuffer = ConsoleWindow->GetTextBuffer();
		PATELLA_ASSERT(TextBuffer!=nullptr);
		ClearTextBuffer(TextBuffer);
		break;
	}
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TEditContextMenu ----------------------------------------------------------

// 0x444eb0, lines 1324-1349. The assertion comes before the try block.
TEditContextMenu::TEditContextMenu(TEditLine *_Edit, int x, int y, int ParentWidth, int ParentHeight)
{
	PATELLA_ASSERT(_Edit!=nullptr);
	Edit = _Edit;
	if (_Edit->SelectionStart != -1 && _Edit->SelectionLength != 0) {
		if (!_Edit->ReadOnly)
			AddEntry(EVENT_MENU_CUT, "Cut", nullptr);
		AddEntry(EVENT_MENU_COPY, "Copy", nullptr);
	}
	if (!IsClipboardEmpty() && !Edit->ReadOnly)
		AddEntry(EVENT_MENU_PASTE, "Paste", nullptr);
	Separator = true;
	if (Edit->SelectionStart != -1 && Edit->SelectionLength != 0 && !Edit->ReadOnly) {
		AddEntry(EVENT_MENU_DELETE, "Delete", nullptr);
		Separator = true;
	}
	AddEntry(EVENT_MENU_SELECT_ALL, "Select All", nullptr);
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x441370, lines 1363-1366; jump table 0x44163c (Event-10200, 0-6). The
// edit line does the work (its HandleEvent).
void TEditContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_PASTE:
		Edit->HandleEvent(EVENT_PASTE);
		break;
	case EVENT_MENU_CUT:
		Edit->HandleEvent(EVENT_CUT);
		break;
	case EVENT_MENU_DELETE:
		Edit->HandleEvent(EVENT_DELETE);
		break;
	case EVENT_MENU_COPY:
		Edit->HandleEvent(EVENT_COPY);
		break;
	case EVENT_MENU_SELECT_ALL:
		Edit->HandleEvent(EVENT_SELECT_ALL);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TMarkContextMenu ----------------------------------------------------------

// 0x4451e0, lines 1379-1397. The assertion comes before the try block; the
// menu keeps a copy of the data and deletes the original.
TMarkContextMenu::TMarkContextMenu(TMarkMenuData *_Data, int x, int y, int ParentWidth, int ParentHeight)
{
	PATELLA_ASSERT(_Data!=nullptr);
	Data = *_Data;
	if (Data.Mark.x == 0 && Data.Mark.y == 0) {
		AddEntry(EVENT_MENU_SET_MARK, "Set Mark", nullptr);
	} else {
		AddEntry(EVENT_MENU_EDIT_MARK, "Edit Mark", nullptr);
		AddEntry(EVENT_MENU_DELETE_MARK, "Delete Mark", nullptr);
	}
	Show(x, y, ParentWidth, ParentHeight);
	delete _Data;
}

// 0x441660, lines 1426-1429; jump table 0x4419ec (Event-10400, 0-3). The
// mark dialog (Control.cpp's events 0x1d and 0x1e) gets copies of the
// position and the mark. Event 10402 (walk to the mark) has no entry.
void TMarkContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_SET_MARK:
		AddMessage(MESSAGE_SET_MARK_DIALOG, reinterpret_cast<std::intptr_t>(new TCoordinate(Data.Coordinate)), 0);
		break;
	case EVENT_MENU_EDIT_MARK:
		AddMessage(MESSAGE_EDIT_MARK_DIALOG, reinterpret_cast<std::intptr_t>(new TCoordinate(Data.Coordinate)), reinterpret_cast<std::intptr_t>(new TMark(Data.Mark)));
		break;
	case EVENT_MENU_GO_TO_MARK:
		if (Data.Mark.x != 0 && Data.Mark.y != 0) {
			switch (SetPlayerDestination(Data.Mark.x, Data.Mark.y, Data.Coordinate.z, false, false)) {
			case 1:
				SendWalk();
				break;
			case 2:
				SendStop();
				break;
			default:
				break;
			}
		}
		break;
	case EVENT_MENU_DELETE_MARK:
		DeleteAutomapMark(Data.Mark, Data.Coordinate.z);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TSkillContextMenu ---------------------------------------------------------

// 0x4454f0, lines 1442-1465. The assertion comes before the try block. The
// separator before the last two entries is drawn directly.
TSkillContextMenu::TSkillContextMenu(TSkillsWindowBody *_SkillsWindow, int x, int y, int ParentWidth,
	int ParentHeight)
{
	PATELLA_ASSERT(_SkillsWindow);
	SkillsWindow = _SkillsWindow;
	SetSkillsWindowMenu(_SkillsWindow, this);
	AddEntry(EVENT_MENU_LEVEL_BAR, GetLevelBar() ? "Hide Level" : "Show Level", nullptr);
	AddEntry(EVENT_MENU_MAGIC_LEVEL_BAR, GetMagLevelBar() ? "Hide Magic Level" : "Show Magic Level", nullptr);
	AddEntry(EVENT_MENU_FIST_BAR, GetSkillBar(0) ? "Hide Fist Fighting" : "Show Fist Fighting", nullptr);
	AddEntry(EVENT_MENU_CLUB_BAR, GetSkillBar(1) ? "Hide Club Fighting" : "Show Club Fighting", nullptr);
	AddEntry(EVENT_MENU_SWORD_BAR, GetSkillBar(2) ? "Hide Sword Fighting" : "Show Sword Fighting", nullptr);
	AddEntry(EVENT_MENU_AXE_BAR, GetSkillBar(3) ? "Hide Axe Fighting" : "Show Axe Fighting", nullptr);
	AddEntry(EVENT_MENU_DISTANCE_BAR, GetSkillBar(4) ? "Hide Distance Fighting" : "Show Distance Fighting", nullptr);
	AddEntry(EVENT_MENU_SHIELDING_BAR, GetSkillBar(5) ? "Hide Shielding" : "Show Shielding", nullptr);
	AddEntry(EVENT_MENU_FISHING_BAR, GetSkillBar(6) ? "Hide Fishing" : "Show Fishing", nullptr);
	AddSeparator();
	AddEntry(EVENT_MENU_SHOW_ALL_BARS, "Show all Skill Bars", nullptr);
	AddEntry(EVENT_MENU_HIDE_ALL_BARS, "Hide all Skill Bars", nullptr);
	Show(x, y, ParentWidth, ParentHeight);
}

// 0x441a00, lines 1525-1530; jump table 0x441d34 with the byte table 0x441d48
// (Event-10500, 0-10). The skill window is laid out again.
void TSkillContextMenu::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_MENU_LEVEL_BAR:
		SetLevelBar(!GetLevelBar());
		break;
	case EVENT_MENU_MAGIC_LEVEL_BAR:
		SetMagLevelBar(!GetMagLevelBar());
		break;
	case EVENT_MENU_FIST_BAR:
	case EVENT_MENU_CLUB_BAR:
	case EVENT_MENU_SWORD_BAR:
	case EVENT_MENU_AXE_BAR:
	case EVENT_MENU_DISTANCE_BAR:
	case EVENT_MENU_SHIELDING_BAR:
	case EVENT_MENU_FISHING_BAR:
		SetSkillBar(Event - EVENT_MENU_FIST_BAR, !GetSkillBar(Event - EVENT_MENU_FIST_BAR));
		break;
	case EVENT_MENU_SHOW_ALL_BARS:
		SetLevelBar(true);
		SetMagLevelBar(true);
		for (int i = 0; i < 7; i++)
			SetSkillBar(i, true);
		break;
	case EVENT_MENU_HIDE_ALL_BARS:
		SetLevelBar(false);
		SetMagLevelBar(false);
		for (int i = 0; i < 7; i++)
			SetSkillBar(i, false);
		break;
	default:
		TERROR1("unknown event", Event);
	}
	if (SkillsWindow != nullptr)
		ArrangeSkillsWindow(SkillsWindow);
}
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)

// GUI.cpp: reconstructed from Tibia.exe 7.72.
//
// The window system's base classes (TComponent, TElement, TWindow), skin
// drawing, the timers, the hotkey table and the skin properties. Functions
// without line evidence come first, in binary order (the gap before the
// first asserted function: the compiler-generated constructors of
// SkinProperties and its blocks, Container.h code and the small functions
// without exception frame), then the asserted ones in line order. The
// TComponent functions that the binary has only as inline copies in earlier
// objects (GetNext, GetParent, ...) are in GUI.h.

#include "GUI.h"
#include "Skins.h"
#include "sysinc.h"
#include "Help.h"
#include "common.h"
#include "graphics.h"
#include "log/Log.h"

#include <sstream>
#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

// TComponent::DrawingList as a std::unique_ptr<TDrawing[]> (issue #58): a pointer at the original's offset. The
// subclasses' constructors allocate it with reset(new TDrawing[n]) (each list is the first: TComponent's constructor
// leaves it null, and no class allocates over a base's), ~TComponent frees it with reset() where the original had
// delete[]; TDrawing's destructor is trivial, so the reset cannot throw.
static_assert(sizeof(std::unique_ptr<TDrawing[]>) == sizeof(TDrawing *)
		&& alignof(std::unique_ptr<TDrawing[]>) == alignof(TDrawing *),
	"std::unique_ptr<TDrawing[]> is a pointer");

// The data of .bss (NumberOfHotkeys, Timers, Hotkeys, UpdateTimers' static LastTime) is TGUI's (GUI.h, issue #107);
// SkinProperties is TGraphics' TSkins' (Skins.h, issue #116), which InitSkinProperties below fills.

// The skins' arrays std::arrays (issue #60): the original's layouts (no pointers: the same on x64).
static_assert(sizeof(TInventoryViewProperties) == 0x158 && offsetof(TInventoryViewProperties, Objects) == 0x10
		&& offsetof(TInventoryViewProperties, Slots) == 0x60 && offsetof(TInventoryViewProperties, EmptySlotSkins) == 0x100,
	"TInventoryViewProperties's layout");
static_assert(sizeof(TDialogProperties) == 0x8c && offsetof(TDialogProperties, Buttons) == 0x2c, "TDialogProperties's layout");
static_assert(sizeof(TEditBuddyDialogProperties) == 0xc0 && offsetof(TEditBuddyDialogProperties, IconButtons) == 0x38, "TEditBuddyDialogProperties's layout");
static_assert(sizeof(TCombatControlsProperties) == 0x50 && offsetof(TCombatControlsProperties, Buttons) == 0x08,
	"TCombatControlsProperties' layout");
static_assert(sizeof(TSkinProperties) == 0x1528 && offsetof(TSkinProperties, OutfitDialogPartButtons) == 0xc88
		&& offsetof(TSkinProperties, EditMarkDialogTypeButtons) == 0x1204,
	"TSkinProperties' layout");

int TTimer::GetTime() const
{
	return Remaining / 100;
}

bool TTimer::IsExpired() const
{
	return !(Active && Remaining != 0);
}

// noexcept since issue #108: the work of the window classes' destructors that
// could throw (TTab's assertion, the Send functions, the preferences,
// DeleteTextBuffer) is their Close (OnClose), which every delete of a component
// runs first; it was noexcept(false) from issue #34 to #108 (VC7.1's
// destructors let exceptions out; notes/ownership.md, section 7).
TComponent::~TComponent()
{
	if (DrawingList != nullptr)
		DrawingList.reset();
}

static_assert(std::is_nothrow_destructible_v<TComponent> && std::is_nothrow_destructible_v<TWindow>,
	"issue #108: the window classes' destructors are noexcept");
// offsetof of a class with virtual functions: conditionally supported, and supported by every compiler here (GCC's
// and Clang's -Winvalid-offsetof, issue #111).
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(sizeof(void *) != 8 || (sizeof(TComponent) == 56 && offsetof(TComponent, Parent) == 24),
	"issue #108: Closed is in the padding after DrawingListLength, TComponent's x64 layout unchanged");
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Issue #108 (not in the original). Closed is set first: a close that throws is
// not run again, as the original's destructor was not.
void TComponent::Close()
{
	if (Closed)
		return;
	Closed = true;
	OnClose();
}

// Issue #108.
void TComponent::CheckClosed(const char *Class) const noexcept
{
	if (!Closed)
		patella::log::Error("{} destroyed without Close: its close was not done (issue #108)", Class);
}

// Issue #108: the original's `delete Component`.
void DeleteComponent(TComponent *Component)
{
	if (Component == nullptr)
		return;
	Component->Close();
	delete Component;
}

void TComponent::SetPosition(int _OffsetX, int _OffsetY)
{
	OffsetX = _OffsetX;
	OffsetY = _OffsetY;
}

TWindow::TWindow()
{
	FirstChild = nullptr;
	Capture = nullptr;
}

bool TWindow::IsChild(TComponent *Child)
{
	for (const TComponent *Search = FirstChild; Search != nullptr; Search = Search->Next) {
		if (Search == Child)
			return true;
	}
	return false;
}

TComponent *TWindow::GetLastChild()
{
	if (FirstChild == nullptr)
		return nullptr;
	TComponent *Search = FirstChild;
	while (Search->Next != nullptr)
		Search = Search->Next;
	return Search;
}

TElement::TElement() = default;

// Identical code to any two-member constructor, so the linker may have
// folded others into it.
TCallback::TCallback(TComponent *Receiver, int Event)
{
	this->Receiver = Receiver;
	this->Event = Event;
}

// Called last by InitControl.
void patella::TGUI::InitHotkeys()
{
	Hotkeys.clear();
}

// The one copy the linker kept of many identical empty functions (folded).
// ExitControl calls this one first; TComponent::Arrange and
// PositionChanged and TElement::StopDragging have the same body.
void patella::TGUI::ExitHotkeys()
{
}

// --- Skins ----------------------------------------------------------------

// Tiles the skin over width x height; srcx and srcy cut that much off the
// first column and row.
void DrawSkin(int Surface, int x, int y, int width, int height, int skin, int srcx, int srcy)
{
	if (skin == SKIN_NONE || width == 0 || height == 0)
		return;
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	PATELLA_ASSERT_VALUES(In(skin,0,LASTSKIN-1), skin);
	PATELLA_ASSERT_VALUES(Skins()[skin].width>0, Skins()[skin].width);
	PATELLA_ASSERT_VALUES(Skins()[skin].height>0, Skins()[skin].height);
	PATELLA_ASSERT_VALUES(width>0, width);
	PATELLA_ASSERT_VALUES(height>0, height);
	if (Skins()[skin].width == width && Skins()[skin].height == height) {
		DrawPicturePart(Surface, x, y, 4, Skins()[skin].x, Skins()[skin].y, Skins()[skin].width, Skins()[skin].height);
	} else {
		int PieceHeight = 0;
		int dy = 0;
		for (int h = height; h > 0; h -= PieceHeight) {
			int dx = 0;
			int PieceWidth;
			for (int w = width; w > 0; w -= PieceWidth) {
				int SourceX, SourceY;
				if (w == width && srcx > 0) {
					PieceWidth = Skins()[skin].width - srcx;
					SourceX = Skins()[skin].x + srcx;
				} else {
					SourceX = Skins()[skin].x;
					PieceWidth = Skins()[skin].width;
				}
				if (w < PieceWidth)
					PieceWidth = w;
				if (h == height && srcy > 0) {
					SourceY = Skins()[skin].y + srcy;
					PieceHeight = Skins()[skin].height - srcy;
				} else {
					SourceY = Skins()[skin].y;
					PieceHeight = Skins()[skin].height;
				}
				if (PieceHeight > h)
					PieceHeight = h;
				DrawPicturePart(Surface, dx + x, dy + y, 4, SourceX, SourceY, PieceWidth, PieceHeight);
				dx += PieceWidth;
			}
			dy += PieceHeight;
		}
	}
}

// DrawSkin with a clipping rectangle, which is passed on to the picture
// function.
void DrawSkinClipped(int Surface, int x, int y, int width, int height, int skin, int ClipX, int ClipY,
	int ClipWidth, int ClipHeight, int skinoffsetx, int skinoffsety)
{
	if (skin == SKIN_NONE || width == 0 || height == 0)
		return;
	PATELLA_ASSERT_VALUES(In(skin,0,LASTSKIN-1), skin);
	PATELLA_ASSERT_VALUES(width>0, width);
	PATELLA_ASSERT_VALUES(height>0, height);
	if (Skins()[skin].width < 1 || Skins()[skin].height < 1) {
		std::stringstream s;
		s << "some assertions failed " << "(surface: " << Surface << " x: " << x << " y: " << y
		  << " width: " << width << " height: " << height << " skin: " << skin
		  << " skinwidth: " << Skins()[skin].width << " skinheight: " << Skins()[skin].height
		  << " skinoffsetx: " << skinoffsetx << " skinoffsety: " << skinoffsety
		  << " cliprect: " << ClipX << "/" << ClipY << "/" << ClipWidth << "/" << ClipHeight << ")";
		TERROR(s.str().c_str());
	}
	if (Skins()[skin].width == width && Skins()[skin].height == height) {
		DrawPicturePartClipped(Surface, x, y, 4, Skins()[skin].x, Skins()[skin].y, Skins()[skin].width,
			Skins()[skin].height, TRect(ClipX, ClipY, ClipWidth, ClipHeight));
	} else {
		int PieceHeight = 0;
		int dy = 0;
		for (int h = height; h > 0; h -= PieceHeight) {
			int dx = 0;
			int PieceWidth;
			for (int w = width; w > 0; w -= PieceWidth) {
				int SourceX, SourceY;
				if (w == width && skinoffsetx > 0) {
					PieceWidth = Skins()[skin].width - skinoffsetx;
					SourceX = Skins()[skin].x + skinoffsetx;
				} else {
					PieceWidth = Skins()[skin].width;
					SourceX = Skins()[skin].x;
				}
				if (w < PieceWidth)
					PieceWidth = w;
				if (h == height && skinoffsety > 0) {
					SourceY = Skins()[skin].y + skinoffsety;
					PieceHeight = Skins()[skin].height - skinoffsety;
				} else {
					SourceY = Skins()[skin].y;
					PieceHeight = Skins()[skin].height;
				}
				if (PieceHeight > h)
					PieceHeight = h;
				DrawPicturePartClipped(Surface, dx + x, dy + y, 4, SourceX, SourceY, PieceWidth, PieceHeight,
					TRect(ClipX, ClipY, ClipWidth, ClipHeight));
				dx += PieceWidth;
			}
			dy += PieceHeight;
		}
	}
}

// --- Skin properties ------------------------------------------------------

// Called by InitControl after InitSkins. The binary stores the values in
// another order; they are written here by offset.
void InitSkinProperties()
{
	SkinProperties().CursorHotspot1.x = 0;
	SkinProperties().CursorHotspot1.y = 0;
	SkinProperties().CursorHotspot2.x = 4;
	SkinProperties().CursorHotspot2.y = 9;
	SkinProperties().CursorHotspot3.x = 6;
	SkinProperties().CursorHotspot3.y = 10;
	SkinProperties().CursorHotspot4.x = 9;
	SkinProperties().CursorHotspot4.y = 9;
	SkinProperties().CursorHotspot5.x = 9;
	SkinProperties().CursorHotspot5.y = 9;
	SkinProperties().CursorHotspot6.x = 6;
	SkinProperties().CursorHotspot6.y = 6;
	SkinProperties().TooltipFont.FontNumber = FONT_SHARP;
	SkinProperties().TooltipFont.Color.R = 64;
	SkinProperties().TooltipFont.Color.G = 64;
	SkinProperties().TooltipFont.Color.B = 64;
	SkinProperties().TooltipFont.Alignment = 0;
	SkinProperties().TooltipBorder = 4;
	SkinProperties().TooltipBackground.R = 192;
	SkinProperties().TooltipBackground.G = 192;
	SkinProperties().TooltipBackground.B = 192;
	SkinProperties().TooltipFrame.R = 0;
	SkinProperties().TooltipFrame.G = 0;
	SkinProperties().TooltipFrame.B = 0;
	SkinProperties().HighlightColor.R = 248;
	SkinProperties().HighlightColor.G = 248;
	SkinProperties().HighlightColor.B = 248;
	SkinProperties().AttackColor.R = 224;
	SkinProperties().AttackColor.G = 64;
	SkinProperties().AttackColor.B = 64;
	SkinProperties().FollowColor.R = 64;
	SkinProperties().FollowColor.G = 224;
	SkinProperties().FollowColor.B = 64;
	SkinProperties().HighlightedAttackColor.R = 248;
	SkinProperties().HighlightedAttackColor.G = 164;
	SkinProperties().HighlightedAttackColor.B = 164;
	SkinProperties().HighlightedFollowColor.R = 180;
	SkinProperties().HighlightedFollowColor.G = 248;
	SkinProperties().HighlightedFollowColor.B = 180;
	SkinProperties().MenuShortcutGap = 40;
	SkinProperties().MenuMargins.x = 4;
	SkinProperties().MenuMargins.y = 4;
	SkinProperties().MenuMargins.width = -4;
	SkinProperties().MenuMargins.height = -4;
	SkinProperties().MenuFont.FontNumber = FONT_BORDERED;
	SkinProperties().MenuFont.Color.R = 224;
	SkinProperties().MenuFont.Color.G = 224;
	SkinProperties().MenuFont.Color.B = 224;
	SkinProperties().MenuFont.Alignment = 0;
	SkinProperties().MenuSeparatorHeight = 8;
	SkinProperties().MenuHighlightBackground.R = 128;
	SkinProperties().MenuHighlightBackground.G = 128;
	SkinProperties().MenuHighlightBackground.B = 128;
	SkinProperties().MenuTextMargin = 2;
	SkinProperties().StartWindow.PanelWidth = 86;
	SkinProperties().StartWindow.PanelHeight = 140;
	SkinProperties().StartWindow.FramePosition.x = 60;
	SkinProperties().StartWindow.FramePosition.y = -240;
	SkinProperties().StartWindow.EnterGameButton.x = 0;
	SkinProperties().StartWindow.EnterGameButton.y = 0;
	SkinProperties().StartWindow.OptionsButton.x = 0;
	SkinProperties().StartWindow.OptionsButton.y = 60;
	SkinProperties().StartWindow.InfoButton.x = 0;
	SkinProperties().StartWindow.InfoButton.y = 90;
	SkinProperties().StartWindow.ExitGameButton.x = 0;
	SkinProperties().StartWindow.ExitGameButton.y = 120;
	SkinProperties().StartWindow.AccessAccountButton.x = 0;
	SkinProperties().StartWindow.AccessAccountButton.y = 30;
	SkinProperties().StartWindowBorder = 10;
	SkinProperties().SplitterHeight = 5;
	SkinProperties().MapFrameBorder = 5;
	SkinProperties().MapHintButton.x = -35;
	SkinProperties().MapHintButton.y = 5;
	SkinProperties().MapHintButton.width = 32;
	SkinProperties().MapHintButton.height = 32;
	SkinProperties().HelpBubbleMinimumWidth = 43;
	SkinProperties().HelpBubbleMinimumHeight = 43;
	SkinProperties().HelpBubbleMaximumWidth = 322;
	SkinProperties().HelpBubbleMaximumHeight = 447;
	SkinProperties().HelpBubbleTextMargins.x = 4;
	SkinProperties().HelpBubbleTextMargins.y = 4;
	SkinProperties().HelpBubbleTextMargins.width = -4;
	SkinProperties().HelpBubbleTextMargins.height = -4;
	SkinProperties().HelpBubbleSizeStep = 4;
	SkinProperties().HelpBubbleSizeRemainder = 3;
	SkinProperties().HelpBubbleMargin = 3;
	SkinProperties().FormattedMessageWidth = 400;
	SkinProperties().FormattedMessageHeight = 340;
	SkinProperties().SideBarWidth = 176;
	SkinProperties().DraggedWindowPlaceColor.R = 0;
	SkinProperties().DraggedWindowPlaceColor.G = 0;
	SkinProperties().DraggedWindowPlaceColor.B = 0;
	SkinProperties().SubWindow.Icon.x = 4;
	SkinProperties().SubWindow.Icon.y = 2;
	SkinProperties().SubWindow.Icon.width = 12;
	SkinProperties().SubWindow.Icon.height = 12;
	SkinProperties().SubWindow.CloseButton.x = -15;
	SkinProperties().SubWindow.CloseButton.y = 2;
	SkinProperties().SubWindow.MinimizeButton.x = -28;
	SkinProperties().SubWindow.MinimizeButton.y = 2;
	SkinProperties().SubWindow.UpButton.x = -39;
	SkinProperties().SubWindow.UpButton.y = 2;
	SkinProperties().SubWindow.Title.x = 20;
	SkinProperties().SubWindow.Title.y = 4;
	SkinProperties().SubWindow.Title.width = -41;
	SkinProperties().SubWindow.Title.height = 10;
	SkinProperties().SubWindow.TitleFont.FontNumber = FONT_SMOOTH;
	SkinProperties().SubWindow.TitleFont.Color.R = 144;
	SkinProperties().SubWindow.TitleFont.Color.G = 144;
	SkinProperties().SubWindow.TitleFont.Color.B = 144;
	SkinProperties().SubWindow.TitleFont.Alignment = 0;
	SkinProperties().SubWindow.MinimizedHeight = 19;
	SkinProperties().SubWindow.Body.x = 4;
	SkinProperties().SubWindow.Body.y = 15;
	SkinProperties().SubWindow.Body.width = -4;
	SkinProperties().SubWindow.Body.height = -4;
	SkinProperties().SubWindow.Resizer1.x = 3;
	SkinProperties().SubWindow.Resizer1.y = -16;
	SkinProperties().SubWindow.Resizer1.width = 13;
	SkinProperties().SubWindow.Resizer1.height = 13;
	SkinProperties().SubWindow.Resizer2.x = 0;
	SkinProperties().SubWindow.Resizer2.y = -4;
	SkinProperties().SubWindow.Resizer2.width = 0;
	SkinProperties().SubWindow.Resizer2.height = 0;
	SkinProperties().SubWindow.IconHighlight.R = 56;
	SkinProperties().SubWindow.IconHighlight.G = 56;
	SkinProperties().SubWindow.IconHighlight.B = 56;
	SkinProperties().AutomapWindowHeight = 117;
	SkinProperties().AutomapFrame.x = 8;
	SkinProperties().AutomapFrame.y = 4;
	SkinProperties().AutomapFrame.width = 108;
	SkinProperties().AutomapFrame.height = 108;
	SkinProperties().AutomapView.x = 9;
	SkinProperties().AutomapView.y = 5;
	SkinProperties().AutomapView.width = 106;
	SkinProperties().AutomapView.height = 106;
	SkinProperties().AutomapZoomButtons.x = 124;
	SkinProperties().AutomapZoomButtons.y = 71;
	SkinProperties().AutomapZoomButtons.width = 124;
	SkinProperties().AutomapZoomButtons.height = 50;
	SkinProperties().AutomapFloorButtons.x = 147;
	SkinProperties().AutomapFloorButtons.y = 50;
	SkinProperties().AutomapFloorButtons.width = 147;
	SkinProperties().AutomapFloorButtons.height = 71;
	SkinProperties().AutomapCentreButtonAndCompassRose.x = 124;
	SkinProperties().AutomapCentreButtonAndCompassRose.y = 92;
	SkinProperties().AutomapCentreButtonAndCompassRose.width = 124;
	SkinProperties().AutomapCentreButtonAndCompassRose.height = 5;
	SkinProperties().InventoryWindowHeight = 170;
	SkinProperties().InventoryControlsAndView.x = 124;
	SkinProperties().InventoryControlsAndView.y = 19;
	SkinProperties().InventoryControlsAndView.width = 8;
	SkinProperties().InventoryControlsAndView.height = 4;
	SkinProperties().InventoryView.Width = 108;
	SkinProperties().InventoryView.Height = 163;
	SkinProperties().InventoryView.MinimiseButton.x = 0;
	SkinProperties().InventoryView.MinimiseButton.y = 0;
	SkinProperties().InventoryView.Objects[0].x = 38;
	SkinProperties().InventoryView.Objects[0].y = 1;
	SkinProperties().InventoryView.Objects[1].x = 1;
	SkinProperties().InventoryView.Objects[1].y = 15;
	SkinProperties().InventoryView.Objects[2].x = 75;
	SkinProperties().InventoryView.Objects[2].y = 15;
	SkinProperties().InventoryView.Objects[3].x = 38;
	SkinProperties().InventoryView.Objects[3].y = 38;
	SkinProperties().InventoryView.Objects[4].x = 75;
	SkinProperties().InventoryView.Objects[4].y = 52;
	SkinProperties().InventoryView.Objects[5].x = 1;
	SkinProperties().InventoryView.Objects[5].y = 52;
	SkinProperties().InventoryView.Objects[6].x = 38;
	SkinProperties().InventoryView.Objects[6].y = 75;
	SkinProperties().InventoryView.Objects[7].x = 38;
	SkinProperties().InventoryView.Objects[7].y = 112;
	SkinProperties().InventoryView.Objects[8].x = 1;
	SkinProperties().InventoryView.Objects[8].y = 89;
	SkinProperties().InventoryView.Objects[9].x = 75;
	SkinProperties().InventoryView.Objects[9].y = 89;
	SkinProperties().InventoryView.Slots[0].x = 37;
	SkinProperties().InventoryView.Slots[0].y = 0;
	SkinProperties().InventoryView.Slots[0].width = 34;
	SkinProperties().InventoryView.Slots[0].height = 34;
	SkinProperties().InventoryView.Slots[1].x = 0;
	SkinProperties().InventoryView.Slots[1].y = 14;
	SkinProperties().InventoryView.Slots[1].width = 34;
	SkinProperties().InventoryView.Slots[1].height = 34;
	SkinProperties().InventoryView.Slots[2].x = 74;
	SkinProperties().InventoryView.Slots[2].y = 14;
	SkinProperties().InventoryView.Slots[2].width = 34;
	SkinProperties().InventoryView.Slots[2].height = 34;
	SkinProperties().InventoryView.Slots[3].x = 37;
	SkinProperties().InventoryView.Slots[3].y = 37;
	SkinProperties().InventoryView.Slots[3].width = 34;
	SkinProperties().InventoryView.Slots[3].height = 34;
	SkinProperties().InventoryView.Slots[4].x = 74;
	SkinProperties().InventoryView.Slots[4].y = 51;
	SkinProperties().InventoryView.Slots[4].width = 34;
	SkinProperties().InventoryView.Slots[4].height = 34;
	SkinProperties().InventoryView.Slots[5].x = 0;
	SkinProperties().InventoryView.Slots[5].y = 51;
	SkinProperties().InventoryView.Slots[5].width = 34;
	SkinProperties().InventoryView.Slots[5].height = 34;
	SkinProperties().InventoryView.Slots[6].x = 37;
	SkinProperties().InventoryView.Slots[6].y = 74;
	SkinProperties().InventoryView.Slots[6].width = 34;
	SkinProperties().InventoryView.Slots[6].height = 34;
	SkinProperties().InventoryView.Slots[7].x = 37;
	SkinProperties().InventoryView.Slots[7].y = 111;
	SkinProperties().InventoryView.Slots[7].width = 34;
	SkinProperties().InventoryView.Slots[7].height = 34;
	SkinProperties().InventoryView.Slots[8].x = 0;
	SkinProperties().InventoryView.Slots[8].y = 88;
	SkinProperties().InventoryView.Slots[8].width = 34;
	SkinProperties().InventoryView.Slots[8].height = 34;
	SkinProperties().InventoryView.Slots[9].x = 74;
	SkinProperties().InventoryView.Slots[9].y = 88;
	SkinProperties().InventoryView.Slots[9].width = 34;
	SkinProperties().InventoryView.Slots[9].height = 34;
	SkinProperties().InventoryView.EmptySlotSkins[0] = SKIN_EMPTY_SLOT_1;
	SkinProperties().InventoryView.EmptySlotSkins[1] = SKIN_EMPTY_SLOT_2;
	SkinProperties().InventoryView.EmptySlotSkins[2] = SKIN_EMPTY_SLOT_3;
	SkinProperties().InventoryView.EmptySlotSkins[3] = SKIN_EMPTY_SLOT_4;
	SkinProperties().InventoryView.EmptySlotSkins[4] = SKIN_EMPTY_SLOT_5;
	SkinProperties().InventoryView.EmptySlotSkins[5] = SKIN_EMPTY_SLOT_6;
	SkinProperties().InventoryView.EmptySlotSkins[6] = SKIN_EMPTY_SLOT_7;
	SkinProperties().InventoryView.EmptySlotSkins[7] = SKIN_EMPTY_SLOT_8;
	SkinProperties().InventoryView.EmptySlotSkins[8] = SKIN_EMPTY_SLOT_9;
	SkinProperties().InventoryView.EmptySlotSkins[9] = SKIN_EMPTY_SLOT_10;
	SkinProperties().InventoryView.Capacity.x = 74;
	SkinProperties().InventoryView.Capacity.y = 124;
	SkinProperties().InventoryView.Capacity.width = 34;
	SkinProperties().InventoryView.Capacity.height = 21;
	SkinProperties().InventoryView.Soul.x = 0;
	SkinProperties().InventoryView.Soul.y = 124;
	SkinProperties().InventoryView.Soul.width = 34;
	SkinProperties().InventoryView.Soul.height = 21;
	SkinProperties().InventoryView.States.x = 0;
	SkinProperties().InventoryView.States.y = 147;
	SkinProperties().InventoryView.States.width = 108;
	SkinProperties().InventoryView.States.height = 13;
	SkinProperties().CapacityLabel.Caption.x = 17;
	SkinProperties().CapacityLabel.Caption.y = 2;
	SkinProperties().CapacityLabel.Caption.width = 32;
	SkinProperties().CapacityLabel.Caption.height = 9;
	SkinProperties().CapacityLabel.CaptionFont.FontNumber = FONT_SMALL;
	SkinProperties().CapacityLabel.CaptionFont.Color.R = 255;
	SkinProperties().CapacityLabel.CaptionFont.Color.G = 255;
	SkinProperties().CapacityLabel.CaptionFont.Color.B = 255;
	SkinProperties().CapacityLabel.CaptionFont.Alignment = 1;
	SkinProperties().CapacityLabel.Value.x = 17;
	SkinProperties().CapacityLabel.Value.y = 11;
	SkinProperties().CapacityLabel.Value.width = 32;
	SkinProperties().CapacityLabel.Value.height = 9;
	SkinProperties().CapacityLabel.ValueFont.FontNumber = FONT_SMOOTH;
	SkinProperties().CapacityLabel.ValueFont.Color.R = 176;
	SkinProperties().CapacityLabel.ValueFont.Color.G = 176;
	SkinProperties().CapacityLabel.ValueFont.Color.B = 176;
	SkinProperties().CapacityLabel.ValueFont.Alignment = 1;
	SkinProperties().SoulLabel.Caption.x = 17;
	SkinProperties().SoulLabel.Caption.y = 2;
	SkinProperties().SoulLabel.Caption.width = 32;
	SkinProperties().SoulLabel.Caption.height = 9;
	SkinProperties().SoulLabel.CaptionFont.FontNumber = FONT_SMALL;
	SkinProperties().SoulLabel.CaptionFont.Color.R = 255;
	SkinProperties().SoulLabel.CaptionFont.Color.G = 255;
	SkinProperties().SoulLabel.CaptionFont.Color.B = 255;
	SkinProperties().SoulLabel.CaptionFont.Alignment = 1;
	SkinProperties().SoulLabel.Value.x = 17;
	SkinProperties().SoulLabel.Value.y = 11;
	SkinProperties().SoulLabel.Value.width = 32;
	SkinProperties().SoulLabel.Value.height = 9;
	SkinProperties().SoulLabel.ValueFont.FontNumber = FONT_SMOOTH;
	SkinProperties().SoulLabel.ValueFont.Color.R = 176;
	SkinProperties().SoulLabel.ValueFont.Color.G = 176;
	SkinProperties().SoulLabel.ValueFont.Color.B = 176;
	SkinProperties().SoulLabel.ValueFont.Alignment = 1;
	SkinProperties().PlayerStates.LargeRow.x = 2;
	SkinProperties().PlayerStates.LargeRow.y = 2;
	SkinProperties().PlayerStates.LargeRow.width = 104;
	SkinProperties().PlayerStates.LargeRow.height = 9;
	SkinProperties().PlayerStates.SmallRow.x = 2;
	SkinProperties().PlayerStates.SmallRow.y = 2;
	SkinProperties().PlayerStates.SmallRow.width = 90;
	SkinProperties().PlayerStates.SmallRow.height = 9;
	SkinProperties().PlayerStates.NumberOfIcons = 9;
	SkinProperties().PlayerStates.IconDistance = 10;
	SkinProperties().ObjectCountFont.FontNumber = FONT_BORDERED;
	SkinProperties().ObjectCountFont.Color.R = 192;
	SkinProperties().ObjectCountFont.Color.G = 192;
	SkinProperties().ObjectCountFont.Color.B = 192;
	SkinProperties().ObjectCountFont.Alignment = 2;
	SkinProperties().LargeCombatControls.Height = 134;
	SkinProperties().LargeCombatControls.Width = 43;
	SkinProperties().LargeCombatControls.Buttons[0].x = 0;
	SkinProperties().LargeCombatControls.Buttons[0].y = 0;
	SkinProperties().LargeCombatControls.Buttons[1].x = 0;
	SkinProperties().LargeCombatControls.Buttons[1].y = 20;
	SkinProperties().LargeCombatControls.Buttons[2].x = 0;
	SkinProperties().LargeCombatControls.Buttons[2].y = 40;
	SkinProperties().LargeCombatControls.Buttons[3].x = 23;
	SkinProperties().LargeCombatControls.Buttons[3].y = 0;
	SkinProperties().LargeCombatControls.Buttons[4].x = 23;
	SkinProperties().LargeCombatControls.Buttons[4].y = 20;
	SkinProperties().LargeCombatControls.Buttons[5].x = 23;
	SkinProperties().LargeCombatControls.Buttons[5].y = 40;
	SkinProperties().LargeCombatControls.Buttons[6].x = 0;
	SkinProperties().LargeCombatControls.Buttons[6].y = 62;
	SkinProperties().LargeCombatControls.Buttons[7].x = 0;
	SkinProperties().LargeCombatControls.Buttons[7].y = 86;
	SkinProperties().LargeCombatControls.Buttons[8].x = 0;
	SkinProperties().LargeCombatControls.Buttons[8].y = 108;
	SkinProperties().SmallCombatControls.Height = 42;
	SkinProperties().SmallCombatControls.Width = 111;
	SkinProperties().SmallCombatControls.Buttons[0].x = 0;
	SkinProperties().SmallCombatControls.Buttons[0].y = 0;
	SkinProperties().SmallCombatControls.Buttons[1].x = 20;
	SkinProperties().SmallCombatControls.Buttons[1].y = 0;
	SkinProperties().SmallCombatControls.Buttons[2].x = 40;
	SkinProperties().SmallCombatControls.Buttons[2].y = 0;
	SkinProperties().SmallCombatControls.Buttons[3].x = 0;
	SkinProperties().SmallCombatControls.Buttons[3].y = 22;
	SkinProperties().SmallCombatControls.Buttons[4].x = 20;
	SkinProperties().SmallCombatControls.Buttons[4].y = 22;
	SkinProperties().SmallCombatControls.Buttons[5].x = 40;
	SkinProperties().SmallCombatControls.Buttons[5].y = 22;
	SkinProperties().SmallCombatControls.Buttons[6].x = 68;
	SkinProperties().SmallCombatControls.Buttons[6].y = 0;
	SkinProperties().SmallCombatControls.Buttons[7].x = 68;
	SkinProperties().SmallCombatControls.Buttons[7].y = 22;
	SkinProperties().SmallCombatControls.Buttons[8].x = 90;
	SkinProperties().SmallCombatControls.Buttons[8].y = 22;
	SkinProperties().SmallInventoryWindowHeight = 64;
	SkinProperties().SmallInventoryControlsAndMaximise.x = 56;
	SkinProperties().SmallInventoryControlsAndMaximise.y = 4;
	SkinProperties().SmallInventoryControlsAndMaximise.width = 8;
	SkinProperties().SmallInventoryControlsAndMaximise.height = 4;
	SkinProperties().SmallInventoryCapacity.x = 22;
	SkinProperties().SmallInventoryCapacity.y = 4;
	SkinProperties().SmallInventoryCapacity.width = 32;
	SkinProperties().SmallInventoryCapacity.height = 21;
	SkinProperties().SmallInventorySoul.x = 22;
	SkinProperties().SmallInventorySoul.y = 25;
	SkinProperties().SmallInventorySoul.width = 32;
	SkinProperties().SmallInventorySoul.height = 21;
	SkinProperties().SmallInventoryStates.x = 22;
	SkinProperties().SmallInventoryStates.y = 48;
	SkinProperties().SmallInventoryStates.width = 94;
	SkinProperties().SmallInventoryStates.height = 13;
	SkinProperties().SmallInventoryDropArea.x = 22;
	SkinProperties().SmallInventoryDropArea.y = 4;
	SkinProperties().SmallInventoryDropArea.width = 32;
	SkinProperties().SmallInventoryDropArea.height = 42;
	SkinProperties().StatusWindow.Height = 32;
	SkinProperties().StatusWindow.Pictures.x = 9;
	SkinProperties().StatusWindow.Pictures.y = 5;
	SkinProperties().StatusWindow.Pictures.width = 9;
	SkinProperties().StatusWindow.Pictures.height = 18;
	SkinProperties().StatusWindow.HitpointsBar.x = 26;
	SkinProperties().StatusWindow.HitpointsBar.y = 4;
	SkinProperties().StatusWindow.HitpointsBar.width = 90;
	SkinProperties().StatusWindow.HitpointsBar.height = 11;
	SkinProperties().StatusWindow.ManaBar.x = 26;
	SkinProperties().StatusWindow.ManaBar.y = 17;
	SkinProperties().StatusWindow.ManaBar.width = 90;
	SkinProperties().StatusWindow.ManaBar.height = 11;
	SkinProperties().StatusWindow.HitpointsValue.x = 127;
	SkinProperties().StatusWindow.HitpointsValue.y = 5;
	SkinProperties().StatusWindow.HitpointsValue.width = 39;
	SkinProperties().StatusWindow.HitpointsValue.height = 9;
	SkinProperties().StatusWindow.ManaValue.x = 127;
	SkinProperties().StatusWindow.ManaValue.y = 18;
	SkinProperties().StatusWindow.ManaValue.width = 39;
	SkinProperties().StatusWindow.ManaValue.height = 9;
	SkinProperties().StatusWindow.ValueFont.FontNumber = FONT_SMOOTH;
	SkinProperties().StatusWindow.ValueFont.Color.R = 176;
	SkinProperties().StatusWindow.ValueFont.Color.G = 176;
	SkinProperties().StatusWindow.ValueFont.Color.B = 176;
	SkinProperties().StatusWindow.ValueFont.Alignment = 0;
	SkinProperties().ButtonsWindowHeight = 26;
	SkinProperties().ButtonsWindowSkillsAndBattle.x = 8;
	SkinProperties().ButtonsWindowSkillsAndBattle.y = 3;
	SkinProperties().ButtonsWindowSkillsAndBattle.width = 45;
	SkinProperties().ButtonsWindowSkillsAndBattle.height = 3;
	SkinProperties().ButtonsWindowBuddyAndLogout.x = 82;
	SkinProperties().ButtonsWindowBuddyAndLogout.y = 3;
	SkinProperties().ButtonsWindowBuddyAndLogout.width = 124;
	SkinProperties().ButtonsWindowBuddyAndLogout.height = 3;
	SkinProperties().ContainerSlotsPerRow = 4;
	SkinProperties().ContainerFirstSlot.x = 7;
	SkinProperties().ContainerFirstSlot.y = 4;
	SkinProperties().ContainerSlotSizeAndObject.x = 34;
	SkinProperties().ContainerSlotSizeAndObject.y = 34;
	SkinProperties().ContainerSlotSizeAndObject.width = 8;
	SkinProperties().ContainerSlotSizeAndObject.height = 5;
	SkinProperties().ContainerColumnWidth = 37;
	SkinProperties().ContainerRowHeight = 37;
	SkinProperties().ContainerMinimumHeight = 41;
	SkinProperties().SkillsFont.FontNumber = FONT_SMOOTH;
	SkinProperties().SkillsFont.Color.R = 176;
	SkinProperties().SkillsFont.Color.G = 176;
	SkinProperties().SkillsFont.Color.B = 176;
	SkinProperties().SkillsFont.Alignment = 0;
	SkinProperties().SkillsHorizontalMargin = 10;
	SkinProperties().SkillsVerticalMargin = 10;
	SkinProperties().SkillsFirstBarsColor.R = 192;
	SkinProperties().SkillsFirstBarsColor.G = 0;
	SkinProperties().SkillsFirstBarsColor.B = 0;
	SkinProperties().SkillsOtherBarsColor.R = 0;
	SkinProperties().SkillsOtherBarsColor.G = 192;
	SkinProperties().SkillsOtherBarsColor.B = 0;
	SkinProperties().BattleWindow.RowHeight = 22;
	SkinProperties().BattleWindow.Outfit.x = 2;
	SkinProperties().BattleWindow.Outfit.y = 2;
	SkinProperties().BattleWindow.Outfit.width = 18;
	SkinProperties().BattleWindow.Outfit.height = 18;
	SkinProperties().BattleWindow.Name.x = 23;
	SkinProperties().BattleWindow.Name.y = 2;
	SkinProperties().BattleWindow.Name.width = 110;
	SkinProperties().BattleWindow.Name.height = 11;
	SkinProperties().BattleWindow.FlagOffset = 13;
	SkinProperties().BattleWindow.HealthBar.x = 23;
	SkinProperties().BattleWindow.HealthBar.y = 14;
	SkinProperties().BattleWindow.HealthBar.width = 122;
	SkinProperties().BattleWindow.HealthBar.height = 5;
	SkinProperties().BattleWindow.Frame.x = 1;
	SkinProperties().BattleWindow.Frame.y = 1;
	SkinProperties().BattleWindow.Frame.width = 20;
	SkinProperties().BattleWindow.Frame.height = 20;
	SkinProperties().BattleWindow.HealthBarFrameColor.R = 0;
	SkinProperties().BattleWindow.HealthBarFrameColor.G = 0;
	SkinProperties().BattleWindow.HealthBarFrameColor.B = 0;
	SkinProperties().BattleWindow.NameFont.FontNumber = FONT_SMOOTH;
	SkinProperties().BattleWindow.NameFont.Color.R = 176;
	SkinProperties().BattleWindow.NameFont.Color.G = 176;
	SkinProperties().BattleWindow.NameFont.Color.B = 176;
	SkinProperties().BattleWindow.NameFont.Alignment = 0;
	SkinProperties().BattleWindow.OutfitHighlight.R = 71;
	SkinProperties().BattleWindow.OutfitHighlight.G = 71;
	SkinProperties().BattleWindow.OutfitHighlight.B = 71;
	SkinProperties().TradeWindow.MinimumHeight = 78;
	SkinProperties().TradeWindow.Names.x = 0;
	SkinProperties().TradeWindow.Names.y = 0;
	SkinProperties().TradeWindow.Names.width = 0;
	SkinProperties().TradeWindow.Names.height = 12;
	SkinProperties().TradeWindow.OwnName.x = 3;
	SkinProperties().TradeWindow.OwnName.y = 2;
	SkinProperties().TradeWindow.OwnName.width = 71;
	SkinProperties().TradeWindow.OwnName.height = 9;
	SkinProperties().TradeWindow.CounterName.x = 82;
	SkinProperties().TradeWindow.CounterName.y = 2;
	SkinProperties().TradeWindow.CounterName.width = 71;
	SkinProperties().TradeWindow.CounterName.height = 9;
	SkinProperties().TradeWindow.NamesSeparator.x = 0;
	SkinProperties().TradeWindow.NamesSeparator.y = -2;
	SkinProperties().TradeWindow.NamesSeparator.width = 0;
	SkinProperties().TradeWindow.NamesSeparator.height = 2;
	SkinProperties().TradeWindow.NameFont.FontNumber = FONT_SMALL;
	SkinProperties().TradeWindow.NameFont.Color.R = 255;
	SkinProperties().TradeWindow.NameFont.Color.G = 255;
	SkinProperties().TradeWindow.NameFont.Color.B = 255;
	SkinProperties().TradeWindow.NameFont.Alignment = 0;
	SkinProperties().TradeWindow.Offers.x = 0;
	SkinProperties().TradeWindow.Offers.y = 12;
	SkinProperties().TradeWindow.Offers.width = 0;
	SkinProperties().TradeWindow.Offers.height = -26;
	SkinProperties().TradeWindow.OwnOffer.x = 3;
	SkinProperties().TradeWindow.OwnOffer.y = 3;
	SkinProperties().TradeWindow.OwnOffer.width = 34;
	SkinProperties().TradeWindow.OwnOffer.height = 34;
	SkinProperties().TradeWindow.CounterOffer.x = 82;
	SkinProperties().TradeWindow.CounterOffer.y = 3;
	SkinProperties().TradeWindow.CounterOffer.width = 34;
	SkinProperties().TradeWindow.CounterOffer.height = 34;
	SkinProperties().TradeWindow.ObjectOffsets.x = 4;
	SkinProperties().TradeWindow.ObjectOffsets.y = 4;
	SkinProperties().TradeWindow.ObjectOffsets.width = 83;
	SkinProperties().TradeWindow.ObjectOffsets.height = 4;
	SkinProperties().TradeWindow.ObjectSize = 32;
	SkinProperties().TradeWindow.RowHeight = 36;
	SkinProperties().TradeWindow.ColumnWidth = 36;
	SkinProperties().TradeWindow.SlotsPerRow = 2;
	SkinProperties().TradeWindow.OffersSeparator.x = 77;
	SkinProperties().TradeWindow.OffersSeparator.y = 0;
	SkinProperties().TradeWindow.OffersSeparator.width = 2;
	SkinProperties().TradeWindow.OffersSeparator.height = 0;
	SkinProperties().TradeWindow.Buttons.x = 0;
	SkinProperties().TradeWindow.Buttons.y = -26;
	SkinProperties().TradeWindow.Buttons.width = 0;
	SkinProperties().TradeWindow.Buttons.height = 0;
	SkinProperties().TradeWindow.AcceptAndCancelButtons.x = 74;
	SkinProperties().TradeWindow.AcceptAndCancelButtons.y = 4;
	SkinProperties().TradeWindow.AcceptAndCancelButtons.width = 121;
	SkinProperties().TradeWindow.AcceptAndCancelButtons.height = 4;
	SkinProperties().TradeWindow.WaitLabel.x = 4;
	SkinProperties().TradeWindow.WaitLabel.y = 4;
	SkinProperties().TradeWindow.WaitLabel.width = 113;
	SkinProperties().TradeWindow.WaitLabel.height = 20;
	SkinProperties().TradeWindow.ButtonsSeparator.x = 0;
	SkinProperties().TradeWindow.ButtonsSeparator.y = 0;
	SkinProperties().TradeWindow.ButtonsSeparator.width = 0;
	SkinProperties().TradeWindow.ButtonsSeparator.height = 2;
	SkinProperties().TradeWindow.WaitFont.FontNumber = FONT_SMALL;
	SkinProperties().TradeWindow.WaitFont.Color.R = 255;
	SkinProperties().TradeWindow.WaitFont.Color.G = 255;
	SkinProperties().TradeWindow.WaitFont.Color.B = 255;
	SkinProperties().TradeWindow.WaitFont.Alignment = 1;
	SkinProperties().BuddylistWindow.Data00 = 32;
	SkinProperties().BuddylistWindow.RowHeight = 16;
	SkinProperties().BuddylistWindow.Logo.x = 2;
	SkinProperties().BuddylistWindow.Logo.y = 2;
	SkinProperties().BuddylistWindow.Logo.width = 12;
	SkinProperties().BuddylistWindow.Logo.height = 12;
	SkinProperties().BuddylistWindow.Name.x = 20;
	SkinProperties().BuddylistWindow.Name.y = 4;
	SkinProperties().BuddylistWindow.Name.width = 132;
	SkinProperties().BuddylistWindow.Name.height = 13;
	SkinProperties().BuddylistWindow.NameFont.FontNumber = FONT_SMOOTH;
	SkinProperties().BuddylistWindow.NameFont.Color.R = 224;
	SkinProperties().BuddylistWindow.NameFont.Color.G = 224;
	SkinProperties().BuddylistWindow.NameFont.Color.B = 224;
	SkinProperties().BuddylistWindow.NameFont.Alignment = 0;
	SkinProperties().BuddylistWindow.OnlineColor.R = 96;
	SkinProperties().BuddylistWindow.OnlineColor.G = 248;
	SkinProperties().BuddylistWindow.OnlineColor.B = 96;
	SkinProperties().BuddylistWindow.OfflineColor.R = 248;
	SkinProperties().BuddylistWindow.OfflineColor.G = 96;
	SkinProperties().BuddylistWindow.OfflineColor.B = 96;
	SkinProperties().BuddylistWindow.NewColor.R = 248;
	SkinProperties().BuddylistWindow.NewColor.G = 248;
	SkinProperties().BuddylistWindow.NewColor.B = 248;
	SkinProperties().Data868 = 10;
	SkinProperties().ScrollBarRangeMinimumHeight = 14;
	SkinProperties().ScrollBarMinimumWidth = 12;
	SkinProperties().ScrollBarMinimumHeight = 38;
	SkinProperties().ScrollBarWidth = 12;
	SkinProperties().HorizontalScrollBarRangeMinimumWidth = 14;
	SkinProperties().HorizontalScrollBarHeight = 12;
	SkinProperties().HorizontalScrollBarMinimumWidth = 38;
	SkinProperties().Data888 = 12;
	SkinProperties().Buttons.Font.FontNumber = FONT_SMALL;
	SkinProperties().Buttons.Font.Color.R = 255;
	SkinProperties().Buttons.Font.Color.G = 255;
	SkinProperties().Buttons.Font.Color.B = 255;
	SkinProperties().Buttons.Font.Alignment = 1;
	SkinProperties().Buttons.DisabledFont.FontNumber = FONT_SMALL;
	SkinProperties().Buttons.DisabledFont.Color.R = 192;
	SkinProperties().Buttons.DisabledFont.Color.G = 192;
	SkinProperties().Buttons.DisabledFont.Color.B = 192;
	SkinProperties().Buttons.DisabledFont.Alignment = 1;
	SkinProperties().Buttons.PressedOffset = 1;
	SkinProperties().Buttons.Texture.x = 2;
	SkinProperties().Buttons.Texture.y = 2;
	SkinProperties().BlinkingButtonInterval = 100;
	SkinProperties().Style8C4.Data00.FontNumber = FONT_SMOOTH;
	SkinProperties().Style8C4.Data00.Color.R = 224;
	SkinProperties().Style8C4.Data00.Color.G = 224;
	SkinProperties().Style8C4.Data00.Color.B = 224;
	SkinProperties().Style8C4.Data00.Alignment = 0;
	SkinProperties().Style8C4.Data14.FontNumber = FONT_SMOOTH;
	SkinProperties().Style8C4.Data14.Color.R = 255;
	SkinProperties().Style8C4.Data14.Color.G = 255;
	SkinProperties().Style8C4.Data14.Color.B = 255;
	SkinProperties().Style8C4.Data14.Alignment = 0;
	SkinProperties().Style8C4.Data28.FontNumber = FONT_SMOOTH;
	SkinProperties().Style8C4.Data28.Color.R = 160;
	SkinProperties().Style8C4.Data28.Color.G = 160;
	SkinProperties().Style8C4.Data28.Color.B = 255;
	SkinProperties().Style8C4.Data28.Alignment = 0;
	SkinProperties().CheckBox.Item.x = 6;
	SkinProperties().CheckBox.Item.y = 5;
	SkinProperties().CheckBox.Label.x = 24;
	SkinProperties().CheckBox.Label.y = 4;
	SkinProperties().CheckBox.Label.width = -10;
	SkinProperties().CheckBox.Label.height = -6;
	SkinProperties().CheckBox.Font.FontNumber = FONT_SMOOTH;
	SkinProperties().CheckBox.Font.Color.R = 176;
	SkinProperties().CheckBox.Font.Color.G = 176;
	SkinProperties().CheckBox.Font.Color.B = 176;
	SkinProperties().CheckBox.Font.Alignment = 0;
	SkinProperties().CheckBox.DisabledFont.FontNumber = FONT_SMOOTH;
	SkinProperties().CheckBox.DisabledFont.Color.R = 112;
	SkinProperties().CheckBox.DisabledFont.Color.G = 112;
	SkinProperties().CheckBox.DisabledFont.Color.B = 112;
	SkinProperties().CheckBox.DisabledFont.Alignment = 0;
	SkinProperties().ObjectIconFrameWidth = 1;
	SkinProperties().EditLine.Background.R = 54;
	SkinProperties().EditLine.Background.G = 54;
	SkinProperties().EditLine.Background.B = 54;
	SkinProperties().EditLine.DisabledBackground.R = 71;
	SkinProperties().EditLine.DisabledBackground.G = 71;
	SkinProperties().EditLine.DisabledBackground.B = 71;
	SkinProperties().EditLine.CursorColor.R = 255;
	SkinProperties().EditLine.CursorColor.G = 255;
	SkinProperties().EditLine.CursorColor.B = 255;
	SkinProperties().EditLine.Font.FontNumber = FONT_SMOOTH;
	SkinProperties().EditLine.Font.Color.R = 224;
	SkinProperties().EditLine.Font.Color.G = 224;
	SkinProperties().EditLine.Font.Color.B = 224;
	SkinProperties().EditLine.Font.Alignment = 0;
	SkinProperties().EditLine.ReadOnlyFont.FontNumber = FONT_SMOOTH;
	SkinProperties().EditLine.ReadOnlyFont.Color.R = 176;
	SkinProperties().EditLine.ReadOnlyFont.Color.G = 176;
	SkinProperties().EditLine.ReadOnlyFont.Color.B = 176;
	SkinProperties().EditLine.ReadOnlyFont.Alignment = 0;
	SkinProperties().EditLine.SelectionColor.R = 128;
	SkinProperties().EditLine.SelectionColor.G = 128;
	SkinProperties().EditLine.SelectionColor.B = 128;
	SkinProperties().EditLineTextMargin = 3;
	SkinProperties().ListBox.Background.R = 64;
	SkinProperties().ListBox.Background.G = 64;
	SkinProperties().ListBox.Background.B = 64;
	SkinProperties().ListBox.Font.FontNumber = FONT_SMOOTH;
	SkinProperties().ListBox.Font.Color.R = 176;
	SkinProperties().ListBox.Font.Color.G = 176;
	SkinProperties().ListBox.Font.Color.B = 176;
	SkinProperties().ListBox.Font.Alignment = 0;
	SkinProperties().ListBox.TextMargin = 3;
	SkinProperties().ListBox.SelectedBackground.R = 112;
	SkinProperties().ListBox.SelectedBackground.G = 112;
	SkinProperties().ListBox.SelectedBackground.B = 112;
	SkinProperties().ListBox.SelectedFont.FontNumber = FONT_SMOOTH;
	SkinProperties().ListBox.SelectedFont.Color.R = 255;
	SkinProperties().ListBox.SelectedFont.Color.G = 255;
	SkinProperties().ListBox.SelectedFont.Color.B = 255;
	SkinProperties().ListBox.SelectedFont.Alignment = 0;
	SkinProperties().ListBoxSelectionMargin = 1;
	SkinProperties().ConsoleWindow.MinimumHeight = 106;
	SkinProperties().ConsoleWindow.TabBar.x = 18;
	SkinProperties().ConsoleWindow.TabBar.y = 0;
	SkinProperties().ConsoleWindow.TabBar.width = -82;
	SkinProperties().ConsoleWindow.TabBar.height = 18;
	SkinProperties().ConsoleWindow.Data14.x = 0;
	SkinProperties().ConsoleWindow.Data14.y = 0;
	SkinProperties().ConsoleWindow.Data14.width = 0;
	SkinProperties().ConsoleWindow.Data14.height = 18;
	SkinProperties().ConsoleWindow.Input.x = 23;
	SkinProperties().ConsoleWindow.Input.y = -20;
	SkinProperties().ConsoleWindow.Input.width = -4;
	SkinProperties().ConsoleWindow.Input.height = 16;
	SkinProperties().ConsoleWindow.Data34.x = 55;
	SkinProperties().ConsoleWindow.Data34.y = -20;
	SkinProperties().ConsoleWindow.Data34.width = -4;
	SkinProperties().ConsoleWindow.Data34.height = 16;
	SkinProperties().ConsoleWindow.CurrentConsole.x = 4;
	SkinProperties().ConsoleWindow.CurrentConsole.y = 21;
	SkinProperties().ConsoleWindow.CurrentConsole.width = -4;
	SkinProperties().ConsoleWindow.CurrentConsole.height = -22;
	SkinProperties().ConsoleWindow.MoveRightButton.x = 0;
	SkinProperties().ConsoleWindow.MoveRightButton.y = 0;
	SkinProperties().ConsoleWindow.MoveLeftButton.x = -82;
	SkinProperties().ConsoleWindow.MoveLeftButton.y = 0;
	SkinProperties().ConsoleWindow.VolumeButton.x = 5;
	SkinProperties().ConsoleWindow.VolumeButton.y = -20;
	SkinProperties().ConsoleWindow.CloseButton.x = -64;
	SkinProperties().ConsoleWindow.CloseButton.y = 0;
	SkinProperties().ConsoleWindow.ServerMessagesButton.x = -48;
	SkinProperties().ConsoleWindow.ServerMessagesButton.y = 0;
	SkinProperties().ConsoleWindow.IgnoreButton.x = -16;
	SkinProperties().ConsoleWindow.IgnoreButton.y = 0;
	SkinProperties().ConsoleWindow.NewChannelButton.x = -32;
	SkinProperties().ConsoleWindow.NewChannelButton.y = 0;
	SkinProperties().ConsoleWindow.TextWindow.x = 5;
	SkinProperties().ConsoleWindow.TextWindow.y = 5;
	SkinProperties().ConsoleWindow.TextWindow.width = -17;
	SkinProperties().ConsoleWindow.TextWindow.height = -5;
	SkinProperties().ConsoleWindow.SelectionColor.R = 128;
	SkinProperties().ConsoleWindow.SelectionColor.G = 128;
	SkinProperties().ConsoleWindow.SelectionColor.B = 128;
	SkinProperties().ConsoleWindow.InputFont.FontNumber = FONT_SMOOTH;
	SkinProperties().ConsoleWindow.InputFont.Color.R = 160;
	SkinProperties().ConsoleWindow.InputFont.Color.G = 160;
	SkinProperties().ConsoleWindow.InputFont.Color.B = 255;
	SkinProperties().ConsoleWindow.InputFont.Alignment = 0;
	SkinProperties().Tab.Width = 96;
	SkinProperties().Tab.Height = 18;
	SkinProperties().Tab.Caption.x = 48;
	SkinProperties().Tab.Caption.y = 4;
	SkinProperties().Tab.CaptionMargin = 4;
	SkinProperties().Tab.ActiveFont.FontNumber = FONT_BORDERED;
	SkinProperties().Tab.ActiveFont.Color.R = 224;
	SkinProperties().Tab.ActiveFont.Color.G = 224;
	SkinProperties().Tab.ActiveFont.Color.B = 224;
	SkinProperties().Tab.ActiveFont.Alignment = 1;
	SkinProperties().Tab.InactiveFont.FontNumber = FONT_BORDERED;
	SkinProperties().Tab.InactiveFont.Color.R = 128;
	SkinProperties().Tab.InactiveFont.Color.G = 128;
	SkinProperties().Tab.InactiveFont.Color.B = 128;
	SkinProperties().Tab.InactiveFont.Alignment = 1;
	SkinProperties().Tab.UnreadFont.FontNumber = FONT_BORDERED;
	SkinProperties().Tab.UnreadFont.Color.R = 248;
	SkinProperties().Tab.UnreadFont.Color.G = 96;
	SkinProperties().Tab.UnreadFont.Color.B = 96;
	SkinProperties().Tab.UnreadFont.Alignment = 1;
	SkinProperties().Tab.BlinkingFont.FontNumber = FONT_BORDERED;
	SkinProperties().Tab.BlinkingFont.Color.R = 248;
	SkinProperties().Tab.BlinkingFont.Color.G = 248;
	SkinProperties().Tab.BlinkingFont.Color.B = 248;
	SkinProperties().Tab.BlinkingFont.Alignment = 1;
	SkinProperties().Dialog.MinimumWidth = 162;
	SkinProperties().Dialog.MinimumHeight = 68;
	SkinProperties().Dialog.Caption.x = 5;
	SkinProperties().Dialog.Caption.y = 5;
	SkinProperties().Dialog.Caption.width = -5;
	SkinProperties().Dialog.Caption.height = 10;
	SkinProperties().Dialog.CaptionFont.FontNumber = FONT_SMOOTH;
	SkinProperties().Dialog.CaptionFont.Color.R = 144;
	SkinProperties().Dialog.CaptionFont.Color.G = 144;
	SkinProperties().Dialog.CaptionFont.Color.B = 144;
	SkinProperties().Dialog.CaptionFont.Alignment = 1;
	SkinProperties().Dialog.Buttons[0].x = -56;
	SkinProperties().Dialog.Buttons[0].y = -30;
	SkinProperties().Dialog.Buttons[1].x = -109;
	SkinProperties().Dialog.Buttons[1].y = -30;
	SkinProperties().Dialog.Buttons[2].x = -162;
	SkinProperties().Dialog.Buttons[2].y = -30;
	SkinProperties().Dialog.Separator.x = 13;
	SkinProperties().Dialog.Separator.y = -40;
	SkinProperties().Dialog.Separator.width = -13;
	SkinProperties().Dialog.Separator.height = 2;
	SkinProperties().Dialog.Body.x = 18;
	SkinProperties().Dialog.Body.y = 32;
	SkinProperties().Dialog.Body.width = -18;
	SkinProperties().Dialog.Body.height = -60;
	SkinProperties().Dialog.Data64.FontNumber = FONT_SMOOTH;
	SkinProperties().Dialog.Data64.Color.R = 192;
	SkinProperties().Dialog.Data64.Color.G = 192;
	SkinProperties().Dialog.Data64.Color.B = 192;
	SkinProperties().Dialog.Data64.Alignment = 0;
	SkinProperties().Dialog.Data78.FontNumber = FONT_SMOOTH;
	SkinProperties().Dialog.Data78.Color.R = 192;
	SkinProperties().Dialog.Data78.Color.G = 192;
	SkinProperties().Dialog.Data78.Color.B = 192;
	SkinProperties().Dialog.Data78.Alignment = 0;
	SkinProperties().LoginDialogWidth = 200;
	SkinProperties().LoginDialogHeight = 86;
	SkinProperties().LoginDialogAccountLabel.x = 0;
	SkinProperties().LoginDialogAccountLabel.y = 2;
	SkinProperties().LoginDialogAccountLabel.width = 107;
	SkinProperties().LoginDialogAccountLabel.height = 14;
	SkinProperties().LoginDialogAccount.x = -86;
	SkinProperties().LoginDialogAccount.y = 0;
	SkinProperties().LoginDialogAccount.width = 0;
	SkinProperties().LoginDialogAccount.height = 16;
	SkinProperties().LoginDialogPasswordLabel.x = 0;
	SkinProperties().LoginDialogPasswordLabel.y = 31;
	SkinProperties().LoginDialogPasswordLabel.width = 107;
	SkinProperties().LoginDialogPasswordLabel.height = 14;
	SkinProperties().LoginDialogPassword.x = -86;
	SkinProperties().LoginDialogPassword.y = 29;
	SkinProperties().LoginDialogPassword.width = 0;
	SkinProperties().LoginDialogPassword.height = 16;
	SkinProperties().LoginDialogLink.x = 0;
	SkinProperties().LoginDialogLink.y = 58;
	SkinProperties().LoginDialogLink.width = 0;
	SkinProperties().LoginDialogLink.height = 28;
	SkinProperties().ReconnectWindowSpacing = 13;
	SkinProperties().CharacterListDialogWidth = 200;
	SkinProperties().CharacterListDialogHeight = 202;
	SkinProperties().CharacterListDialogLabel.x = 0;
	SkinProperties().CharacterListDialogLabel.y = 0;
	SkinProperties().CharacterListDialogLabel.width = 0;
	SkinProperties().CharacterListDialogLabel.height = 14;
	SkinProperties().CharacterListDialogList.x = 0;
	SkinProperties().CharacterListDialogList.y = 15;
	SkinProperties().CharacterListDialogList.width = 0;
	SkinProperties().CharacterListDialogList.height = 146;
	SkinProperties().CharacterListDialogStatus.x = 0;
	SkinProperties().CharacterListDialogStatus.y = 174;
	SkinProperties().CharacterListDialogStatus.width = 0;
	SkinProperties().CharacterListDialogStatus.height = 28;
	SkinProperties().MoveObjectsDialogWidth = 200;
	SkinProperties().MoveObjectsDialogHeight = 44;
	SkinProperties().MoveObjectsDialogScrollBar.x = 45;
	SkinProperties().MoveObjectsDialogScrollBar.y = 32;
	SkinProperties().MoveObjectsDialogScrollBar.width = 0;
	SkinProperties().MoveObjectsDialogScrollBar.height = 12;
	SkinProperties().MoveObjectsDialogLabel.x = 45;
	SkinProperties().MoveObjectsDialogLabel.y = 2;
	SkinProperties().MoveObjectsDialogLabel.width = 0;
	SkinProperties().MoveObjectsDialogLabel.height = 28;
	SkinProperties().MoveObjectsDialogObject.x = 0;
	SkinProperties().MoveObjectsDialogObject.y = 0;
	SkinProperties().OutfitDialogWidth = 384;
	SkinProperties().OutfitDialogHeight = 133;
	SkinProperties().OutfitDialogIcon.x = 0;
	SkinProperties().OutfitDialogIcon.y = 0;
	SkinProperties().OutfitDialogIcon.width = 68;
	SkinProperties().OutfitDialogIcon.height = 68;
	SkinProperties().OutfitDialogIconBorder = 8;
	SkinProperties().OutfitDialogIconHighlight.R = 71;
	SkinProperties().OutfitDialogIconHighlight.G = 71;
	SkinProperties().OutfitDialogIconHighlight.B = 71;
	SkinProperties().OutfitDialogNextButton.x = 13;
	SkinProperties().OutfitDialogNextButton.y = 72;
	SkinProperties().OutfitDialogPartButtons[0].x = 81;
	SkinProperties().OutfitDialogPartButtons[0].y = 0;
	SkinProperties().OutfitDialogPartButtons[1].x = 81;
	SkinProperties().OutfitDialogPartButtons[1].y = 24;
	SkinProperties().OutfitDialogPartButtons[2].x = 81;
	SkinProperties().OutfitDialogPartButtons[2].y = 48;
	SkinProperties().OutfitDialogPartButtons[3].x = 81;
	SkinProperties().OutfitDialogPartButtons[3].y = 72;
	SkinProperties().OutfitDialogColorButtons.x = 137;
	SkinProperties().OutfitDialogColorButtons.y = 0;
	SkinProperties().OutfitDialogColorColumnWidth = 13;
	SkinProperties().OutfitDialogColorRowHeight = 13;
	SkinProperties().OutfitDialogLabel.x = 0;
	SkinProperties().OutfitDialogLabel.y = 105;
	SkinProperties().TextDialogWidth = 250;
	SkinProperties().TextDialogHeight = 177;
	SkinProperties().TextDialogObjectIcon.x = 0;
	SkinProperties().TextDialogObjectIcon.y = 0;
	SkinProperties().TextDialogObjectIcon.width = 32;
	SkinProperties().TextDialogObjectIcon.height = 32;
	SkinProperties().TextDialogObjectHighlight.R = 71;
	SkinProperties().TextDialogObjectHighlight.G = 71;
	SkinProperties().TextDialogObjectHighlight.B = 71;
	SkinProperties().TextDialogDescription.x = 45;
	SkinProperties().TextDialogDescription.y = 0;
	SkinProperties().TextDialogDescription.width = 0;
	SkinProperties().TextDialogDescription.height = 42;
	SkinProperties().TextDialogText.x = 0;
	SkinProperties().TextDialogText.y = 45;
	SkinProperties().TextDialogText.width = 0;
	SkinProperties().TextDialogText.height = 132;
	SkinProperties().EditListDialogWidth = 250;
	SkinProperties().EditListDialogHeight = 177;
	SkinProperties().EditListDialogPicture.x = 0;
	SkinProperties().EditListDialogPicture.y = 0;
	SkinProperties().EditListDialogPicture.width = 32;
	SkinProperties().EditListDialogPicture.height = 32;
	SkinProperties().EditListDialogDescription.x = 45;
	SkinProperties().EditListDialogDescription.y = 0;
	SkinProperties().EditListDialogDescription.width = 0;
	SkinProperties().EditListDialogDescription.height = 28;
	SkinProperties().EditListDialogText.x = 0;
	SkinProperties().EditListDialogText.y = 45;
	SkinProperties().EditListDialogText.width = 0;
	SkinProperties().EditListDialogText.height = 132;
	SkinProperties().IgnoreListDialog.Width = 498;
	SkinProperties().IgnoreListDialog.Height = 272;
	SkinProperties().IgnoreListDialog.IgnoreListCheckBox.x = 0;
	SkinProperties().IgnoreListDialog.IgnoreListCheckBox.y = 0;
	SkinProperties().IgnoreListDialog.IgnoreListCheckBox.width = 242;
	SkinProperties().IgnoreListDialog.IgnoreListCheckBox.height = 22;
	SkinProperties().IgnoreListDialog.IgnoreListLabel.x = 0;
	SkinProperties().IgnoreListDialog.IgnoreListLabel.y = 35;
	SkinProperties().IgnoreListDialog.IgnoreListLabel.width = 0;
	SkinProperties().IgnoreListDialog.IgnoreListLabel.height = 14;
	SkinProperties().IgnoreListDialog.IgnoreList.x = 0;
	SkinProperties().IgnoreListDialog.IgnoreList.y = 50;
	SkinProperties().IgnoreListDialog.IgnoreList.width = 242;
	SkinProperties().IgnoreListDialog.IgnoreList.height = 104;
	SkinProperties().IgnoreListDialog.AddIgnoreButton.x = 161;
	SkinProperties().IgnoreListDialog.AddIgnoreButton.y = 159;
	SkinProperties().IgnoreListDialog.DeleteIgnoreButton.x = 208;
	SkinProperties().IgnoreListDialog.DeleteIgnoreButton.y = 159;
	SkinProperties().IgnoreListDialog.IgnorePermanently.x = 0;
	SkinProperties().IgnoreListDialog.IgnorePermanently.y = 158;
	SkinProperties().IgnoreListDialog.IgnorePermanently.width = 146;
	SkinProperties().IgnoreListDialog.IgnorePermanently.height = 22;
	SkinProperties().IgnoreListDialog.GlobalIgnoreLabel.x = 0;
	SkinProperties().IgnoreListDialog.GlobalIgnoreLabel.y = 197;
	SkinProperties().IgnoreListDialog.GlobalIgnoreLabel.width = 0;
	SkinProperties().IgnoreListDialog.GlobalIgnoreLabel.height = 14;
	SkinProperties().IgnoreListDialog.IgnoreYelling.x = 0;
	SkinProperties().IgnoreListDialog.IgnoreYelling.y = 215;
	SkinProperties().IgnoreListDialog.IgnoreYelling.width = 242;
	SkinProperties().IgnoreListDialog.IgnoreYelling.height = 22;
	SkinProperties().IgnoreListDialog.IgnorePrivate.x = 0;
	SkinProperties().IgnoreListDialog.IgnorePrivate.y = 250;
	SkinProperties().IgnoreListDialog.IgnorePrivate.width = 242;
	SkinProperties().IgnoreListDialog.IgnorePrivate.height = 22;
	SkinProperties().IgnoreListDialog.PermanentColor.R = 192;
	SkinProperties().IgnoreListDialog.PermanentColor.G = 0;
	SkinProperties().IgnoreListDialog.PermanentColor.B = 0;
	SkinProperties().IgnoreListDialog.WhiteListCheckBox.x = 256;
	SkinProperties().IgnoreListDialog.WhiteListCheckBox.y = 0;
	SkinProperties().IgnoreListDialog.WhiteListCheckBox.width = 242;
	SkinProperties().IgnoreListDialog.WhiteListCheckBox.height = 22;
	SkinProperties().IgnoreListDialog.WhiteListLabel.x = 256;
	SkinProperties().IgnoreListDialog.WhiteListLabel.y = 35;
	SkinProperties().IgnoreListDialog.WhiteListLabel.width = 0;
	SkinProperties().IgnoreListDialog.WhiteListLabel.height = 14;
	SkinProperties().IgnoreListDialog.WhiteList.x = 256;
	SkinProperties().IgnoreListDialog.WhiteList.y = 50;
	SkinProperties().IgnoreListDialog.WhiteList.width = 242;
	SkinProperties().IgnoreListDialog.WhiteList.height = 104;
	SkinProperties().IgnoreListDialog.AddWhiteButton.x = 417;
	SkinProperties().IgnoreListDialog.AddWhiteButton.y = 159;
	SkinProperties().IgnoreListDialog.DeleteWhiteButton.x = 464;
	SkinProperties().IgnoreListDialog.DeleteWhiteButton.y = 159;
	SkinProperties().IgnoreListDialog.AllowPermanently.x = 256;
	SkinProperties().IgnoreListDialog.AllowPermanently.y = 158;
	SkinProperties().IgnoreListDialog.AllowPermanently.width = 146;
	SkinProperties().IgnoreListDialog.AllowPermanently.height = 22;
	SkinProperties().IgnoreListDialog.GlobalAllowLabel.x = 256;
	SkinProperties().IgnoreListDialog.GlobalAllowLabel.y = 197;
	SkinProperties().IgnoreListDialog.GlobalAllowLabel.width = 0;
	SkinProperties().IgnoreListDialog.GlobalAllowLabel.height = 14;
	SkinProperties().IgnoreListDialog.AllowBuddies.x = 256;
	SkinProperties().IgnoreListDialog.AllowBuddies.y = 215;
	SkinProperties().IgnoreListDialog.AllowBuddies.width = 242;
	SkinProperties().IgnoreListDialog.AllowBuddies.height = 22;
	SkinProperties().BugReportDialogWidth = 250;
	SkinProperties().BugReportDialogHeight = 161;
	SkinProperties().BugReportDialogText.x = 0;
	SkinProperties().BugReportDialogText.y = 29;
	SkinProperties().BugReportDialogText.width = 0;
	SkinProperties().BugReportDialogText.height = 132;
	SkinProperties().BugReportDialogLabel.x = 0;
	SkinProperties().BugReportDialogLabel.y = 0;
	SkinProperties().BugReportDialogLabel.width = 0;
	SkinProperties().BugReportDialogLabel.height = 28;
	SkinProperties().InfoDialog.Width = 190;
	SkinProperties().InfoDialog.Height = 111;
	SkinProperties().InfoDialog.LabelFont.FontNumber = FONT_SMOOTH;
	SkinProperties().InfoDialog.LabelFont.Color.R = 176;
	SkinProperties().InfoDialog.LabelFont.Color.G = 176;
	SkinProperties().InfoDialog.LabelFont.Color.B = 176;
	SkinProperties().InfoDialog.LabelFont.Alignment = 1;
	SkinProperties().InfoDialog.LinkFont.FontNumber = FONT_SMOOTH;
	SkinProperties().InfoDialog.LinkFont.Color.R = 176;
	SkinProperties().InfoDialog.LinkFont.Color.G = 176;
	SkinProperties().InfoDialog.LinkFont.Color.B = 176;
	SkinProperties().InfoDialog.LinkFont.Alignment = 0;
	SkinProperties().InfoDialog.Border = 8;
	SkinProperties().InfoDialog.Label.x = 0;
	SkinProperties().InfoDialog.Label.y = 0;
	SkinProperties().InfoDialog.Label.width = 0;
	SkinProperties().InfoDialog.Label.height = 70;
	SkinProperties().InfoDialog.Link.x = 0;
	SkinProperties().InfoDialog.Link.y = 91;
	SkinProperties().InfoDialog.Link.width = 0;
	SkinProperties().InfoDialog.Link.height = 20;
	SkinProperties().InfoDialog.Separator.x = 0;
	SkinProperties().InfoDialog.Separator.y = 74;
	SkinProperties().InfoDialog.Separator.width = 0;
	SkinProperties().InfoDialog.Separator.height = 2;
	SkinProperties().NewChannelDialogWidth = 200;
	SkinProperties().NewChannelDialogHeight = 163;
	SkinProperties().NewChannelDialogListLabel.x = 0;
	SkinProperties().NewChannelDialogListLabel.y = 0;
	SkinProperties().NewChannelDialogListLabel.width = 0;
	SkinProperties().NewChannelDialogListLabel.height = 14;
	SkinProperties().NewChannelDialogList.x = 0;
	SkinProperties().NewChannelDialogList.y = 15;
	SkinProperties().NewChannelDialogList.width = 0;
	SkinProperties().NewChannelDialogList.height = 104;
	SkinProperties().NewChannelDialogNameLabel.x = 0;
	SkinProperties().NewChannelDialogNameLabel.y = 132;
	SkinProperties().NewChannelDialogNameLabel.width = 0;
	SkinProperties().NewChannelDialogNameLabel.height = 14;
	SkinProperties().NewChannelDialogName.x = 0;
	SkinProperties().NewChannelDialogName.y = 147;
	SkinProperties().NewChannelDialogName.width = 0;
	SkinProperties().NewChannelDialogName.height = 16;
	SkinProperties().PrivateChannelDialogWidth = 225;
	SkinProperties().PrivateChannelDialogHeight = 31;
	SkinProperties().PrivateChannelDialogName.x = 0;
	SkinProperties().PrivateChannelDialogName.y = 15;
	SkinProperties().PrivateChannelDialogName.width = 0;
	SkinProperties().PrivateChannelDialogName.height = 16;
	SkinProperties().PrivateChannelDialogLabel.x = 0;
	SkinProperties().PrivateChannelDialogLabel.y = 0;
	SkinProperties().PrivateChannelDialogLabel.width = 0;
	SkinProperties().PrivateChannelDialogLabel.height = 14;
	SkinProperties().ReportDialogWidth = 250;
	SkinProperties().ReportDialogHeight = 87;
	SkinProperties().ReportDialogText.x = 0;
	SkinProperties().ReportDialogText.y = 71;
	SkinProperties().ReportDialogText.width = 0;
	SkinProperties().ReportDialogText.height = 16;
	SkinProperties().ReportDialogLabel.x = 0;
	SkinProperties().ReportDialogLabel.y = 0;
	SkinProperties().ReportDialogLabel.width = 0;
	SkinProperties().ReportDialogLabel.height = 70;
	SkinProperties().AddBuddyDialogWidth = 200;
	SkinProperties().AddBuddyDialogHeight = 31;
	SkinProperties().AddBuddyDialogName.x = 0;
	SkinProperties().AddBuddyDialogName.y = 15;
	SkinProperties().AddBuddyDialogName.width = 0;
	SkinProperties().AddBuddyDialogName.height = 16;
	SkinProperties().AddBuddyDialogLabel.x = 0;
	SkinProperties().AddBuddyDialogLabel.y = 0;
	SkinProperties().AddBuddyDialogLabel.width = 0;
	SkinProperties().AddBuddyDialogLabel.height = 14;
	SkinProperties().EditBuddyDialog.Width = 250;
	SkinProperties().EditBuddyDialog.Height = 174;
	SkinProperties().EditBuddyDialog.Name.x = 0;
	SkinProperties().EditBuddyDialog.Name.y = 0;
	SkinProperties().EditBuddyDialog.Name.width = 0;
	SkinProperties().EditBuddyDialog.Name.height = 14;
	SkinProperties().EditBuddyDialog.Separator.x = 0;
	SkinProperties().EditBuddyDialog.Separator.y = 15;
	SkinProperties().EditBuddyDialog.Separator.width = 0;
	SkinProperties().EditBuddyDialog.Separator.height = 2;
	SkinProperties().EditBuddyDialog.TypeLabel.x = 0;
	SkinProperties().EditBuddyDialog.TypeLabel.y = 32;
	SkinProperties().EditBuddyDialog.TypeLabel.width = 0;
	SkinProperties().EditBuddyDialog.TypeLabel.height = 14;
	SkinProperties().EditBuddyDialog.IconButtons[0].x = 0;
	SkinProperties().EditBuddyDialog.IconButtons[0].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[1].x = 20;
	SkinProperties().EditBuddyDialog.IconButtons[1].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[2].x = 40;
	SkinProperties().EditBuddyDialog.IconButtons[2].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[3].x = 60;
	SkinProperties().EditBuddyDialog.IconButtons[3].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[4].x = 80;
	SkinProperties().EditBuddyDialog.IconButtons[4].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[5].x = 100;
	SkinProperties().EditBuddyDialog.IconButtons[5].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[6].x = 120;
	SkinProperties().EditBuddyDialog.IconButtons[6].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[7].x = 140;
	SkinProperties().EditBuddyDialog.IconButtons[7].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[8].x = 160;
	SkinProperties().EditBuddyDialog.IconButtons[8].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[9].x = 180;
	SkinProperties().EditBuddyDialog.IconButtons[9].y = 47;
	SkinProperties().EditBuddyDialog.IconButtons[10].x = 200;
	SkinProperties().EditBuddyDialog.IconButtons[10].y = 47;
	SkinProperties().EditBuddyDialog.DescriptionLabel.x = 0;
	SkinProperties().EditBuddyDialog.DescriptionLabel.y = 76;
	SkinProperties().EditBuddyDialog.DescriptionLabel.width = 0;
	SkinProperties().EditBuddyDialog.DescriptionLabel.height = 14;
	SkinProperties().EditBuddyDialog.Description.x = 0;
	SkinProperties().EditBuddyDialog.Description.y = 91;
	SkinProperties().EditBuddyDialog.Description.width = 0;
	SkinProperties().EditBuddyDialog.Description.height = 48;
	SkinProperties().EditBuddyDialog.Notify.x = 0;
	SkinProperties().EditBuddyDialog.Notify.y = 152;
	SkinProperties().EditBuddyDialog.Notify.width = 0;
	SkinProperties().EditBuddyDialog.Notify.height = 22;
	SkinProperties().Data104C = 250;
	SkinProperties().Data1050 = 29;
	SkinProperties().Data1054.x = 0;
	SkinProperties().Data1054.y = 0;
	SkinProperties().Data1054.width = 0;
	SkinProperties().Data1054.height = 14;
	SkinProperties().Data1074.x = 0;
	SkinProperties().Data1074.y = 18;
	SkinProperties().Data1074.width = 0;
	SkinProperties().Data1074.height = 11;
	SkinProperties().HelpDialog.Width = 250;
	SkinProperties().HelpDialog.Height = 185;
	SkinProperties().HelpDialog.ClientHelpButton.x = 0;
	SkinProperties().HelpDialog.ClientHelpButton.y = 0;
	SkinProperties().HelpDialog.HintsButton.x = 0;
	SkinProperties().HelpDialog.HintsButton.y = 33;
	SkinProperties().HelpDialog.TutorHelpButton.x = 0;
	SkinProperties().HelpDialog.TutorHelpButton.y = 66;
	SkinProperties().HelpDialog.RuleViolationButton.x = 0;
	SkinProperties().HelpDialog.RuleViolationButton.y = 99;
	SkinProperties().HelpDialog.ManualButton.x = 0;
	SkinProperties().HelpDialog.ManualButton.y = 132;
	SkinProperties().HelpDialog.FAQButton.x = 0;
	SkinProperties().HelpDialog.FAQButton.y = 165;
	SkinProperties().HelpDialog.ClientHelpLabel.x = 99;
	SkinProperties().HelpDialog.ClientHelpLabel.y = 0;
	SkinProperties().HelpDialog.HintsLabel.x = 99;
	SkinProperties().HelpDialog.HintsLabel.y = 33;
	SkinProperties().HelpDialog.TutorHelpLabel.x = 99;
	SkinProperties().HelpDialog.TutorHelpLabel.y = 66;
	SkinProperties().HelpDialog.RuleViolationLabel.x = 99;
	SkinProperties().HelpDialog.RuleViolationLabel.y = 99;
	SkinProperties().HelpDialog.ManualLabel.x = 99;
	SkinProperties().HelpDialog.ManualLabel.y = 132;
	SkinProperties().HelpDialog.FAQLabel.x = 99;
	SkinProperties().HelpDialog.FAQLabel.y = 165;
	SkinProperties().HintsDialogWidth = 200;
	SkinProperties().HintsDialogHeight = 119;
	SkinProperties().HintsDialogLabel.x = 0;
	SkinProperties().HintsDialogLabel.y = 0;
	SkinProperties().HintsDialogLabel.width = 0;
	SkinProperties().HintsDialogLabel.height = 14;
	SkinProperties().HintsDialogList.x = 0;
	SkinProperties().HintsDialogList.y = 15;
	SkinProperties().HintsDialogList.width = 0;
	SkinProperties().HintsDialogList.height = 104;
	SkinProperties().RuleViolationDialog.Width = 300;
	SkinProperties().RuleViolationDialog.Height = 346;
	SkinProperties().RuleViolationDialog.NameLabel.x = 0;
	SkinProperties().RuleViolationDialog.NameLabel.y = 0;
	SkinProperties().RuleViolationDialog.NameLabel.width = 0;
	SkinProperties().RuleViolationDialog.NameLabel.height = 14;
	SkinProperties().RuleViolationDialog.Name.x = 0;
	SkinProperties().RuleViolationDialog.Name.y = 15;
	SkinProperties().RuleViolationDialog.Name.width = 0;
	SkinProperties().RuleViolationDialog.Name.height = 16;
	SkinProperties().RuleViolationDialog.ReasonLabel.x = 0;
	SkinProperties().RuleViolationDialog.ReasonLabel.y = 92;
	SkinProperties().RuleViolationDialog.ReasonLabel.width = 0;
	SkinProperties().RuleViolationDialog.ReasonLabel.height = 14;
	SkinProperties().RuleViolationDialog.ReasonList.x = 0;
	SkinProperties().RuleViolationDialog.ReasonList.y = 107;
	SkinProperties().RuleViolationDialog.ReasonList.width = 0;
	SkinProperties().RuleViolationDialog.ReasonList.height = 77;
	SkinProperties().RuleViolationDialog.ActionLabel.x = 0;
	SkinProperties().RuleViolationDialog.ActionLabel.y = 199;
	SkinProperties().RuleViolationDialog.ActionLabel.width = 0;
	SkinProperties().RuleViolationDialog.ActionLabel.height = 14;
	SkinProperties().RuleViolationDialog.ActionList.x = 0;
	SkinProperties().RuleViolationDialog.ActionList.y = 214;
	SkinProperties().RuleViolationDialog.ActionList.width = 0;
	SkinProperties().RuleViolationDialog.ActionList.height = 49;
	SkinProperties().RuleViolationDialog.CommentLabel.x = 0;
	SkinProperties().RuleViolationDialog.CommentLabel.y = 315;
	SkinProperties().RuleViolationDialog.CommentLabel.width = 0;
	SkinProperties().RuleViolationDialog.CommentLabel.height = 14;
	SkinProperties().RuleViolationDialog.Comment.x = 0;
	SkinProperties().RuleViolationDialog.Comment.y = 330;
	SkinProperties().RuleViolationDialog.Comment.width = 0;
	SkinProperties().RuleViolationDialog.Comment.height = 16;
	SkinProperties().RuleViolationDialog.StatementLabel.x = 0;
	SkinProperties().RuleViolationDialog.StatementLabel.y = 46;
	SkinProperties().RuleViolationDialog.StatementLabel.width = 0;
	SkinProperties().RuleViolationDialog.StatementLabel.height = 14;
	SkinProperties().RuleViolationDialog.Statement.x = 0;
	SkinProperties().RuleViolationDialog.Statement.y = 61;
	SkinProperties().RuleViolationDialog.Statement.width = 0;
	SkinProperties().RuleViolationDialog.Statement.height = 16;
	SkinProperties().RuleViolationDialog.IPBanishment.x = 0;
	SkinProperties().RuleViolationDialog.IPBanishment.y = 278;
	SkinProperties().RuleViolationDialog.IPBanishment.width = 0;
	SkinProperties().RuleViolationDialog.IPBanishment.height = 22;
	SkinProperties().EditMarkDialogWidth = 250;
	SkinProperties().EditMarkDialogHeight = 97;
	SkinProperties().EditMarkDialogDescriptionLabel.x = 0;
	SkinProperties().EditMarkDialogDescriptionLabel.y = 0;
	SkinProperties().EditMarkDialogDescriptionLabel.width = 0;
	SkinProperties().EditMarkDialogDescriptionLabel.height = 14;
	SkinProperties().EditMarkDialogDescription.x = 0;
	SkinProperties().EditMarkDialogDescription.y = 15;
	SkinProperties().EditMarkDialogDescription.width = 0;
	SkinProperties().EditMarkDialogDescription.height = 16;
	SkinProperties().EditMarkDialogTypeLabel.x = 0;
	SkinProperties().EditMarkDialogTypeLabel.y = 46;
	SkinProperties().EditMarkDialogTypeLabel.width = 0;
	SkinProperties().EditMarkDialogTypeLabel.height = 14;
	SkinProperties().EditMarkDialogTypeButtons[0].x = 0;
	SkinProperties().EditMarkDialogTypeButtons[0].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[1].x = 20;
	SkinProperties().EditMarkDialogTypeButtons[1].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[2].x = 40;
	SkinProperties().EditMarkDialogTypeButtons[2].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[3].x = 60;
	SkinProperties().EditMarkDialogTypeButtons[3].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[4].x = 80;
	SkinProperties().EditMarkDialogTypeButtons[4].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[5].x = 100;
	SkinProperties().EditMarkDialogTypeButtons[5].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[6].x = 120;
	SkinProperties().EditMarkDialogTypeButtons[6].y = 61;
	SkinProperties().EditMarkDialogTypeButtons[7].x = 0;
	SkinProperties().EditMarkDialogTypeButtons[7].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[8].x = 20;
	SkinProperties().EditMarkDialogTypeButtons[8].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[9].x = 40;
	SkinProperties().EditMarkDialogTypeButtons[9].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[10].x = 60;
	SkinProperties().EditMarkDialogTypeButtons[10].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[11].x = 80;
	SkinProperties().EditMarkDialogTypeButtons[11].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[12].x = 100;
	SkinProperties().EditMarkDialogTypeButtons[12].y = 81;
	SkinProperties().EditMarkDialogTypeButtons[13].x = 120;
	SkinProperties().EditMarkDialogTypeButtons[13].y = 81;
	SkinProperties().OptionsDialog.Width = 250;
	SkinProperties().OptionsDialog.Height = 202;
	SkinProperties().OptionsDialog.GeneralButton.x = 0;
	SkinProperties().OptionsDialog.GeneralButton.y = 0;
	SkinProperties().OptionsDialog.GraphicsButton.x = 0;
	SkinProperties().OptionsDialog.GraphicsButton.y = 33;
	SkinProperties().OptionsDialog.ConsoleButton.x = 0;
	SkinProperties().OptionsDialog.ConsoleButton.y = 66;
	SkinProperties().OptionsDialog.NetworkButton.x = 0;
	SkinProperties().OptionsDialog.NetworkButton.y = 99;
	SkinProperties().OptionsDialog.HotkeysButton.x = 0;
	SkinProperties().OptionsDialog.HotkeysButton.y = 132;
	SkinProperties().OptionsDialog.MotdButton.x = 0;
	SkinProperties().OptionsDialog.MotdButton.y = 182;
	SkinProperties().OptionsDialog.GeneralLabel.x = 99;
	SkinProperties().OptionsDialog.GeneralLabel.y = 0;
	SkinProperties().OptionsDialog.GraphicsLabel.x = 99;
	SkinProperties().OptionsDialog.GraphicsLabel.y = 33;
	SkinProperties().OptionsDialog.ConsoleLabel.x = 99;
	SkinProperties().OptionsDialog.ConsoleLabel.y = 66;
	SkinProperties().OptionsDialog.NetworkLabel.x = 99;
	SkinProperties().OptionsDialog.NetworkLabel.y = 99;
	SkinProperties().OptionsDialog.HotkeysLabel.x = 99;
	SkinProperties().OptionsDialog.HotkeysLabel.y = 132;
	SkinProperties().OptionsDialog.MotdLabel.x = 99;
	SkinProperties().OptionsDialog.MotdLabel.y = 182;
	SkinProperties().OptionsDialog.Separator.x = 0;
	SkinProperties().OptionsDialog.Separator.y = 165;
	SkinProperties().OptionsDialog.Separator.width = 0;
	SkinProperties().OptionsDialog.Separator.height = 2;
	SkinProperties().GeneralOptionsDialogWidth = 200;
	SkinProperties().GeneralOptionsDialogHeight = 154;
	SkinProperties().GeneralOptionsDialogClassicControl.x = 0;
	SkinProperties().GeneralOptionsDialogClassicControl.y = 0;
	SkinProperties().GeneralOptionsDialogClassicControl.width = 0;
	SkinProperties().GeneralOptionsDialogClassicControl.height = 22;
	SkinProperties().GeneralOptionsDialogAutoChaseOff.x = 0;
	SkinProperties().GeneralOptionsDialogAutoChaseOff.y = 33;
	SkinProperties().GeneralOptionsDialogAutoChaseOff.width = 0;
	SkinProperties().GeneralOptionsDialogAutoChaseOff.height = 22;
	SkinProperties().GeneralOptionsDialogShowHints.x = 0;
	SkinProperties().GeneralOptionsDialogShowHints.y = 66;
	SkinProperties().GeneralOptionsDialogShowHints.width = 0;
	SkinProperties().GeneralOptionsDialogShowHints.height = 22;
	SkinProperties().GeneralOptionsDialogTextualEffects.x = 0;
	SkinProperties().GeneralOptionsDialogTextualEffects.y = 132;
	SkinProperties().GeneralOptionsDialogTextualEffects.width = 0;
	SkinProperties().GeneralOptionsDialogTextualEffects.height = 22;
	SkinProperties().GeneralOptionsDialogCreatureNames.x = 0;
	SkinProperties().GeneralOptionsDialogCreatureNames.y = 99;
	SkinProperties().GeneralOptionsDialogCreatureNames.width = 0;
	SkinProperties().GeneralOptionsDialogCreatureNames.height = 22;
	SkinProperties().GraphicsOptionsDialogWidth = 200;
	SkinProperties().GraphicsOptionsDialogHeight = 185;
	SkinProperties().GraphicsOptionsDialogFullscreen.x = 0;
	SkinProperties().GraphicsOptionsDialogFullscreen.y = 0;
	SkinProperties().GraphicsOptionsDialogFullscreen.width = 0;
	SkinProperties().GraphicsOptionsDialogFullscreen.height = 22;
	SkinProperties().GraphicsOptionsDialogResolutionList.x = 0;
	SkinProperties().GraphicsOptionsDialogResolutionList.y = 50;
	SkinProperties().GraphicsOptionsDialogResolutionList.width = 0;
	SkinProperties().GraphicsOptionsDialogResolutionList.height = 104;
	SkinProperties().GraphicsOptionsDialogResolutionLabel.x = 0;
	SkinProperties().GraphicsOptionsDialogResolutionLabel.y = 35;
	SkinProperties().GraphicsOptionsDialogResolutionLabel.width = 0;
	SkinProperties().GraphicsOptionsDialogResolutionLabel.height = 14;
	SkinProperties().GraphicsOptionsDialogAdvancedButton.x = -86;
	SkinProperties().GraphicsOptionsDialogAdvancedButton.y = 167;
	SkinProperties().GraphicsOptionsDialogAdvancedLabel.x = 0;
	SkinProperties().GraphicsOptionsDialogAdvancedLabel.y = 171;
	SkinProperties().GraphicsOptionsDialogAdvancedLabel.width = 0;
	SkinProperties().GraphicsOptionsDialogAdvancedLabel.height = 14;
	SkinProperties().AdvancedGraphicsDialog.Width = 200;
	SkinProperties().AdvancedGraphicsDialog.Height = 213;
	SkinProperties().AdvancedGraphicsDialog.DeviceLabel.x = 0;
	SkinProperties().AdvancedGraphicsDialog.DeviceLabel.y = 0;
	SkinProperties().AdvancedGraphicsDialog.DirectX5Button.x = 0;
	SkinProperties().AdvancedGraphicsDialog.DirectX5Button.y = 15;
	SkinProperties().AdvancedGraphicsDialog.DirectX9Button.x = 70;
	SkinProperties().AdvancedGraphicsDialog.DirectX9Button.y = 15;
	SkinProperties().AdvancedGraphicsDialog.OpenGLButton.x = 140;
	SkinProperties().AdvancedGraphicsDialog.OpenGLButton.y = 15;
	SkinProperties().AdvancedGraphicsDialog.LightEffects.x = 0;
	SkinProperties().AdvancedGraphicsDialog.LightEffects.y = 51;
	SkinProperties().AdvancedGraphicsDialog.LightEffects.width = 0;
	SkinProperties().AdvancedGraphicsDialog.LightEffects.height = 22;
	SkinProperties().AdvancedGraphicsDialog.NoStretching.x = 0;
	SkinProperties().AdvancedGraphicsDialog.NoStretching.y = 86;
	SkinProperties().AdvancedGraphicsDialog.NoStretching.width = 0;
	SkinProperties().AdvancedGraphicsDialog.NoStretching.height = 22;
	SkinProperties().AdvancedGraphicsDialog.WindowsMousepointer.x = 0;
	SkinProperties().AdvancedGraphicsDialog.WindowsMousepointer.y = 121;
	SkinProperties().AdvancedGraphicsDialog.WindowsMousepointer.width = 0;
	SkinProperties().AdvancedGraphicsDialog.WindowsMousepointer.height = 22;
	SkinProperties().AdvancedGraphicsDialog.Framerate.x = 0;
	SkinProperties().AdvancedGraphicsDialog.Framerate.y = 181;
	SkinProperties().AdvancedGraphicsDialog.Framerate.width = 0;
	SkinProperties().AdvancedGraphicsDialog.Framerate.height = 12;
	SkinProperties().AdvancedGraphicsDialog.FramerateLimitText.x = 0;
	SkinProperties().AdvancedGraphicsDialog.FramerateLimitText.y = 166;
	SkinProperties().AdvancedGraphicsDialog.CurrentFramerateText.x = 0;
	SkinProperties().AdvancedGraphicsDialog.CurrentFramerateText.y = 199;
	SkinProperties().ConsoleOptionsDialogWidth = 260;
	SkinProperties().ConsoleOptionsDialogHeight = 162;
	SkinProperties().ConsoleOptionsDialogInfoMessages.x = 0;
	SkinProperties().ConsoleOptionsDialogInfoMessages.y = 0;
	SkinProperties().ConsoleOptionsDialogInfoMessages.width = 0;
	SkinProperties().ConsoleOptionsDialogInfoMessages.height = 22;
	SkinProperties().ConsoleOptionsDialogEventMessages.x = 0;
	SkinProperties().ConsoleOptionsDialogEventMessages.y = 35;
	SkinProperties().ConsoleOptionsDialogEventMessages.width = 0;
	SkinProperties().ConsoleOptionsDialogEventMessages.height = 22;
	SkinProperties().ConsoleOptionsDialogStatusMessages.x = 0;
	SkinProperties().ConsoleOptionsDialogStatusMessages.y = 70;
	SkinProperties().ConsoleOptionsDialogStatusMessages.width = 0;
	SkinProperties().ConsoleOptionsDialogStatusMessages.height = 22;
	SkinProperties().ConsoleOptionsDialogTimeStamps.x = 0;
	SkinProperties().ConsoleOptionsDialogTimeStamps.y = 105;
	SkinProperties().ConsoleOptionsDialogTimeStamps.width = 0;
	SkinProperties().ConsoleOptionsDialogTimeStamps.height = 22;
	SkinProperties().ConsoleOptionsDialogPrivateMessages.x = 0;
	SkinProperties().ConsoleOptionsDialogPrivateMessages.y = 140;
	SkinProperties().ConsoleOptionsDialogPrivateMessages.width = 0;
	SkinProperties().ConsoleOptionsDialogPrivateMessages.height = 22;
	SkinProperties().NetworkOptionsDialogWidth = 200;
	SkinProperties().NetworkOptionsDialogHeight = 22;
	SkinProperties().NetworkOptionsDialogTestServerLogin.x = 0;
	SkinProperties().NetworkOptionsDialogTestServerLogin.y = 0;
	SkinProperties().NetworkOptionsDialogTestServerLogin.width = 0;
	SkinProperties().NetworkOptionsDialogTestServerLogin.height = 22;
	SkinProperties().HotkeyDialogWidth = 200;
	SkinProperties().HotkeyDialogHeight = 198;
	SkinProperties().HotkeyDialogList.x = 0;
	SkinProperties().HotkeyDialogList.y = 15;
	SkinProperties().HotkeyDialogList.width = 0;
	SkinProperties().HotkeyDialogList.height = 104;
	SkinProperties().HotkeyDialogListLabel.x = 0;
	SkinProperties().HotkeyDialogListLabel.y = 0;
	SkinProperties().HotkeyDialogListLabel.width = 0;
	SkinProperties().HotkeyDialogListLabel.height = 14;
	SkinProperties().HotkeyDialogText.x = 0;
	SkinProperties().HotkeyDialogText.y = 147;
	SkinProperties().HotkeyDialogText.width = 0;
	SkinProperties().HotkeyDialogText.height = 16;
	SkinProperties().HotkeyDialogTextLabel.x = 0;
	SkinProperties().HotkeyDialogTextLabel.y = 132;
	SkinProperties().HotkeyDialogTextLabel.width = 0;
	SkinProperties().HotkeyDialogTextLabel.height = 14;
	SkinProperties().HotkeyDialogAutoSend.x = 0;
	SkinProperties().HotkeyDialogAutoSend.y = 176;
	SkinProperties().HotkeyDialogAutoSend.width = 0;
	SkinProperties().HotkeyDialogAutoSend.height = 22;
	SkinProperties().HotkeyDialogAutoSendColor.R = 255;
	SkinProperties().HotkeyDialogAutoSendColor.G = 255;
	SkinProperties().HotkeyDialogAutoSendColor.B = 255;
	SkinProperties().Data14E0 = 475;
	SkinProperties().Data14E4 = 284;
	SkinProperties().Data14E8.x = 0;
	SkinProperties().Data14E8.y = 0;
	SkinProperties().Data14E8.width = 0;
	SkinProperties().Data14E8.height = 56;
	SkinProperties().Data14F8.x = 0;
	SkinProperties().Data14F8.y = 57;
	SkinProperties().Data14F8.width = 0;
	SkinProperties().Data14F8.height = 174;
	SkinProperties().Data1508.x = 0;
	SkinProperties().Data1508.y = 253;
	SkinProperties().Data1508.width = 0;
	SkinProperties().Data1508.height = 14;
	SkinProperties().Data1518.x = 0;
	SkinProperties().Data1518.y = 268;
	SkinProperties().Data1518.width = 0;
	SkinProperties().Data1518.height = 16;
}

// --- Timers ---------------------------------------------------------------

// Takes the first free entry.
void patella::TGUI::AddTimer(TTimer *Timer)
{
	const auto Free = std::ranges::find(Timers, nullptr);
	if (Free == Timers.end())
		Timers.push_back(Timer);
	else
		*Free = Timer;
}

void patella::TGUI::RemoveTimer(TTimer *Timer)
{
	const auto Entry = std::ranges::find(Timers, Timer);
	if (Entry != Timers.end())
		*Entry = nullptr;
}

// Called by ProcessEvents. The first call only starts the clock.
void patella::TGUI::UpdateTimers()
{
	if (!LastTimeSet) { // the original's function-local static, initialised on the first call (issue #107)
		LastTime = GetMilliseconds();
		LastTimeSet = true;
	}
	const int elapsed = GetMilliseconds() - LastTime;
	LastTime = GetMilliseconds();
	// By index, the size read each time: a timer's callback may add or remove timers, as in the original.
	for (std::size_t i = 0; i < Timers.size(); i++) {
		if (Timers[i] != nullptr && !Timers[i]->IsExpired())
			Timers[i]->Update(elapsed);
	}
}

// A time of 0 leaves the timer inactive.
TTimer::TTimer(int Time, TCallback Callback)
{
	this->Callback = Callback;
	Remaining = Time * 100;
	if (Remaining == 0) {
		Active = false;
	} else {
		AddTimer(this);
		Active = true;
	}
}

// The deleting destructor; this destructor is inlined into it.
TTimer::~TTimer()
{
	RemoveTimer(this);
	Active = false;
}

// The callback is sent on every update, not only when the time is up.
void TTimer::Update(int elapsed)
{
	if (Active) {
		if (Remaining > 0) {
			Remaining -= elapsed;
			if (Remaining < 0)
				Remaining = 0;
		}
		Callback.Send();
		if (Remaining == 0) {
			RemoveTimer(this);
			Active = false;
		}
	}
}

// --- TComponent -------------------------------------------------------------

void TComponent::InsertChild([[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

void TComponent::AppendChild([[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

void TComponent::RemoveChild([[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

void TComponent::DeleteChild([[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

void TComponent::InsertChildAfter([[maybe_unused]] TComponent *Previous, [[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

void TComponent::SwapChildren([[maybe_unused]] TComponent *Child1, [[maybe_unused]] TComponent *Child2)
{
	TERROR("not possible");
}

bool TComponent::IsChild([[maybe_unused]] TComponent *Child)
{
	TERROR("not possible");
}

TComponent *TComponent::GetPrevious()
{
	PATELLA_ASSERT(Parent!=nullptr);
	TComponent *Search = Parent->GetFirstChild();
	while (Search != nullptr) {
		if (Search->GetNext() == this)
			return Search;
		Search = Search->GetNext();
	}
	return nullptr;
}

void TComponent::SetSize(int _Width, int _Height)
{
	PATELLA_ASSERT_VALUES(_Width>=0, _Width);
	PATELLA_ASSERT_VALUES(_Height>=0, _Height);
	Height = _Height;
	Width = _Width;
}

// Draws the drawing list at (x, y); entries without a size draw the
// whole skin once.
void TComponent::Draw(int Surface, int x, int y)
{
	int i = 0;
	for (; i < DrawingListLength; i++) {
		if (DrawingList[i].skin == SKIN_NONE)
			continue;
		PATELLA_ASSERT_VALUES(DrawingList[i].skin<patella::bridge::Skins().NumberOfSkins, DrawingList[i].skin);
		if (DrawingList[i].width == 0 && DrawingList[i].height == 0) {
			const TSkin *Skin = &Skins()[DrawingList[i].skin];
			DrawPicturePart(Surface, DrawingList[i].x + x, DrawingList[i].y + y, 4, Skin->x, Skin->y, Skin->width,
				Skin->height);
			continue;
		}
		if (DrawingList[i].x + x < 0 || DrawingList[i].y + y < 0) {
			std::stringstream s;
			const char *Name = " (CONSOLETEXT_CORNER_TOPRIGHT)";
			if (DrawingList[i].skin != SKIN_CONSOLETEXT_CORNER_TOPRIGHT)
				Name = "";
			s << "dialog origin outside main window " << "(surface: " << Surface << " x: " << x << " y: " << y
			  << " DrawingListLength: " << DrawingListLength << " i: " << i << " DrawingList.Skin: "
			  << DrawingList[i].skin << Name << " DrawingList.x: " << DrawingList[i].x
			  << " DrawingList.y: " << DrawingList[i].y << " DrawingList.width: " << DrawingList[i].width
			  << " DrawingList.height: " << DrawingList[i].height << " DrawingList.srcx: "
			  << DrawingList[i].srcx << " DrawingList.srcy: " << DrawingList[i].srcy << ")";
			TERROR(s.str().c_str());
		}
		DrawSkin(Surface, DrawingList[i].x + x, DrawingList[i].y + y, DrawingList[i].width,
			DrawingList[i].height, DrawingList[i].skin, DrawingList[i].srcx, DrawingList[i].srcy);
	}
}

// --- TWindow ----------------------------------------------------------------

// Inserts the child at the front.
void TWindow::InsertChild(TComponent *Child)
{
	PATELLA_ASSERT(Child!=nullptr);
	PATELLA_ASSERT(Child->Parent==nullptr);
	Child->Next = FirstChild;
	FirstChild = Child;
	Child->Parent = this;
}

// The first child's Next is not reset.
void TWindow::AppendChild(TComponent *Child)
{
	PATELLA_ASSERT(Child!=nullptr);
	PATELLA_ASSERT(Child->Parent==nullptr);
	if (FirstChild == nullptr) {
		FirstChild = Child;
	} else {
		TComponent *Search = FirstChild;
		while (Search->Next != nullptr)
			Search = Search->Next;
		Search->Next = Child;
		Child->Next = nullptr;
	}
	Child->Parent = this;
}

// The child is closed after it is removed, where the original's delete
// ran its destructor (issue #108).
void TWindow::DeleteChild(TComponent *Child)
{
	RemoveChild(Child);
	DeleteComponent(Child);
}

// A child holding the capture stops dragging and releases it through
// itself (with NULL), which reaches this window's ReleaseCapture by its
// parent.
void TWindow::RemoveChild(TComponent *Child)
{
	PATELLA_ASSERT(Child!=nullptr);
	PATELLA_ASSERT(Child->Parent==this);
	if (Child == Capture) {
		Child->StopDragging();
		Child->ReleaseCapture(nullptr);
	}
	if (Child == FirstChild) {
		FirstChild = Child->Next;
	} else {
		TComponent *Search = FirstChild;
		for (;;) {
			PATELLA_ASSERT(Search!=nullptr);
			if (Search->Next == Child)
				break;
			Search = Search->Next;
		}
		Search->Next = Child->Next;
	}
	Child->Parent = nullptr;
	Child->Next = nullptr;
}

// Inserts the child after Previous, at the front when Previous is NULL.
void TWindow::InsertChildAfter(TComponent *Previous, TComponent *Child)
{
	PATELLA_ASSERT(Child!=nullptr);
	PATELLA_ASSERT(Child->Parent==nullptr);
	if (Previous == nullptr) {
		InsertChild(Child);
		return;
	}
	TComponent *Search = FirstChild;
	PATELLA_ASSERT(Search!=nullptr);
	while (Search != nullptr && Search != Previous)
		Search = Search->Next;
	PATELLA_ASSERT(Search!=nullptr);
	Child->Parent = this;
	Child->Next = Search->Next;
	Search->Next = Child;
}

// Odd behaviour kept: two children that are not neighbours are removed
// and inserted again after their own predecessors, so they keep their
// places.
void TWindow::SwapChildren(TComponent *Child1, TComponent *Child2)
{
	PATELLA_ASSERT(Child1!=nullptr);
	PATELLA_ASSERT(Child2!=nullptr);
	PATELLA_ASSERT(Child1->Parent==this);
	PATELLA_ASSERT(Child2->Parent==this);
	PATELLA_ASSERT(FirstChild!=nullptr);
	TComponent *Before1 = nullptr;
	TComponent *Search1 = FirstChild;
	while (Search1 != nullptr && Search1 != Child1) {
		Before1 = Search1;
		Search1 = Search1->Next;
	}
	PATELLA_ASSERT(Search1!=nullptr);
	TComponent *Before2 = nullptr;
	TComponent *Search2 = FirstChild;
	while (Search2 != nullptr && Search2 != Child2) {
		Before2 = Search2;
		Search2 = Search2->Next;
	}
	PATELLA_ASSERT(Search2!=nullptr);
	PATELLA_ASSERT(Before1!=Before2);
	TComponent *After2 = Search2->Next;
	if (After2 == Search1) {
		TComponent *After1 = Search1->Next;
		if (FirstChild == Search2)
			FirstChild = Search1;
		else
			Before2->Next = Search1;
		Search1->Next = Search2;
		Search2->Next = After1;
	} else if (Search1->Next == Search2) {
		if (FirstChild == Search1)
			FirstChild = Search2;
		else
			Before1->Next = Search2;
		Search2->Next = Search1;
		Search1->Next = After2;
	} else {
		RemoveChild(Search1);
		RemoveChild(Search2);
		InsertChildAfter(Before1, Search1);
		InsertChildAfter(Before2, Search2);
	}
}

void TWindow::SetPosition(int _OffsetX, int _OffsetY)
{
	OffsetX = _OffsetX;
	OffsetY = _OffsetY;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next)
		Child->PositionChanged();
}

// Deletes the children. After the window's Close (issue #108) its children are
// closed and DeleteChild only removes and frees them. A window destroyed
// without a close is one whose constructor threw (its fully made bases are
// destroyed): then DeleteChild closes each child, as the original's destructors
// did their work there, and an exception of it ends the program, as the
// original's second exception during the unwinding did.
TWindow::~TWindow()
{
	while (FirstChild != nullptr)
		DeleteChild(FirstChild);
}

// Issue #108: ~TWindow's order, the first child first. The next child is taken
// before the close (a close does not change the list; the original's
// destructors ran on children already removed from it).
void TWindow::OnClose()
{
	TComponent *Child = FirstChild;
	while (Child != nullptr) {
		TComponent *Next = Child->Next;
		Child->Close();
		Child = Next;
	}
}

// The window's own drawing list, then the children.
void TWindow::Draw(int Surface, int x, int y)
{
	TComponent::Draw(Surface, x, y);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next)
		Child->Draw(Surface, Child->OffsetX + x, Child->OffsetY + y);
}

void TWindow::Arrange()
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next)
		Child->Arrange();
}

// The mouse functions go to the capture if there is one, else to the
// first child under the point that takes the event, in child
// coordinates.
bool TWindow::MouseMove(int x, int y)
{
	if (Capture != nullptr)
		return Capture->MouseMove(x - Capture->OffsetX, y - Capture->OffsetY);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->MouseMove(x - Child->OffsetX, y - Child->OffsetY))
			return true;
	}
	return false;
}

bool TWindow::LeftButtonDown(int x, int y, int Flags)
{
	if (Capture != nullptr)
		return Capture->LeftButtonDown(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->LeftButtonDown(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
}

bool TWindow::LeftButtonUp(int x, int y, int Flags)
{
	if (Capture != nullptr)
		return Capture->LeftButtonUp(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->LeftButtonUp(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
}

bool TWindow::LeftButtonDoubleClick(int x, int y, int Flags)
{
	if (Capture != nullptr)
		return Capture->LeftButtonDoubleClick(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->LeftButtonDoubleClick(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
}

bool TWindow::RightButtonUp(int x, int y, int Flags)
{
	if (Capture != nullptr)
		return Capture->RightButtonUp(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->RightButtonUp(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
}

bool TWindow::MouseWheel(int x, int y, int Direction)
{
	if (Capture != nullptr)
		return Capture->MouseWheel(x - Capture->OffsetX, y - Capture->OffsetY, Direction);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->MouseWheel(x - Child->OffsetX, y - Child->OffsetY, Direction))
			return true;
	}
	return false;
}

// No capture check; only the first child under the point gets it.
void TWindow::MouseHover(int x, int y)
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)) {
			Child->MouseHover(x - Child->OffsetX, y - Child->OffsetY);
			return;
		}
	}
}

// No capture check. The child that explains itself moves the help area by
// its offset; without one the help handle is cleared.
bool TWindow::HelpMouseMove(int x, int y)
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
			&& Child->HelpMouseMove(x - Child->OffsetX, y - Child->OffsetY)) {
			MoveHelpPosition(Child->OffsetX, Child->OffsetY);
			return true;
		}
	}
	SetHelpHandle(HELP_NONE);
	return false;
}

// The parents up to the main window capture this window in turn.
void TWindow::SetCapture(TComponent *Component)
{
	PATELLA_ASSERT(Capture==nullptr);
	Capture = Component;
	if (Parent != nullptr)
		Parent->SetCapture(this);
}

void TWindow::ReleaseCapture(TComponent *Component)
{
	PATELLA_ASSERT(Capture==Component);
	Capture = nullptr;
	if (Parent != nullptr)
		Parent->ReleaseCapture(this);
}

// Does not release the parents' capture.
void TWindow::StopDragging()
{
	if (Capture != nullptr) {
		Capture->StopDragging();
		Capture = nullptr;
	}
}

// Negative coordinates count from the right and bottom edges, sizes below
// 1 are relative to the space left. They name the window's Width and
// Height, not the area (whose name is ours, as FitToChild's).
void TWindow::SetChildArea(TComponent *Child, TRect Area)
{
	PATELLA_ASSERT(Child->Parent==this);
	PATELLA_ASSERT_VALUES(Width>0, Width);
	PATELLA_ASSERT_VALUES(Height>0, Height);
	if (Area.x < 0)
		Area.x = Width + Area.x;
	if (Area.y < 0)
		Area.y = Height + Area.y;
	Child->SetPosition(Area.x, Area.y);
	if (Area.width < 1)
		Area.width = (Width - Area.x) + Area.width;
	if (Area.height < 1)
		Area.height = (Height - Area.y) + Area.height;
	Child->SetSize(Area.width, Area.height);
}

void TWindow::SetChildPosition(TComponent *Child, int x, int y)
{
	PATELLA_ASSERT(Child->Parent==this);
	PATELLA_ASSERT_VALUES(Width>0, Width);
	PATELLA_ASSERT_VALUES(Height>0, Height);
	if (x < 0)
		x = Width + x;
	if (y < 0)
		y = Height + y;
	Child->SetPosition(x, y);
}

// Sizes the window to the child placed at (Area.x, Area.y), with a negative
// Area.width and Area.height as margins after it, and not below the minimum
// size. Notify chooses the virtual SetSize; otherwise only the size fields
// change.
void TWindow::FitToChild(TComponent *Child, TRect Area, bool Notify)
{
	PATELLA_ASSERT(Child!=nullptr);
	PATELLA_ASSERT_VALUES(Child->getWidth()>0, Child->getWidth());
	PATELLA_ASSERT_VALUES(Child->getHeight()>0, Child->getHeight());
	int NewWidth = Child->Width + (Area.x > 0 ? Area.x : 0) + (Area.width < 0 ? -Area.width : 0);
	int NewHeight = (Area.y > 0 ? Area.y : 0) + Child->Height + (Area.height < 0 ? -Area.height : 0);
	const int MinimumWidth = getMinimumWidth();
	if (NewWidth < MinimumWidth)
		NewWidth = MinimumWidth;
	const int MinimumHeight = getMinimumHeight();
	if (NewHeight < MinimumHeight)
		NewHeight = MinimumHeight;
	if (Notify)
		SetSize(NewWidth, NewHeight);
	else
		TComponent::SetSize(NewWidth, NewHeight);
}

// Moves the children so that their bounding box starts at (|left margin|,
// |top margin|) and sizes the window to it plus the right and bottom
// margins (also taken as absolute values), not below the minimum size.
// Nothing happens without children.
void TWindow::FitToChildren(TRect Margin, bool Notify)
{
	if (FirstChild == nullptr)
		return;
	int LeastX = INT_MAX;
	int LeastY = INT_MAX;
	int Width = 0;
	int Height = 0;
	TComponent *Child;
	for (Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if (Child->OffsetX <= LeastX)
			LeastX = Child->OffsetX;
		if (Child->OffsetY <= LeastY)
			LeastY = Child->OffsetY;
		if (Width <= Child->OffsetX + Child->Width)
			Width = Child->OffsetX + Child->Width;
		if (Height <= Child->Height + Child->OffsetY)
			Height = Child->Height + Child->OffsetY;
	}
	PATELLA_ASSERT_VALUES(Width>0, Width);
	PATELLA_ASSERT_VALUES(Height>0, Height);
	PATELLA_ASSERT_VALUES(LeastX<INT_MAX, LeastX);
	PATELLA_ASSERT_VALUES(LeastY<INT_MAX, LeastY);
	const int dx = abs(Margin.x) - LeastX;
	const int dy = abs(Margin.y) - LeastY;
	Width = Width + abs(Margin.width) + abs(Margin.x);
	Height = Height + abs(Margin.height) + abs(Margin.y);
	for (Child = FirstChild; Child != nullptr; Child = Child->GetNext())
		Child->SetPosition(Child->OffsetX + dx, Child->OffsetY + dy);
	const int MinimumWidth = getMinimumWidth();
	if (Width < MinimumWidth)
		Width = MinimumWidth;
	const int MinimumHeight = getMinimumHeight();
	if (Height < MinimumHeight)
		Height = MinimumHeight;
	if (Notify)
		SetSize(Width, Height);
	else
		TComponent::SetSize(Width, Height);
}

// Cuts the children to the window. A child that sticks out is deleted when
// Strict is set (or when nothing of it is left), else moved and resized to
// the visible part. The checks against the right and bottom edges use the
// old position.
void TWindow::ClipChildren(bool Strict)
{
	if (FirstChild != nullptr) {
		TComponent *Next = FirstChild;
		while (Next != nullptr) {
			TComponent *Child = Next;
			Next = Child->GetNext();
			int x = Child->OffsetX;
			int NewHeight = Child->Height;
			const int y = Child->OffsetY;
			const int OldWidth = Child->Width;
			bool Changed = x < 0;
			int NewWidth = OldWidth;
			if (Changed) {
				NewWidth = Child->OffsetX + OldWidth;
				x = 0;
			}
			bool Remove = Changed && Strict;
			int NewY = y;
			if (y < 0) {
				Remove = Strict || Remove;
				NewHeight = Child->Height + y;
				NewY = 0;
			}
			Changed = y < 0 || Changed;
			if (Width < Child->OffsetX + OldWidth) {
				if (Strict)
					Remove = true;
				NewWidth = Width - Child->OffsetX;
				Changed = true;
			}
			if (Height < Child->Height + y) {
				if (Strict)
					Remove = true;
				NewHeight = Height - y;
				Changed = true;
			}
			if (NewWidth < 0 || NewHeight < 0 || Remove) {
				DeleteChild(Child);
			} else if (Changed) {
				Child->SetPosition(x, NewY);
				Child->SetSize(NewWidth, NewHeight);
			}
		}
	}
}

// --- TElement -------------------------------------------------------------

// Captures this element in the parent; the argument is not used.
void TElement::SetCapture([[maybe_unused]] TComponent *Component)
{
	if (GetParent() != nullptr)
		GetParent()->SetCapture(this);
}

void TElement::ReleaseCapture([[maybe_unused]] TComponent *Component)
{
	if (GetParent() != nullptr)
		GetParent()->ReleaseCapture(this);
}

// --- TCallback --------------------------------------------------------------

void TCallback::Send() const
{
	if (Receiver != nullptr)
		Receiver->HandleEvent(Event);
}

// --- Hotkeys --------------------------------------------------------------

// Replaces the callback of a key that is already there.
void patella::TGUI::AddHotkey(int Key, TCallback Callback)
{
	Hotkeys.insert_or_assign(Key, Callback);
}

// The last entry takes the place of the removed one.
void patella::TGUI::RemoveHotkey(int Key)
{
	Hotkeys.erase(Key);
}

// Called by Control.cpp's KeyPress when no modal window is open. Flag 0x200
// is ignored and lower-case letters count as upper case.
bool patella::TGUI::CheckHotkey(int Key)
{
	if ((Key & KEY_SHIFT) != 0)
		Key -= KEY_SHIFT;
	if ((Key & KEY_CODE_MASK) > 0x60 && (Key & KEY_CODE_MASK) < 0x7b)
		Key -= 'a' - 'A';
	const auto Hotkey = Hotkeys.find(Key);
	if (Hotkey == Hotkeys.end())
		return false;
	// A copy: the receiver may add or remove hotkeys (the original's Send read the entry before it called).
	const TCallback Callback = Hotkey->second;
	Callback.Send();
	return true;
}

// The functions of the original (issue #107): each calls its member on the one TClient's TGUI.

void AddTimer(TTimer *Timer)
{
	patella::bridge::GUI().AddTimer(Timer);
}

void RemoveTimer(TTimer *Timer)
{
	patella::bridge::GUI().RemoveTimer(Timer);
}

void UpdateTimers()
{
	patella::bridge::GUI().UpdateTimers();
}

void InitHotkeys()
{
	patella::bridge::GUI().InitHotkeys();
}

void ExitHotkeys()
{
	patella::TGUI::ExitHotkeys();
}

void AddHotkey(int Key, TCallback Callback)
{
	patella::bridge::GUI().AddHotkey(Key, Callback);
}

void RemoveHotkey(int Key)
{
	patella::bridge::GUI().RemoveHotkey(Key);
}

bool CheckHotkey(int Key)
{
	return patella::bridge::GUI().CheckHotkey(Key);
}
// NOLINTEND(cppcoreguidelines-owning-memory)

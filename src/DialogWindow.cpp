// DialogWindow.cpp: reconstructed from Tibia.exe 7.72.
//
// The columns of content windows (TDialogWindow), their base class
// (TContentWindow) and the windows with a body and a frame (TSubWindow); see
// DialogWindow.h. The object opens with six functions without exception
// frames or line evidence; they stay at the start, where the binary has them.
// GetFrameHeight and TContentWindow's constructor are called by
// ContentWindows.obj, which comes earlier in the link order, so they are not
// inline header code (a header copy would have been kept from that object).
// The other functions follow in line order. The binary moves four of them to
// the end of the object (Arrange, Shrink, Grow, Rearrange): each calls a
// function defined further down.
//
// A column lays its children out from the top of Area, each as wide as the
// column. Rearrange is called after every change with a strategy: when a
// child is dragged it swaps places with a neighbour it overlaps by half; when
// a child grows the others shrink to their minimum heights from the bottom
// up; when one shrinks the windows below it take the space it gave up; when
// a new one comes the ones at the bottom shrink or close to make room; when
// the column gets smaller the windows that do not fit any more shrink or
// close. Arrange then fills the space below the last child with nine skins.

#include "DialogWindow.h"
#include "DialogElements.h"
#include "Preferences.h"
#include "Skins.h"
#include "Surface.h"
#include "GUI.h"
#include "common.h"

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

// A column is as wide as the side bar.
int TDialogWindow::getMinimumWidth()
{
	return SkinProperties().SideBarWidth;
}

// Also inlined into TSubWindow's constructor.
TContentWindow::TContentWindow(int _Identifier, bool _Background)
{
	Identifier = _Identifier;
	Dragging = false;
	DragX = 0;
	DragY = 0;
	DragBottom = 0;
	BackgroundY = 0;
	Background = _Background;
}

// The parts that are not children at the moment (a hidden "up" button, the
// parts of a minimized window) are deleted here (the close closed them);
// ~TWindow deletes the children.
TSubWindow::~TSubWindow()
{
	CheckClosed("TSubWindow");
	if (UpButton != nullptr && !IsChild(UpButton))
		delete UpButton;
	if (CloseButton != nullptr && !IsChild(CloseButton))
		delete CloseButton;
	if (MinimizeButton != nullptr && !IsChild(MinimizeButton))
		delete MinimizeButton;
	if (Icon != nullptr && !IsChild(Icon))
		delete Icon;
	if (Title != nullptr && !IsChild(Title))
		delete Title;
	if (Body != nullptr && !IsChild(Body))
		delete Body;
	if (Resizer1 != nullptr && !IsChild(Resizer1))
		delete Resizer1;
	if (Resizer2 != nullptr && !IsChild(Resizer2))
		delete Resizer2;
}

// The destructor's work until issue #108, in its order: the skill, battle and
// VIP windows store their body's height in the preferences ("ContentWindow"),
// the parts that are not children are closed where it deleted them (a
// minimized container's body sends its close); then the children.
void TSubWindow::OnClose()
{
	try {
		if (Identifier == SKILLS_WINDOW || Identifier == BATTLE_WINDOW || Identifier == BUDDYLIST_WINDOW)
			SetContentWindowSize(Identifier, Body->Height);
		if (UpButton != nullptr && !IsChild(UpButton))
			UpButton->Close();
		if (CloseButton != nullptr && !IsChild(CloseButton))
			CloseButton->Close();
		if (MinimizeButton != nullptr && !IsChild(MinimizeButton))
			MinimizeButton->Close();
		if (Icon != nullptr && !IsChild(Icon))
			Icon->Close();
		if (Title != nullptr && !IsChild(Title))
			Title->Close();
		if (Body != nullptr && !IsChild(Body))
			Body->Close();
		if (Resizer1 != nullptr && !IsChild(Resizer1))
			Resizer1->Close();
		if (Resizer2 != nullptr && !IsChild(Resizer2))
			Resizer2->Close();
	} catch (...) {
		CloseWhileUnwinding([this] { TContentWindow::OnClose(); });
		throw;
	}
	TContentWindow::OnClose();
}

// ContentWindows.cpp adds the body's height to get the window's.
int TSubWindow::GetFrameHeight()
{
	return Skins()[SKIN_SUBWINDOW_EDGE_BOTTOM].height + Skins()[SKIN_SUBWINDOW_EDGE_TOP].height;
}

// A minimized window is its frame only.
int TSubWindow::getMinimumHeight()
{
	int Result = GetFrameHeight();
	if (Body != nullptr && !IsMinimized)
		Result += Body->getMinimumHeight();
	return Result;
}

// Nine background skins (Arrange).
TDialogWindow::TDialogWindow()
{
	Area = TRect(0, 0, 0, 0);
	Dragged = nullptr;
	Busy = false;
	FreeHeight = 0;
	UpdateFreeHeight = true;
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
}

// --- TDialogWindow ---------------------------------------------------------

// The catch blocks name no parameter.
//
// Issue #65: a sub-window's SetSize ends with the column's Rearrange(8)
// (FitChildren), which can delete the child being sized. The original then
// reads the deleted child's Next (a use after free); RemoveChild had set it
// to NULL before the delete, so its loop ends there. Ours ends there too,
// without the read: a child that is no longer in the column was deleted
// (strategies 2, 4, 8 and 32 delete children but never insert one, so the
// pointer compared is not a new child's). A child still in it goes on to its
// Next, as before.
void TDialogWindow::SetSize(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	Area.height = Height;
	Area.width = Width;
	TComponent *Child = FirstChild;
	while (Child != nullptr) {
		Child->SetSize(Width, Child->Height);
		Child = TWindow::IsChild(Child) ? Child->GetNext() : nullptr;
	}
	Rearrange(8, nullptr);
}

// The children's heights, whatever their minimum heights.
int TDialogWindow::getMinimumHeight()
{
	int Result = 0;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext())
		Result += Child->Height;
	return Result;
}

// The dragged child's place is a rectangle of SkinProperties.DraggedWindowPlaceColor; the
// child itself is drawn last, over the others.
void TDialogWindow::Draw(int Surface, int x, int y)
{
	int Position = 0;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if (Child == Dragged) {
			GetSurface()->FillRectangle(Surface, TRect(x, Position + y, Width, Dragged->Height),
				SkinProperties().DraggedWindowPlaceColor);
		} else {
			Child->Draw(Surface, Child->OffsetX + x, Child->OffsetY + y);
		}
		Position += Child->Height;
	}
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	TComponent::Draw(Surface, x, y);
	if (Dragged != nullptr)
		Dragged->Draw(Surface, Dragged->OffsetX + x, Dragged->OffsetY + y);
}

// The free space below the children (FreeHeight pixels from Top down) gets
// a frame of skins 26-33 around skin 7. The top row is cut to the free
// height, the bottom row starts below the top row's skins. Odd, kept: the
// bottom left skin (28) is as wide as the top left one (26), and the right
// edge (31) is as wide as skin 135.
void TDialogWindow::Arrange()
{
	const int Free = GetFreeHeight();
	for (int i = 0; i < DrawingListLength; i++)
		DrawingList[i] = TDrawing();
	if (Free != 0) {
		const int Top = Height - Free;
		int h = Free < Skins()[SKIN_BORDER_CORNER_TOPLEFT].height ? Free : Skins()[SKIN_BORDER_CORNER_TOPLEFT].height;
		if (h > 0)
			DrawingList[0] = TDrawing(SKIN_BORDER_CORNER_TOPLEFT, 0, Top, Skins()[SKIN_BORDER_CORNER_TOPLEFT].width, h, 0, 0);
		h = Free < Skins()[SKIN_BORDER_EDGE_TOP].height ? Free : Skins()[SKIN_BORDER_EDGE_TOP].height;
		if (h > 0)
			DrawingList[1] = TDrawing(SKIN_BORDER_EDGE_TOP, Skins()[SKIN_BORDER_CORNER_TOPLEFT].width, Top, Width - Skins()[SKIN_BORDER_CORNER_TOPRIGHT].width - Skins()[SKIN_BORDER_CORNER_TOPLEFT].width, h, 0, 0);
		h = Free < Skins()[SKIN_BORDER_CORNER_TOPRIGHT].height ? Free : Skins()[SKIN_BORDER_CORNER_TOPRIGHT].height;
		if (h > 0)
			DrawingList[2] = TDrawing(SKIN_BORDER_CORNER_TOPRIGHT, Width - Skins()[SKIN_BORDER_CORNER_TOPRIGHT].width, Top, Skins()[SKIN_BORDER_CORNER_TOPRIGHT].width, h, 0, 0);
		h = Free - Skins()[SKIN_BORDER_CORNER_TOPLEFT].height;
		if (h >= Skins()[SKIN_BORDER_CORNER_BOTTOMLEFT].height)
			h = Skins()[SKIN_BORDER_CORNER_BOTTOMLEFT].height;
		if (h > 0)
			DrawingList[3] = TDrawing(SKIN_BORDER_CORNER_BOTTOMLEFT, 0, Height - h, Skins()[SKIN_BORDER_CORNER_TOPLEFT].width, h, 0, 0);
		h = Free - Skins()[SKIN_BORDER_EDGE_TOP].height;
		if (h >= Skins()[SKIN_BORDER_EDGE_BOTTOM].height)
			h = Skins()[SKIN_BORDER_EDGE_BOTTOM].height;
		if (h > 0)
			DrawingList[4] = TDrawing(SKIN_BORDER_EDGE_BOTTOM, Skins()[SKIN_BORDER_CORNER_BOTTOMLEFT].width, Height - h, Width - Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_BORDER_CORNER_BOTTOMLEFT].width, h,
				0, 0);
		h = Free - Skins()[SKIN_BORDER_CORNER_TOPRIGHT].height;
		if (h >= Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].height)
			h = Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].height;
		if (h > 0)
			DrawingList[5] = TDrawing(SKIN_BORDER_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].width, Height - h, Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].width, h, 0, 0);
		h = Free - Skins()[SKIN_BORDER_CORNER_BOTTOMLEFT].height - Skins()[SKIN_BORDER_CORNER_TOPLEFT].height;
		if (h > 0)
			DrawingList[6] = TDrawing(SKIN_BORDER_EDGE_LEFT, 0, Skins()[SKIN_BORDER_CORNER_TOPLEFT].height + Top, Skins()[SKIN_BORDER_EDGE_LEFT].width, h, 0, 0);
		h = Free - Skins()[SKIN_BORDER_EDGE_BOTTOM].height - Skins()[SKIN_BORDER_EDGE_TOP].height;
		if (h > 0)
			DrawingList[7] = TDrawing(SKIN_BACKGROUND, Skins()[SKIN_BORDER_EDGE_LEFT].width, Skins()[SKIN_BORDER_EDGE_TOP].height + Top,
				Width - Skins()[SKIN_BORDER_EDGE_RIGHT].width - Skins()[SKIN_BORDER_EDGE_LEFT].width, h, 0, Top % Skins()[SKIN_BACKGROUND].height);
		h = Free - Skins()[SKIN_BORDER_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_BORDER_CORNER_TOPRIGHT].height;
		if (h > 0)
			DrawingList[8] = TDrawing(SKIN_BORDER_EDGE_RIGHT, Width - Skins()[SKIN_BORDER_EDGE_RIGHT].width, Skins()[SKIN_BORDER_CORNER_TOPRIGHT].height + Top, Skins()[SKIN_SUBWINDOW_EDGE_RIGHT].width, h, 0, 0);
	}
}

// Children are as wide as the column.
void TDialogWindow::InsertChild(TComponent *Child)
{
	TWindow::InsertChild(Child);
	Child->SetSize(Width, Child->Height);
}

void TDialogWindow::AppendChild(TComponent *Child)
{
	TWindow::AppendChild(Child);
	Child->SetSize(Width, Child->Height);
}

void TDialogWindow::DeleteChild(TComponent *Child)
{
	if (Dragged == Child)
		Dragged = nullptr;
	TWindow::DeleteChild(Child);
	Rearrange(32, nullptr);
}

void TDialogWindow::InsertChildAfter(TComponent *Previous, TComponent *Child)
{
	TWindow::InsertChildAfter(Previous, Child);
	Child->SetSize(Width, Child->Height);
}

// The children of a column are content windows.
TContentWindow *TDialogWindow::GetContentWindow(int Identifier)
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if ((static_cast<TContentWindow *>(Child))->Identifier == Identifier)
			return static_cast<TContentWindow *>(Child);
	}
	return nullptr;
}

// The windows that the next game opens again (not the containers).
void TDialogWindow::StoreOpenDialogs()
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if ((static_cast<TContentWindow *>(Child))->Identifier < CONTAINER_WINDOW)
			AddOpenDialog((static_cast<TContentWindow *>(Child))->Identifier);
	}
}

// Calls during a rearrangement (DeleteChild in FitChildren and MakeRoom, the
// children's SetSize) return at once. Strategy 4 lays the children out
// twice.
void TDialogWindow::Rearrange(int Strategy, TComponent *Work)
{
	if (Busy)
		return;
	Busy = true;
	switch (Strategy) {
	case 1:
		DragChild(Work);
		PositionChildren(true);
		break;
	case 2:
		PositionChildren(false);
		Shrink();
		break;
	case 4:
		PATELLA_ASSERT(Work!=nullptr);
		UpdateFreeHeight = false;
		PositionChildren(false);
		Grow(Work);
		PositionChildren(false);
		break;
	case 8:
		PositionChildren(false);
		FitChildren();
		break;
	case 16:
		PATELLA_ASSERT(Work!=nullptr);
		MakeRoom(Work);
		AppendChild(Work);
		PositionChildren(false);
		break;
	case 32:
		PositionChildren(false);
		break;
	default:
		TERROR1("unknown strategy", Strategy);
	}
	Arrange();
	UpdateFreeHeight = true;
	Busy = false;
}

// The dragged child keeps the position the mouse gave it.
void TDialogWindow::PositionChildren(bool SkipDragged)
{
	const int x = Area.x;
	int y = Area.y;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if (Child != Dragged || !SkipDragged)
			Child->SetPosition(x, y);
		y += Child->Height;
	}
}

// Strategy 8. The children that do not fit any more shrink, down to
// their minimum heights; those that still do not fit are deleted
// (DeleteChild's Rearrange returns at once). Clears Busy itself, before
// Rearrange does.
void TDialogWindow::FitChildren()
{
	int Space = Area.height;
	TComponent *Child = FirstChild;
	while (Child != nullptr) {
		if (Space < Child->Height) {
			TComponent *Delete = nullptr;
			if (Space < Child->getMinimumHeight()) {
				Delete = Child;
			} else if (Space < Child->Height) {
				Child->SetSize(Area.width, Space);
				Child->Arrange();
				if (Space < Child->Height)
					Delete = Child;
				else
					Space -= Child->Height;
			}
			Child = Child->GetNext();
			if (Delete != nullptr)
				DeleteChild(Delete);
		} else {
			Space -= Child->Height;
			Child = Child->GetNext();
		}
	}
	Busy = false;
}

// Strategy 2. While the children are higher than the column, the lowest
// child above its minimum height gives up what is missing (or what it can)
// and the children below it move up. Returns false when no child can shrink
// any more. The second GetFreeHeight cannot be positive here; the test is
// the binary's.
bool TDialogWindow::Shrink()
{
	while (GetFreeHeight() < 0) {
		TComponent *Child = GetLastChild();
		const int Missing = -GetFreeHeight();
		if (Missing < 0)
			break;
		TComponent *Shrinking;
		do {
			if (Child == nullptr)
				return false;
			Shrinking = Child;
			Child = Child->GetPrevious();
		} while (Shrinking->Height == Shrinking->getMinimumHeight());
		int DeltaHeight = Shrinking->Height - Shrinking->getMinimumHeight();
		if (Missing < DeltaHeight)
			DeltaHeight = Missing;
		PATELLA_ASSERT_VALUES(DeltaHeight>0, DeltaHeight);
		Shrinking->SetSize(Shrinking->Width, Shrinking->Height - DeltaHeight);
		for (TComponent *Next = Shrinking->GetNext(); Next != nullptr; Next = Next->GetNext())
			Next->SetPosition(Next->OffsetX, Next->OffsetY - DeltaHeight);
	}
	return true;
}

// Strategy 1. The dragged child swaps places with the child above it when
// its top edge is above that child's middle, and with the child below it
// when its bottom edge is below that child's middle.
void TDialogWindow::DragChild(TComponent *Work)
{
	Dragged = Work;
	if (Dragged != nullptr) {
		TComponent *Previous = nullptr;
		TComponent *Search = FirstChild;
		TComponent *Next;
		for (;;) {
			PATELLA_ASSERT(Search!=nullptr);
			Next = Search->GetNext();
			if (Dragged == Search)
				break;
			Previous = Search;
			Search = Next;
		}
		if (Previous != nullptr && Dragged->OffsetY < Previous->Height / 2 + Previous->OffsetY)
			SwapChildren(Previous, Dragged);
		if (Next != nullptr && Dragged->Height + Dragged->OffsetY > Next->Height / 2 + Next->OffsetY)
			SwapChildren(Next, Dragged);
	}
}

// Strategy 16. Before Work is appended, the children from the bottom up
// shrink to make room for its height (at least its minimum height); a
// child that would have to shrink below its minimum is deleted. Work gets
// at most the space there is then.
void TDialogWindow::MakeRoom(TComponent *Work)
{
	int Space = Area.height;
	for (TComponent *Child = GetFirstChild(); Child != nullptr; Child = Child->GetNext())
		Space -= Child->Height;
	if (Space < Work->Height) {
		int Needed = Work->getMinimumHeight();
		if (Space > Needed)
			Needed = Space;
		TComponent *Child = GetLastChild();
		while (Child != nullptr && Space < Needed) {
			TComponent *Previous = Child->GetPrevious();
			const int OldHeight = Child->Height;
			Child->SetSize(Child->Width, OldHeight - Needed + Space);
			const int NewHeight = Child->Height;
			Space += OldHeight - NewHeight;
			if (Space >= Needed)
				break;
			DeleteChild(Child);
			Space += NewHeight;
			Child = Previous;
		}
		if (Space >= Work->Height)
			Space = Work->Height;
		Work->SetSize(Work->Width, Space);
	}
}

// Strategy 4. The space Work gave up (the free height above the one before;
// UpdateFreeHeight is off) goes to the children below it, from the bottom
// up.
void TDialogWindow::Grow(TComponent *Work)
{
	TComponent *Child = GetLastChild();
	if (Child != Work) {
		const int Free = GetFreeHeight();
		if (Free > FreeHeight) {
			int Delta = Free - FreeHeight;
			for (; Child != nullptr && Delta > 0 && Child != Work; Child = Child->GetPrevious()) {
				const int OldHeight = Child->Height;
				Child->SetSize(Child->Width, OldHeight + Delta);
				Delta += OldHeight - Child->Height;
			}
		}
	}
}

// Negative when the children do not fit.
int TDialogWindow::GetFreeHeight()
{
	const int Available = Area.height;
	int Used = 0;
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext())
		Used += Child->Height;
	if (UpdateFreeHeight)
		FreeHeight = Available - Used;
	return Available - Used;
}

// --- TContentWindow --------------------------------------------------------

// The background is skin 7, tiled from the row that continues the column's
// pattern (BackgroundY).
void TContentWindow::Draw(int Surface, int x, int y)
{
	if (Background) {
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, Width, Height, SKIN_BACKGROUND, 0, BackgroundY);
	}
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	TComponent::Draw(Surface, x, y);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext())
		Child->Draw(Surface, Child->OffsetX + x, Child->OffsetY + y);
}

void TContentWindow::Arrange()
{
	TWindow::Arrange();
	BackgroundY = OffsetY % Skins()[SKIN_BACKGROUND].height;
}

// The catch blocks name no parameter.
void TContentWindow::ReleaseCapture(TComponent *Component)
{
	Dragging = false;
	TWindow::ReleaseCapture(Component);
}

// While dragging, the window follows the mouse inside the column, which
// rearranges the others around it.
bool TContentWindow::MouseMove(int x, int y)
{
	if (Dragging) {
		TDialogWindow *Column = static_cast<TDialogWindow *>(GetParent());
		int NewY = OffsetY - DragY + y;
		if (NewY < 0)
			NewY = 0;
		if (NewY > Column->Height - Height)
			NewY = Column->Height - Height;
		SetPosition(OffsetX, NewY);
		Column->Rearrange(1, this);
		return true;
	}
	return TWindow::MouseMove(x, y);
}

// A press that no child takes starts dragging the window:
// SetCapture(NULL) makes the parents send every mouse event here.
bool TContentWindow::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags)) {
		Dragging = true;
		SetCapture(nullptr);
		DragX = x;
		DragY = y;
		DragBottom = Height + OffsetY - y;
	}
	return true;
}

bool TContentWindow::LeftButtonUp(int x, int y, int Flags)
{
	if (Dragging) {
		ReleaseCapture(nullptr);
		(static_cast<TDialogWindow *>(GetParent()))->Rearrange(1, nullptr);
		return true;
	}
	return TWindow::LeftButtonUp(x, y, Flags);
}

// --- TSubWindow ------------------------------------------------------------

// The body is a child from the start; the buttons, the icon and the title
// come later from the setters.
TSubWindow::TSubWindow(int _Identifier, TSubWindowBody *_Body, bool _Resizable, bool _Background)
	: TContentWindow(_Identifier, _Background)
{
	IsMinimized = false;
	RestoreHeight = 0;
	Data68 = 0;
	DrawingListLength = 8;
	DrawingList.reset(new TDrawing[8]);
	UpButton = nullptr;
	CloseButton = nullptr;
	MinimizeButton = nullptr;
	Icon = nullptr;
	Title = nullptr;
	PATELLA_ASSERT(_Body!=nullptr);
	Body = _Body;
	TWindow::InsertChild(Body);
	if (_Resizable) {
		Resizer1 = new TResizer(this, false, true, false);
		Resizer2 = new TResizer(this, false, true, true);
		TWindow::AppendChild(Resizer1);
		TWindow::AppendChild(Resizer2);
	} else {
		Resizer1 = nullptr;
		Resizer2 = nullptr;
	}
}

// Not lower than the minimum height; a minimized window keeps its frame height
// (SubWindow.MinimizedHeight). The column is told whether the window grew (2),
// shrank (4) or kept its height (32), then fits its children into its height (8).
void TSubWindow::SetSize(int Width, int Height)
{
	TDialogWindow *Column = static_cast<TDialogWindow *>(GetParent());
	int Change = 0;
	if (IsMinimized) {
		TWindow::SetSize(Width, SkinProperties().SubWindow.MinimizedHeight);
	} else {
		if (Height > this->Height)
			Change = 1;
		if (Height < this->Height)
			Change = -1;
		if (Height < getMinimumHeight())
			Height = getMinimumHeight();
		TWindow::SetSize(Width, Height);
		SetChildArea(Body, SkinProperties().SubWindow.Body);
		FitToChild(Body, SkinProperties().SubWindow.Body, false);
		if (Resizer1 != nullptr)
			SetChildArea(Resizer1, SkinProperties().SubWindow.Resizer1);
		if (Resizer2 != nullptr)
			SetChildArea(Resizer2, SkinProperties().SubWindow.Resizer2);
	}
	Arrange();
	if (Column != nullptr) {
		switch (Change) {
		case -1:
			Column->Rearrange(4, this);
			break;
		case 0:
			Column->Rearrange(32, nullptr);
			break;
		case 1:
			Column->Rearrange(2, this);
			break;
		default:
			break;
		}
		Column->Rearrange(8, nullptr);
	}
}

// A container's body updates its "up" button first. The frame: skins
// 128-131 in the corners, 132 and 133 along the top and bottom edges, 134
// and 135 along the left and right ones. Odd, kept: the right edge starts
// at the top edge's height (Skins[132].height), not below the top right
// corner.
void TSubWindow::Arrange()
{
	TContentWindow::Arrange();
	if (Identifier >= CONTAINER_WINDOW) {
		PATELLA_ASSERT(Body!=nullptr);
		UpdateContainerUpButton(Body);
	}
	DrawingList[0] = TDrawing(SKIN_SUBWINDOW_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_SUBWINDOW_EDGE_TOP, Skins()[SKIN_SUBWINDOW_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_SUBWINDOW_CORNER_TOPRIGHT].width - Skins()[SKIN_SUBWINDOW_CORNER_TOPLEFT].width,
		Skins()[SKIN_SUBWINDOW_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_SUBWINDOW_CORNER_TOPRIGHT, Width - Skins()[SKIN_SUBWINDOW_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_SUBWINDOW_EDGE_LEFT, 0, Skins()[SKIN_SUBWINDOW_CORNER_TOPLEFT].height, Skins()[SKIN_SUBWINDOW_EDGE_LEFT].width, Height - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMLEFT].height - Skins()[SKIN_SUBWINDOW_CORNER_TOPLEFT].height,
		0, 0);
	DrawingList[4] = TDrawing(SKIN_SUBWINDOW_EDGE_RIGHT, Width - Skins()[SKIN_SUBWINDOW_EDGE_RIGHT].width, Skins()[SKIN_SUBWINDOW_EDGE_TOP].height, Skins()[SKIN_SUBWINDOW_EDGE_RIGHT].width,
		Height - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_SUBWINDOW_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_SUBWINDOW_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_SUBWINDOW_EDGE_BOTTOM, Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMLEFT].width, Height - Skins()[SKIN_SUBWINDOW_EDGE_BOTTOM].height, Width - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMLEFT].width,
		Skins()[SKIN_SUBWINDOW_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_SUBWINDOW_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
}

// The resizers get the press first, even where they overlap the body.
bool TSubWindow::LeftButtonDown(int x, int y, int Flags)
{
	if (!IsMinimized && Resizer1 != nullptr && In(x - Resizer1->OffsetX, 0, Resizer1->Width - 1)
		&& In(y - Resizer1->OffsetY, 0, Resizer1->Height - 1)
		&& Resizer1->LeftButtonDown(x - Resizer1->OffsetX, y - Resizer1->OffsetY, Flags))
		return true;
	if (!IsMinimized && Resizer2 != nullptr && In(x - Resizer2->OffsetX, 0, Resizer2->Width - 1)
		&& In(y - Resizer2->OffsetY, 0, Resizer2->Height - 1)
		&& Resizer2->LeftButtonDown(x - Resizer2->OffsetX, y - Resizer2->OffsetY, Flags))
		return true;
	return TContentWindow::LeftButtonDown(x, y, Flags);
}

bool TSubWindow::MouseMove(int x, int y)
{
	if (!IsMinimized && Resizer1 != nullptr && In(x - Resizer1->OffsetX, 0, Resizer1->Width - 1)
		&& In(y - Resizer1->OffsetY, 0, Resizer1->Height - 1)
		&& Resizer1->MouseMove(x - Resizer1->OffsetX, y - Resizer1->OffsetY))
		return true;
	if (!IsMinimized && Resizer2 != nullptr && In(x - Resizer2->OffsetX, 0, Resizer2->Width - 1)
		&& In(y - Resizer2->OffsetY, 0, Resizer2->Height - 1)
		&& Resizer2->MouseMove(x - Resizer2->OffsetX, y - Resizer2->OffsetY))
		return true;
	return TContentWindow::MouseMove(x, y);
}

// A double click that no child takes minimizes or restores the window
// (event 9).
bool TSubWindow::LeftButtonDoubleClick(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDoubleClick(x, y, Flags))
		HandleEvent(EVENT_WINDOW_MINIMISE);
	return true;
}

// Event 10 (the close button) deletes the window.
void TSubWindow::HandleEvent(int Event)
{
	if (Event == EVENT_WINDOW_CLOSE) {
		GetParent()->DeleteChild(this);
	} else if (Event == EVENT_WINDOW_MINIMISE) {
		if (IsMinimized)
			Maximize();
		else
			Minimize();
	}
}

// The body is told first (+0xa0, without a NULL check) and leaves the
// window with the resizers.
void TSubWindow::Minimize()
{
	PATELLA_ASSERT(IsMinimized==false);
	IsMinimized = true;
	RestoreHeight = Height;
	Body->Minimize();
	if (Resizer1 != nullptr)
		RemoveChild(Resizer1);
	if (Resizer2 != nullptr)
		RemoveChild(Resizer2);
	if (Body != nullptr)
		RemoveChild(Body);
	if (MinimizeButton != nullptr)
		MinimizeButton->SetSkins(SKIN_MAXIMISE_BUTTON_UP, SKIN_MAXIMISE_BUTTON_DOWN);
	SetSize(Width, SkinProperties().SubWindow.MinimizedHeight);
}

void TSubWindow::Maximize()
{
	PATELLA_ASSERT(IsMinimized==true);
	IsMinimized = false;
	Body->Maximize();
	if (Body != nullptr)
		AppendChild(Body);
	if (Resizer1 != nullptr)
		AppendChild(Resizer1);
	if (Resizer2 != nullptr)
		AppendChild(Resizer2);
	if (MinimizeButton != nullptr)
		MinimizeButton->SetSkins(SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN);
	SetSize(Width, RestoreHeight);
}

void TSubWindow::ShowUpButton()
{
	if (UpButton != nullptr && !IsChild(UpButton))
		InsertChild(UpButton);
}

// The button stays the window's (the destructor deletes it).
void TSubWindow::HideUpButton()
{
	if (UpButton != nullptr && IsChild(UpButton) == true)
		RemoveChild(UpButton);
}

// Each setter deletes the part it replaces.
void TSubWindow::SetUpButton(TComponent *Button)
{
	if (UpButton != nullptr) {
		if (IsChild(UpButton))
			DeleteChild(UpButton);
		else
			DeleteComponent(UpButton);
	}
	UpButton = Button;
	InsertChild(Button);
	SetChildPosition(UpButton, SkinProperties().SubWindow.UpButton.x, SkinProperties().SubWindow.UpButton.y);
}

void TSubWindow::SetCloseButton(TComponent *Button)
{
	if (CloseButton != nullptr) {
		if (IsChild(CloseButton))
			DeleteChild(CloseButton);
		else
			DeleteComponent(CloseButton);
	}
	CloseButton = Button;
	InsertChild(Button);
	SetChildPosition(CloseButton, SkinProperties().SubWindow.CloseButton.x, SkinProperties().SubWindow.CloseButton.y);
}

void TSubWindow::SetMinimizeButton(TSkinButton *Button)
{
	if (MinimizeButton != nullptr) {
		if (IsChild(MinimizeButton))
			DeleteChild(MinimizeButton);
		else
			DeleteComponent(MinimizeButton);
	}
	MinimizeButton = Button;
	InsertChild(Button);
	SetChildPosition(MinimizeButton, SkinProperties().SubWindow.MinimizeButton.x, SkinProperties().SubWindow.MinimizeButton.y);
}

void TSubWindow::SetIcon(TComponent *_Icon)
{
	if (Icon != nullptr) {
		if (IsChild(Icon))
			DeleteChild(Icon);
		else
			DeleteComponent(Icon);
	}
	Icon = _Icon;
	InsertChild(_Icon);
	SetChildArea(Icon, SkinProperties().SubWindow.Icon);
}

void TSubWindow::SetTitle(TComponent *_Title)
{
	if (Title != nullptr) {
		if (IsChild(Title))
			DeleteChild(Title);
		else
			DeleteComponent(Title);
	}
	Title = _Title;
	InsertChild(_Title);
	SetChildArea(Title, SkinProperties().SubWindow.Title);
}
// NOLINTEND(cppcoreguidelines-owning-memory)

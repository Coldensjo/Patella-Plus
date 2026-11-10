// types/GraphicsTypes.h: the value types of the drawing interface (TRect, TRGBColor, TFont and the text
// alignments). A header of their own so that Surface.cpp (TSurface's functions take them) and Skins.h (SkinProperties
// holds them) need them without graphics.h. Header only, portable. graphics.h exports it.

#pragma once

#include "compat/msvc.h" // PATELLA_NO_SANITIZE_ADDRESS

// Text alignment (TFont::Alignment, the Alignment parameters).
// An unnamed enum of int constants (TFont's int Alignment), unscoped as the other unnamed ones.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, above
	LEFT_ALIGN = 0,
	CENTER_ALIGN = 1,
	RIGHT_ALIGN = 2,
};

// --- Graphics types -----------------------------------------------------------
// The types of the drawing interface, used by every window file (GUI.h
// includes graphics.h, which exports this header). Default construction
// zeroes them.

// A rectangle: position and size. The lower-case names are the original's
// (its assertions' texts have them), as TSkin's (Skins.h) and TDrawing's
// (GUI.h).
struct TRect { // passed by value
	constexpr TRect() : x(0), y(0), width(0), height(0) {}
	// The parameters are not named as the members: patella_core's GCC -Wshadow, since Skins.h includes this header.
	constexpr TRect(int X, int Y, int Width, int Height) : x(X), y(Y), width(Width), height(Height) {}
	int x, y, width, height;
};

struct TRGBColor { // passed by value
	constexpr TRGBColor() : R(0), G(0), B(0) {}
	constexpr TRGBColor(int Red, int Green, int Blue) : R(Red), G(Green), B(Blue) {}
	// A colour of the 6x6x6 palette, Color = R*36 + G*6 + B with steps of 51.
	TRGBColor(int Color)
	{
		R = Color / 36 * 51;
		B = Color % 6 * 51;
		G = Color / 6 % 6 * 51;
	}
	// Not checked by AddressSanitizer: GetVideoPicture compares the colour key of the picture after the one it
	// draws, past the end of Pictures for the last one, as the original does (graphics.cpp).
	PATELLA_NO_SANITIZE_ADDRESS bool operator==(TRGBColor Other) const
	{
		return R == Other.R && G == Other.G && B == Other.B;
	}
	bool operator!=(TRGBColor Other) const
	{
		return !(*this == Other);
	}
	int R, G, B;
};

// The colour of TFont's default constructor.
constexpr TRGBColor DefaultFontColor(224, 224, 224);

// The numbers of the four fonts (TFont::FontNumber, the FontNumber parameters, InitFont's FontName), 1 to
// NumberOfFonts. The original's names are not known; these say what graphics.cpp's InitFonts and the fonts' pictures
// show. Enumerators for the reason in GUI.h.
enum : int {           // NOLINT(cppcoreguidelines-use-enum-class): int constants
	FONT_SHARP = 1,    // picture 3, characters of 8x16 with hard edges: the tooltips
	FONT_BORDERED = 2, // picture 5, 16x16 with a black border: the text on the game window, the menus, the console's tabs
	FONT_SMALL = 3,    // picture 6, 8x8: the buttons' captions; GetFontLineHeight's font for a number out of range
	FONT_SMOOTH = 4,   // picture 8, FONT_SHARP's characters with smoothed edges: TFont's default and the console's text
};

// A text format: font, colour and alignment.
struct TFont { // passed by value
	TFont() : FontNumber(FONT_SMOOTH), Color(DefaultFontColor), Alignment(LEFT_ALIGN) {}
	int FontNumber;
	TRGBColor Color;
	int Alignment; // LEFT_ALIGN, CENTER_ALIGN, RIGHT_ALIGN
};

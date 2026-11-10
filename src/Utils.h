// Utils.cpp: Latin-1 character and string helpers, outfit colours, and the
// packet buffers. Member and parameter names come from the assertion strings
// ("data", "size", "Position", "Size", "NewSize", "s", "s1", "Destination",
// "Source", "Text", "Buffer", "Number"); function and class names are ours.

#pragma once

#include "common.h"
#include "compat/span.h"

// Characters are Latin-1; the letter tests include the German umlauts and ß.
int IsSpace(int c);
int IsAlpha(int c);
int IsDigit(int c);
int ToLower(int c);
int ToUpper(int c);
void HSVToRGB(double H, double S, double V, int *R, int *G, int *B);
void GetOutfitColor(int Color, int *R, int *G, int *B); // Color 0..132

void LowerString(char *s);
void CapitalizeString(char *s);
int CompareStrings(const char *s1, const char *s2, int Count); // caseless
void CopyString(patella::TSpan<char> DestinationBytes, const char *Source);
void TrimString(char *Destination, const char *Source);
bool ShortenString(char *Text, int MaxLength);
void FormatNumber(int Number, patella::TSpan<char> BufferBytes); // "1,234,567"
// CompareStrings and ShortenString keep the original's parameters: Count limits the comparison of two C strings (-1:
// none) and MaxLength is the length Text is cut to, not the size of a buffer.

class TBuffer {
public:
	void SetSize(int NewSize)
	{
		PATELLA_ASSERT_VALUES(NewSize>=0, NewSize);
		Size = NewSize;
	}

	uint8 *Data;
	int Size;
	int Position;
};

// Little-endian reads; strings are a u16 length followed by the characters.
class TReadBuffer : public TBuffer {
public:
	explicit TReadBuffer(patella::TSpan<uint8> Bytes);
	uint8 GetByte();
	uint16 GetWord();
	uint32 GetQuad();
	void GetString(patella::TSpan<char> Text); // see Utils.cpp
	char *GetString();                         // returns a new[] copy
};

class TWriteBuffer : public TBuffer {
public:
	explicit TWriteBuffer(patella::TSpan<uint8> Bytes);
	void PutByte(uint8 Data);
	void PutWord(uint16 Data);
	void PutQuad(uint32 Data);
	void PutString(const char *s); // a C string (see Utils.cpp)
	void PutBytes(patella::TSpan<const uint8> Bytes);
};

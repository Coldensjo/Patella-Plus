// Script.h: reading script files (Tibia.cfg), reading and writing binary files.
//
// The class, member and token names follow CipSoft's game server, whose
// TReadScriptFile has the same tokenizer. The client's version reads a single
// file (no #include) and has a smaller string buffer.
//
// The constructors, destructors and close() functions are defined here. The
// FILE * members are TFiles (types/File.h): fopen, fclose and the reads and
// writes with an owner. Each class's TryOpen returns open's failures.

#pragma once

#include "common.h"
#include "compat/expected.h"
#include "compat/span.h"
#include "types/File.h"
#include "types/FixedString.h"
#include <cstring>

inline constexpr int MAX_IDENT_LENGTH = 20;    // "identifier too long"
inline constexpr int MAX_STRING_LENGTH = 1000; // "string too long" at 999 characters

// The kind of the current token (TReadScriptFile::Token). Only assigned and compared, so it is scoped.
enum class TOKEN : int {
	ENDOFFILE,
	IDENTIFIER,
	NUMBER,
	STRING,
	BYTES,      // "1-2-3"
	COORDINATE, // "[x,y,z]"
	SPECIAL,    // any other character; "<=" 'L', ">=" 'G', "<>" 'N', "->" 'I'
};

class TReadScriptFile {
public:
	TReadScriptFile(const TReadScriptFile &) = delete;
	TReadScriptFile &operator=(const TReadScriptFile &) = delete;
	// Token is not set.
	TReadScriptFile()
	{
		memset(Filename, 0, sizeof(Filename));
		Line = 0;
		memset(String, 0, sizeof(String));
		Bytes = reinterpret_cast<uint8 *>(String.Chars);
		Number = 0;
		Special = '#';
	}

	~TReadScriptFile()
	{
		File.Close();
	}

	// Resets everything but Token, like the constructor.
	void close()
	{
		File.Close();
		memset(Filename, 0, sizeof(Filename));
		Line = 0;
		memset(String, 0, sizeof(String));
		Number = 0;
		Bytes = reinterpret_cast<uint8 *>(String.Chars);
		Special = '#';
	}

	// open's failures ("file still open", "failed to open file") returned instead of thrown; open throws them, as the
	// original.
	[[nodiscard]] patella::TExpected<void, TError> TryOpen(const char *FileName);
	void open(const char *FileName);
	void error(const char *Text);
	void nextToken();
	char *getIdentifier();
	int getNumber();
	char *getString();
	char getSpecial();
	char *readIdentifier();
	int readNumber();
	char *readString();
	void readSymbol(char Symbol);

	TOKEN Token;
	TFile File;
	char Filename[1024]; // NOLINT(modernize-avoid-c-arrays): the file errors' reason (a std::string)
	int Line;
	TFixedString<MAX_STRING_LENGTH> String; // identifier or string
	uint8 *Bytes;                           // points to String
	int Number;
	char Special;
};

// The destructor is close().
class TReadBinaryFile {
public:
	TReadBinaryFile(const TReadBinaryFile &) = delete;
	TReadBinaryFile &operator=(const TReadBinaryFile &) = delete;
	TReadBinaryFile()
	{
		memset(Filename, 0, sizeof(Filename));
	}

	~TReadBinaryFile()
	{
		close();
	}

	void close()
	{
		File.Close();
	}

	// open's failures ("file still open", "failed to open file") returned instead of thrown; open throws them, as the
	// original.
	[[nodiscard]] patella::TExpected<void, TError> TryOpen(const char *FileName);
	void open(const char *FileName);
	void seek(int Offset);
	[[nodiscard]] bool eof() const;
	uint8 readByte();
	uint16 readWord();
	int readQuad();
	char *readString();
	void readBytes(patella::TSpan<uint8> Bytes);

	TFile File;
	char Filename[1024]; // NOLINT(modernize-avoid-c-arrays): the file errors' reason
};

class TWriteBinaryFile {
public:
	TWriteBinaryFile(const TWriteBinaryFile &) = delete;
	TWriteBinaryFile &operator=(const TWriteBinaryFile &) = delete;
	TWriteBinaryFile()
	{
		memset(Filename, 0, sizeof(Filename));
	}

	~TWriteBinaryFile()
	{
		close();
	}

	void close()
	{
		File.Close();
	}

	// Opens the file; a failure ("file still open", "failed to open file") is returned, not thrown.
	[[nodiscard]] patella::TExpected<void, TError> TryOpen(const char *FileName);
	void writeByte(uint8 Byte);
	void writeWord(uint16 Word);
	void writeQuad(uint32 Quad);
	void writeBytes(patella::TSpan<const uint8> Bytes);
	void writeString(const char *Text);

	TFile File;
	char Filename[1024]; // NOLINT(modernize-avoid-c-arrays): the file errors' reason
};

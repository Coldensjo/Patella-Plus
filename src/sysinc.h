// sysinc.h: system helpers (time, files, the clipboard, error texts).

#pragma once

#include <array>

int GetMilliseconds();
bool IsClipboardEmpty();
const char *GetErrorText(int Error);
const char *GetErrorHint(int Error);
void InitSysinc();
void ExitSysinc();
void ResizeClipboardBuffer(unsigned int Size);
char *GetClipboardText();
bool SetClipboardText(const char *Text);

// One half of the XOR-encoded window title and class name; Player.cpp has the
// keys and decodes them.
extern const std::array<int, 8> TitleCode;
extern const std::array<int, 11> ClassNameCode;

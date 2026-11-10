// Resource IDs of Patella, the values of Tibia.exe's .rsrc section: the client's icon and its
// two cursors (Patella.rc). The names are ours; the original's resource.h is not known.

#ifndef PATELLA_RESOURCE_H
#define PATELLA_RESOURCE_H

// Icon of the main window's class (SdlFrame.cpp gives SDL its number).
#define IDR_MAINFRAME 128

// Cursors of the client area (SdlFrame.cpp loads them by their numbers). Named after their pictures.
#define IDC_CROSSHAIR 132 // 0x84: DefaultCursor, four arrows pointing inwards
#define IDC_MAGNIFIER 137 // 0x89: HandCursor (mouse cursor 6), a magnifying glass

#endif

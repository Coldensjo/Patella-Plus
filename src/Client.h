// Client.h: TClient, the one object that owns the client's state.
//
// The original's globals (the file-scope data of each .cpp, and the shared ones of the headers) are in subsystem
// objects that TClient owns and passes to each other: TPreferences (Preferences.cpp), TNetwork (Network.cpp),
// TMapState (the game state: Objects.cpp, Map.cpp, Creatures.cpp, Inventory.cpp; MapState.h), TCommunication
// (Communication.cpp and Status.cpp's session), TGraphics (graphics.cpp, Surface.cpp and Skins.cpp with GUI.cpp's
// SkinProperties; graphics.h) and TGUI (Talk.cpp, Help.cpp, Status.cpp's user interaction and the windows' data;
// GUI.h). The platform's services are interfaces (platform/Platform.h): TClient takes them in its constructor and
// passes them on (TPreferences the file system, TNetwork the socket).
//
// There is one TClient, made on first use by Client() (Client.cpp). Each subsystem's header declares an accessor for
// its object (Preferences.h: Preferences()), which Client.cpp defines. How code reaches a subsystem, through what its
// own class holds or through the accessor, is written there. Only Client.cpp includes this header: the subsystems do
// not depend on each other through it.

#pragma once

#include "Communication.h"
#include "GUI.h"
#include "MapState.h"
#include "Network.h"
#include "Preferences.h"
#include "graphics.h"
#include "platform/Platform.h"

namespace patella {

class TClient {
public:
	// The platform's services (platform/Platform.h), which the subsystems take in their constructors.
	explicit TClient(platform::TPlatform &Platform);
	TClient(const TClient &) = delete;
	TClient &operator=(const TClient &) = delete;
	TClient(TClient &&) = delete;
	TClient &operator=(TClient &&) = delete;
	~TClient() = default;

	// The subsystems, in the order they are made (a later one may take an earlier one in its constructor).
	TPreferences Preferences;
	TNetwork Network;
	TMapState MapState;
	TCommunication Communication;
	TGraphics Graphics;
	TGUI GUI;
};

// The one TClient, made on first use (its members start as the original's statics did), on the process's platform
// (platform::Platform(), the entry point's executable's).
TClient &Client();

} // namespace patella

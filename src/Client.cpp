// Client.cpp: the one TClient and the subsystems' accessors (Client.h).

#include "Client.h"

#include "Communication.h"
#include "Creatures.h"
#include "GUI.h"
#include "Help.h"
#include "Inventory.h"
#include "Map.h"
#include "MapState.h"
#include "Network.h"
#include "Objects.h"
#include "Preferences.h"
#include "Status.h"
#include "Skins.h"
#include "Surface.h"
#include "Talk.h"
#include "graphics.h"
#include "common.h"
#include "platform/Platform.h"
#include <cstdint>

namespace patella {

namespace {

// Status.cpp's session and message queue for TNetwork (TNetworkSession). TSession (TCommunication's) and TGUI's
// TInteraction are made after TNetwork, so it stays an adapter (a reference to them would point up). It was kept rather
// than an interface TClient wires into TNetwork after construction: that would be a pointer TNetwork holds unset
// between its construction and TClient's (a two-phase start the golden runner, the unit tests and the test mode would
// each have to reach), and the golden Network test fakes Status.cpp's free functions, which this adapter calls. It goes
// with the bridge or with the ISocket events. Stateless: each member calls Status.cpp's function of its name (from
// another object file than Network.cpp's, so the golden Network test's fakes of those functions are reached as before).
class TNetworkSessionAdapter final : public TNetworkSession {
public:
	void SetConnectionState(int State) override { ::SetConnectionState(State); }
	int GetConnectionState() override { return ::GetConnectionState(); }
	int GetActiveCharacter() override { return ::GetActiveCharacter(); }
	const char *GetCharacterIPAddress(int Number) override { return ::GetCharacterIPAddress(Number); }
	int GetCharacterPort(int Number) override { return ::GetCharacterPort(Number); }
	void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2) override { ::AddMessage(Type, Param1, Param2); }
};

// Made on first use, as TClient is (a static initialiser of another file may make TClient first).
TNetworkSession &NetworkSessionAdapter()
{
	static TNetworkSessionAdapter Session;
	return Session;
}

// Talk.cpp's default console for TMap (TMapConsole) and Talk.cpp's, Help.cpp's and Status.cpp's functions for
// TCreatures and TPlayerState (TCreaturesGUI): TGUI's, made after TMapState, so adapters as TNetworkSessionAdapter is.
// Stateless: each member calls the free functions of the original's code (from another object file than Map.cpp's and
// Creatures.cpp's, so the golden tests' fakes of those functions are reached as before).
class TMapConsoleAdapter final : public TMapConsole {
public:
	bool HasDefaultConsole() override { return Talk().DefaultTextBuffer != nullptr; }
	void AddDefaultConsoleLine(const char *Message) override
	{
		::AddTextBufferLine(Talk().DefaultTextBuffer, 0, Message, TALK_EVENT_STATUS_MESSAGE, nullptr, 0);
		::SetConsoleChanged();
	}
};

class TCreaturesGUIAdapter final : public TCreaturesGUI {
public:
	void SetStatusMessage(const char *Message, bool Permanent) override { ::SetStatusMessage(Message, Permanent); }
	void AddEventHelp(int Number) override { ::AddEventHelp(Number); }
	void CheckEventHelpName(const char *Name) override { ::CheckEventHelpName(Name); }
	void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2) override { ::AddMessage(Type, Param1, Param2); }
	int GetActionNumber() override { return ::GetActionNumber(); }
};

// TGraphics' signatures for TCommunication's login packet (TLoginSignatures): TGraphics is made after TCommunication,
// so an adapter as the ones above. Stateless: it reads the one TClient's TGraphics.
class TGraphicsSignatures final : public TLoginSignatures {
public:
	uint32 SpriteSignature() override { return Graphics().SpriteSignature; }
	uint32 PictureSignature() override { return Graphics().PictureSignature; }
};

TLoginSignatures &GraphicsSignatures()
{
	static TGraphicsSignatures Signatures;
	return Signatures;
}

TMapConsole &MapConsoleAdapter()
{
	static TMapConsoleAdapter Console;
	return Console;
}

TCreaturesGUI &CreaturesGUIAdapter()
{
	static TCreaturesGUIAdapter Adapter;
	return Adapter;
}

} // namespace

TClient::TClient(platform::TPlatform &Platform)
	: Preferences(Platform.FileSystem), Network(Preferences, NetworkSessionAdapter(), Platform.Socket),
	  MapState(MapConsoleAdapter(), CreaturesGUIAdapter()), Communication(Network, MapState, GraphicsSignatures()), GUI(Network, Communication)
{
}

TClient &Client()
{
	static TClient Instance(platform::Platform());
	return Instance;
}

} // namespace patella

// The accessors: how code reaches the one client's subsystems. Each subsystem's header declares its own.
//
// A member function of a subsystem class uses what its class holds: its own members, and the references to other
// subsystems that its class was given in its constructor (Network.SendByte(...) inside TCommunication). Every other
// caller (the windows and dialogs, the free functions, a subsystem that holds no reference to the one it needs) calls
// the accessor: Talk().AddTalkbox(...). Where a class has a member or a function has a variable of an accessor's
// name, the accessor is written ::GUI().

patella::TPreferences &Preferences()
{
	return patella::Client().Preferences;
}

patella::TNetwork &Network()
{
	return patella::Client().Network;
}

patella::TCommunication &Communication()
{
	return patella::Client().Communication;
}

patella::TSession &Session()
{
	return patella::Client().Communication.Session;
}

patella::TObjectTypes &ObjectTypes()
{
	return patella::Client().MapState.ObjectTypes;
}

patella::TMap &Map()
{
	return patella::Client().MapState.Map;
}

patella::TCreatures &Creatures()
{
	return patella::Client().MapState.Creatures;
}

patella::TPlayerState &PlayerState()
{
	return patella::Client().MapState.PlayerState;
}

patella::TInventory &Inventory()
{
	return patella::Client().MapState.Inventory;
}

patella::TGraphics &Graphics()
{
	return patella::Client().Graphics;
}

patella::TGraphicsEngine &GraphicsEngine()
{
	return patella::Client().Graphics.Engine;
}

patella::TSkins &SkinData()
{
	return patella::Client().Graphics.Skins;
}

patella::TGUI &GUI()
{
	return patella::Client().GUI;
}

patella::TTalk &Talk()
{
	return patella::Client().GUI.Talk;
}

patella::THelp &Help()
{
	return patella::Client().GUI.Help;
}

patella::TInteraction &Interaction()
{
	return patella::Client().GUI.Interaction;
}

// Network.cpp: login server list, connecting, packet framing and XTEA.
// Function names are ours; parameter names come from the binary's assertion
// and catch strings.
//
// Patella-Modern issue #105 (M7, notes/decisions.md, "#104: TClient and the subsystem objects" and "#105"): the data
// is TNetwork's, which TClient owns (Client.h); the free functions of the original's names are the bridge that the
// callers use until they hold the object (bridge::Network(), defined in Client.cpp).

#pragma once

#include "common.h"
#include "Events.h"
#include "TXTEA.h"
#include "Utils.h"
#include "compat/span.h"
#include "platform/Socket.h"
#include "platform/Window.h"
#include "types/FixedString.h"
#include <array>
#include <cstddef>
#include <cstdint>

inline constexpr int NUMBEROFSERVER = 4;

// Error codes besides the WinSock ones.
inline constexpr int ERROR_LOGINSERVER_OVERLOADED = 200001;
inline constexpr int ERROR_DISCONNECTED = 200002;

// The message NetworkWindow gets when ResolveHostname's lookup is done (WM_USER + 1; MainFrm.cpp's OnHostResolved,
// which asserts it): a number, so that Network.cpp needs no Windows header (issue #110).
inline constexpr unsigned int HOSTNAME_RESOLVED_MESSAGE = 0x0400 + 1;

// The size of the buffer GetErrorMessage writes into: every caller passes new char[1024].
inline constexpr std::size_t ERROR_MESSAGE_SIZE = 1024;

struct TServer {
	TFixedString<100> Address; // +0x00
	int Port;                  // +0x64
	bool Available;            // +0x68, false after a failed attempt
	bool Resolved;             // +0x69, Address is a numeric IP address
	int ErrorCode;             // +0x6c, last error for this server
};
// The original's layout (no pointers: the same on x64), with Address a TFixedString (issue #57).
static_assert(IsFixedStringLayout<100>);
static_assert(sizeof(TServer) == 0x70 && alignof(TServer) == 4, "TServer is 0x70 bytes");
static_assert(offsetof(TServer, Address) == 0x00 && offsetof(TServer, Port) == 0x64
		&& offsetof(TServer, Available) == 0x68 && offsetof(TServer, Resolved) == 0x69
		&& offsetof(TServer, ErrorCode) == 0x6c,
	"TServer's fields");

namespace patella {

class TPreferences;

// What Network.cpp calls in the files above it (issue #105): Status.cpp's connection state and the active
// character's address (TCommunication's session), and the message queue (TGUI's). TNetwork reaches them only
// through this, so that it does not depend on those files; the functions are Status.h's of the same names. Until
// their objects implement it, Client.cpp's adapter calls Status.cpp's functions.
class TNetworkSession {
public:
	TNetworkSession() = default;
	TNetworkSession(const TNetworkSession &) = delete;
	TNetworkSession &operator=(const TNetworkSession &) = delete;
	TNetworkSession(TNetworkSession &&) = delete;
	TNetworkSession &operator=(TNetworkSession &&) = delete;
	virtual void SetConnectionState(int State) = 0;
	virtual int GetConnectionState() = 0;
	virtual int GetActiveCharacter() = 0;
	virtual const char *GetCharacterIPAddress(int Number) = 0;
	virtual int GetCharacterPort(int Number) = 0;
	virtual void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2) = 0;

protected:
	~TNetworkSession() = default;
};

// Network.cpp's data and functions (issue #105): the globals of the original, with Control.cpp's ReconnectTimeout and
// Reconnect and MainFrm.cpp's PacketHeader, are the members, with the same initial values (the others 0, as the
// original's .bss), and the functions the members of the same names. The data is public: the golden Network test
// reads and sets every member (network_tests.cpp's views), the MainFrm and Control tests PacketHeader, Reconnect and
// ReconnectTimeout, Control.cpp the last two, MainFrm.cpp and Player.cpp PacketHeader.
//
// Issue #109: the socket and the name resolution are the platform's ISocket (platform/Socket.h), which TNetwork takes
// in its constructor and calls where it called WinSock and CAsyncSocket; the connection's events come to it as
// ISocketEvents (OnConnect, OnReceive, OnClose: the TSocket's of Network.cpp until then).
class TNetwork final : public platform::ISocketEvents {
public:
	TNetwork(TPreferences &PreferencesRef, TNetworkSession &SessionRef, platform::ISocket &SocketsRef);
	TNetwork(const TNetwork &) = delete;
	TNetwork &operator=(const TNetwork &) = delete;
	TNetwork(TNetwork &&) = delete;
	TNetwork &operator=(TNetwork &&) = delete;
	~TNetwork() = default;

	void InitServerList();
	void SetTestServerPort(int Port);
	void ResetServerAvailability();
	void SetConnectionTimeout(int Timeout);
	void SetNetworkWindow(platform::TWindowHandle Window);
	void SetTestServerAddress(const char *Address);
	void SetServerError(int errorcode);
	void SetServerIPAddress(const char *ipaddress);
	bool GetCurrentServer(TServer **Server);
	int GetServerErrorCode();

	void BeginCommand(int CommandType);
	void SendByte(uint8 Data);
	void SendWord(uint16 Data);
	void SendQuad(uint32 Data);
	void SendString(const char *Text);
	void SendBytes(TSpan<const uint8> Bytes);
	void SendCommand(bool Encrypt);

	void PrepareNextPacket();
	int GetNextCommand();
	uint8 GetByte();
	uint16 GetWord();
	uint32 GetQuad();
	void GetString(TSpan<char> Text);
	char *GetString();

	void CheckConnectionTimeout();
	void GetErrorMessage(TSpan<char, ERROR_MESSAGE_SIZE> MessageBuffer, int ErrorCode, int ErrorType);
	bool ResolveHostname(const char *Hostname);
	[[nodiscard]] const char *GetResolvedIPAddress() const;
	int CreateSocket();
	int ConnectSocket(const char *IPAddress, int Port, bool SetTimeout);
	void SetSymmetricKey(const uint8 *Key);
	void Disconnect(bool force);
	void ConnectToLoginServer();
	void ConnectToGameServer();
	void ShutdownConnection() const;
	void InitNetwork(void (*ConnectHandler)(), void (*ReceiveHandler)(), void (*CloseHandler)());
	void ExitNetwork();

	// The socket's events: TSocket's OnConnect, OnReceive and OnClose call these.
	void OnConnect(int ErrorCode) override;
	void OnReceive(platform::TSocket &Receiver, int ErrorCode) override;
	void OnClose() override;

	// Data, in the original's .data/.bss order.
	int HostnameResolveBufferSize = platform::HostEntrySize; // initialised to 0x400 (MAXGETHOSTSTRUCT)
	int ReconnectTimeout{};                                  // Control.cpp's: GetMilliseconds time
	bool Reconnect{};                                        // Control.cpp's: set by event 0x3f
	std::array<uint8, 16> PacketHeader{};                    // MainFrm.cpp's: the first bytes of the last packet
	int CurrentServer{};
	TServer TestServer{};
	std::array<TServer, NUMBEROFSERVER> Servers{};
	void *HostnameRequest{};       // ResolveHostname's request (a HANDLE)
	char *HostnameResolveBuffer{}; // a HOSTENT filled by WSAAsyncGetHostByName
	int ReceiveDataSize{};         // bytes in ReceiveData
	int ReceivePacketEnd{};        // end of the complete packets
	int ReceiveRead{};             // start of the next unread packet
	// A frame whose inner length runs past it is dropped in PrepareNextPacket (issues #72 and #139), so
	// ReceiveBuffer's reads stay inside the frame: Tibia.exe read on past ReceiveData, into SendData.
	std::array<uint8, 0x4000> ReceiveData{};
	std::array<uint8, 0x800> SendData{};
	platform::TWindowHandle NetworkWindow{}; // receives WM_USER+1 when a name is resolved
	void (*CloseCallback)(){};
	void (*ReceiveCallback)(){};
	void (*ConnectCallback)(){};
	platform::TSocket *Socket{}; // the platform's connection (issue #109)
	int TimeoutCounter{};
	int TimeoutLimit{}; // in CheckConnectionTimeout calls
	TWriteBuffer SendBuffer{SendData};
	TReadBuffer ReceiveBuffer{{ReceiveData.data(), 0}};
	TXTEA SymmetricKey;
	int RecursionProtection{};

	// Not the original's (issues #72 and #139, deliberate differences; notes/decisions.md, "#139"): a server frame
	// the client cannot use is dropped, a warn line goes to the developer log, and the connection goes on with the
	// next frame. ReceiveSkip: the bytes still to come of a frame too large for ReceiveData (OnReceive reads and
	// drops them).
	int ReceiveSkip{};

private:
	void DropFrame(int FrameEnd);
	void ApplyServerOverride();

	TPreferences &Preferences;
	TNetworkSession &Session;
	platform::ISocket &Sockets;

	// The arrays std::arrays (issue #60).
	static_assert(sizeof(Servers) == NUMBEROFSERVER * sizeof(TServer) && sizeof(ReceiveData) == 0x4000
			&& sizeof(SendData) == 0x800 && sizeof(PacketHeader) == 16,
		"Network.cpp's arrays");
};

// The transition's access path (issue #105; notes/decisions.md, "#104: TClient and the subsystem objects"): the
// network of the one TClient (Client.cpp). It goes when no free function needs it.
namespace bridge {
TNetwork &Network();
} // namespace bridge

} // namespace patella

// The original's functions (the callers' names, and the golden tests'): each calls TNetwork's member of the same
// name on bridge::Network().
void SetTestServerPort(int Port);
void SetConnectionTimeout(int Timeout);
void SetNetworkWindow(patella::platform::TWindowHandle Window);
void SetTestServerAddress(const char *Address);
void SetServerError(int errorcode);
void SetServerIPAddress(const char *ipaddress);

int GetNextCommand();

void CheckConnectionTimeout();
// Message is the caller's new char[ERROR_MESSAGE_SIZE]: a fixed extent, passed as the pointer was (issue #69).
void GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE> MessageBuffer, int ErrorCode, int ErrorType);
static_assert(patella::IsSpanLayout<char, ERROR_MESSAGE_SIZE>, "a pointer's size: the golden tests' fakes take char *");
const char *GetResolvedIPAddress();
void Disconnect(bool force);
void ConnectToLoginServer();
void ConnectToGameServer();
void ShutdownConnection();
void InitNetwork(void (*ConnectHandler)(), void (*ReceiveHandler)(), void (*CloseHandler)());
void ExitNetwork();

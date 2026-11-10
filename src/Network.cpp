// Network.cpp: the login server list, connecting, and the packets' framing and XTEA encryption.
//
// Three functions catch errors, the encryption's (SendCommand, PrepareNextPacket, SetSymmetricKey); any other exception
// passes to the caller.
//
// Wire format, from SendCommand and PrepareNextPacket:
//   unencrypted: [u16 length][payload]
//   encrypted:   [u16 total][XTEA([u16 length][payload][random padding to 8])]

#include "Network.h"
#include "BuildConfig.h"
#include "Utils.h"
#include "TXTEA.h"
#include "Preferences.h"
#include "common.h"
#include "log/Log.h"
#include "compat/span.h"
#include "platform/Socket.h"
#include "platform/Window.h"
#include "types/FixedString.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// The data is TNetwork's (Network.h). The socket is the platform's (ISocket, platform/Socket.h), and its events come to
// TNetwork's OnConnect, OnReceive and OnClose.

patella::TNetwork::TNetwork(TPreferences &PreferencesRef, TNetworkSession &SessionRef, platform::ISocket &SocketsRef)
	: Preferences(PreferencesRef), Session(SessionRef), Sockets(SocketsRef)
{
}

// BuildConfig.h's values fit the fields they go into.
static_assert(patella::buildconfig::LoginServers.size() == NUMBEROFSERVER, "NUMBEROFSERVER and LoginServers");
static_assert(sizeof(TServer::Address) == patella::buildconfig::MaxHostName + 1, "TServer::Address and MaxHostName");

void patella::TNetwork::InitServerList()
{
	memset(Servers.data(), 0, sizeof(Servers));
	constexpr const char *ServerName0 = patella::buildconfig::LoginServers[0];
	constexpr const char *ServerName1 = patella::buildconfig::LoginServers[1];
	constexpr const char *ServerName2 = patella::buildconfig::LoginServers[2];
	constexpr const char *ServerName3 = patella::buildconfig::LoginServers[3];
	Servers[0].Address.Assign(ServerName0);
	Servers[1].Address.Assign(ServerName1);
	Servers[2].Address.Assign(ServerName2);
	Servers[3].Address.Assign(ServerName3);
	TestServer.ErrorCode = 0;
	TestServer.Resolved = false;
	TestServer.Available = true;
	for (int i = 0; i < NUMBEROFSERVER; i++) {
		Servers[i].Port = patella::buildconfig::LoginPort;
		Servers[i].Available = true;
	}
	CurrentServer = rand() % NUMBEROFSERVER;
	ApplyServerOverride();
}

// Patella-Plus, not in the original: BuildConfig.h's overrides, else Patella.ini's ServerAddress and ServerPort, replace
// the address and the port of every login server and of the test server. The four entries repeat the one server, as a
// build with fewer than four does. An IP address goes through the name lookup as a name does: getaddrinfo returns it.
void patella::TNetwork::ApplyServerOverride()
{
	const settings::TServerValues &Values = Preferences.GetServerValues();
	const char *Address = Values.Address.c_str();
	int Port = Values.Port;
	if constexpr (patella::buildconfig::ServerAddressOverride[0] != '\0')
		Address = patella::buildconfig::ServerAddressOverride;
	if constexpr (patella::buildconfig::ServerPortOverride != 0)
		Port = patella::buildconfig::ServerPortOverride;
	if (Address[0] != '\0') {
		for (TServer &Server : Servers)
			Server.Address.Assign(Address);
		TestServer.Address.Assign(Address);
		patella::log::Info("login server address: {}", Address);
	}
	if (Port != 0) {
		for (TServer &Server : Servers)
			Server.Port = Port;
		TestServer.Port = Port;
		patella::log::Info("login server port: {}", Port);
	}
}

void patella::TNetwork::SetTestServerPort(int Port)
{
	TestServer.Port = Port;
}

void patella::TNetwork::ResetServerAvailability()
{
	for (int i = 0; i < NUMBEROFSERVER; i++)
		Servers[i].Available = true;
	TestServer.Available = true;
}

void patella::TNetwork::SetConnectionTimeout(int Timeout)
{
	TimeoutCounter = 0;
	TimeoutLimit = 0;
	if (Timeout > 0)
		TimeoutLimit = Timeout;
}

void patella::TNetwork::SetNetworkWindow(platform::TWindowHandle Window)
{
	NetworkWindow = Window;
}

void patella::TNetwork::SetTestServerAddress(const char *Address)
{
	PATELLA_ASSERT(Address!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Address)<100, strlen(Address));
	TestServer.Address.Assign(Address);
}

void patella::TNetwork::SetServerError(int errorcode)
{
	PATELLA_ASSERT_VALUES(CurrentServer<NUMBEROFSERVER, CurrentServer);
	if (Preferences.GetTestServerLogin()) {
		TestServer.Available = false;
		TestServer.ErrorCode = errorcode;
		return;
	}
	Servers[CurrentServer].Available = false;
	Servers[CurrentServer].ErrorCode = errorcode;
}

void patella::TNetwork::SetServerIPAddress(const char *ipaddress)
{
	PATELLA_ASSERT_VALUES(CurrentServer<NUMBEROFSERVER, CurrentServer);
	PATELLA_ASSERT(ipaddress!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(ipaddress)<100, strlen(ipaddress));
	if (Preferences.GetTestServerLogin()) {
		TestServer.Address.Assign(ipaddress);
		TestServer.Resolved = true;
		return;
	}
	Servers[CurrentServer].Address.Assign(ipaddress);
	Servers[CurrentServer].Resolved = true;
}

// Stores the server to try next; false when none is left.
bool patella::TNetwork::GetCurrentServer(TServer **Server)
{
	PATELLA_ASSERT_VALUES(CurrentServer<NUMBEROFSERVER, CurrentServer);
	if (Preferences.GetTestServerLogin()) {
		*Server = &TestServer;
		const bool Available = TestServer.Available;
		for (int i = 0; i < NUMBEROFSERVER; i++)
			Servers[i].Available = true;
		TestServer.Available = true;
		return Available;
	}
	*Server = &Servers[CurrentServer];
	for (int i = 0; i < NUMBEROFSERVER; i++) {
		if (Servers[CurrentServer].Available) {
			*Server = &Servers[CurrentServer];
			return true;
		}
		CurrentServer++;
		if (CurrentServer >= NUMBEROFSERVER)
			CurrentServer = 0;
	}
	return false;
}

int patella::TNetwork::GetServerErrorCode()
{
	if (Preferences.GetTestServerLogin())
		return TestServer.ErrorCode;
	for (int i = 0; i < NUMBEROFSERVER; i++) {
		if (Servers[i].ErrorCode == ERROR_LOGINSERVER_OVERLOADED)
			return ERROR_LOGINSERVER_OVERLOADED;
	}
	TServer *Server;
	GetCurrentServer(&Server);
	return Server->ErrorCode;
}

// Reserves the 4 header bytes that SendCommand fills in.
void patella::TNetwork::BeginCommand(int CommandType)
{
	PATELLA_ASSERT_VALUES(In(CommandType,0,255), CommandType);
	SendBuffer.Position = 0;
	SendBuffer.PutWord(0);
	SendBuffer.PutWord(0);
	SendBuffer.PutByte((uint8)CommandType);
}

void patella::TNetwork::SendByte(uint8 Data)
{
	SendBuffer.PutByte(Data);
}

void patella::TNetwork::SendWord(uint16 Data)
{
	SendBuffer.PutWord(Data);
}

void patella::TNetwork::SendQuad(uint32 Data)
{
	SendBuffer.PutQuad(Data);
}

void patella::TNetwork::SendString(const char *Text)
{
	PATELLA_ASSERT(Text!=nullptr);
	SendBuffer.PutString(Text);
}

// Buffer is the span's pointer, the name of the assertion's text.
void patella::TNetwork::SendBytes(patella::TSpan<const uint8> Bytes)
{
	const uint8 *const Buffer = Bytes.data();
	PATELLA_ASSERT(Buffer!=nullptr);
	SendBuffer.PutBytes(Bytes);
}

void patella::TNetwork::SendCommand(bool Encrypt)
{
	if (Socket == nullptr)
		return;
	const int Size = SendBuffer.Position - 4;
	if (!Encrypt) {
		SendBuffer.Position = 0;
		SendBuffer.PutWord(0);
		SendBuffer.PutWord((uint16)Size);
		Sockets.Send(Socket, &SendData[2], Size + 2);
		return;
	}
	int Length = Size + 2;
	while (Length % 8 != 0) {
		SendBuffer.PutByte((uint8)(rand() % 256));
		Length++;
	}
	SendBuffer.Position = 0;
	SendBuffer.PutWord((uint16)Length);
	SendBuffer.PutWord((uint16)Size);
	try {
		for (int i = 2; i < Length + 2; i += 8)
			SymmetricKey.EncryptBlock(TXTEA::TBlock(&SendData[i], 8));
	} catch (...) {
		TERROR("Symmetric encryption failed with unknown exception");
	}
	Sockets.Send(Socket, SendData.data(), Length + 2);
}

// Drops the packet already read, then decrypts the next complete packet in place
// and points ReceiveBuffer at its payload.
//
// Deliberate differences from the original: a frame the client cannot use is dropped (DropFrame), a warn line goes to
// the developer log, and the next complete frame is prepared in its place, so the connection goes on. Two such frames:
// - A size that is no multiple of 8 (XTEA's block). Tibia.exe threw "packet size does not fit to symmetric
//   encryption" before it dropped the frame, so every later call threw the same; the frame is not decrypted.
// - The payload, [u16 length][length bytes], must lie inside the frame, its PacketSize bytes after the size (a length
//   that runs into the XTEA padding is still inside it and read as before). A frame of size 0 (no length in it) or
//   one whose length runs past it is dropped. Tibia.exe read the length past an empty frame and set the read buffer's
//   size from it as it was, so its reads went on past the frame: into the next frames, and past ReceiveData into
//   SendData.
// - A packet of inner length 0 (nothing in it) is not a bad frame and is not dropped or logged, but nothing of it is
//   left to read, so the next frame is prepared in its place too, in the same call. Tibia.exe returned it as the
//   prepared packet: GetNextCommand returned -1 with complete frames behind it, and the empty packet's frame stayed in
//   ReceiveData until the next PrepareNextPacket, so a following frame that fits only without it never completed and
//   the connection stalled.
void patella::TNetwork::PrepareNextPacket()
{
	while (true) {
		if (ReceiveRead > 0) {
			if (ReceiveRead < ReceiveDataSize)
				memmove(ReceiveData.data(), &ReceiveData[ReceiveRead], ReceiveDataSize - ReceiveRead);
			ReceiveDataSize -= ReceiveRead;
			ReceivePacketEnd -= ReceiveRead;
			ReceiveRead = 0;
		}
		if (ReceivePacketEnd <= 0)
			return;
		ReceiveBuffer.Size = 4;
		ReceiveBuffer.Position = 0;
		const int PacketSize = ReceiveBuffer.GetWord();
		const int FrameEnd = PacketSize + 2;
		if (PacketSize % 8 != 0) {
			patella::log::Warn("server frame dropped: its size {} is no multiple of 8", PacketSize);
			DropFrame(FrameEnd);
			continue;
		}
		try {
			for (int i = 2; i < PacketSize + 2; i += 8)
				SymmetricKey.DecryptBlock(TXTEA::TBlock(&ReceiveData[i], 8));
		} catch (...) {
			TERROR("Symmetric decryption failed with unknown exception");
		}
		const int Size = PacketSize >= 2 ? ReceiveBuffer.GetWord() + 4 : 0;
		memset(PacketHeader.data(), 0, sizeof(PacketHeader));
		ReceiveRead = FrameEnd;
		memcpy(PacketHeader.data(), &ReceiveData[2], PacketSize < 16 ? PacketSize : 16);
		if (PacketSize < 2 || Size > FrameEnd) {
			if (PacketSize < 2)
				patella::log::Warn("server frame dropped: it has no length (size {})", PacketSize);
			else
				patella::log::Warn("server frame dropped: its length {} runs past its size {}", Size - 4, PacketSize);
			DropFrame(FrameEnd);
			continue;
		}
		// An empty packet (inner length 0) has no command; the next frame is prepared in its place.
		if (Size == 4)
			continue;
		ReceiveBuffer.SetSize(Size);
		return;
	}
}

// Not the original's: the frame up to FrameEnd is dropped (the next PrepareNextPacket moves the data after it down) and
// the read buffer is empty. The caller has written the log line.
void patella::TNetwork::DropFrame(int FrameEnd)
{
	ReceiveRead = FrameEnd;
	ReceiveBuffer.SetSize(ReceiveBuffer.Position);
}

// Returns the next command byte, or -1 when no packet is complete.
int patella::TNetwork::GetNextCommand()
{
	if (ReceiveBuffer.Position == ReceiveBuffer.Size) {
		PrepareNextPacket();
		if (ReceiveBuffer.Position == ReceiveBuffer.Size)
			return -1;
	}
	return ReceiveBuffer.GetByte();
}

uint8 patella::TNetwork::GetByte()
{
	return ReceiveBuffer.GetByte();
}

uint16 patella::TNetwork::GetWord()
{
	return ReceiveBuffer.GetWord();
}

uint32 patella::TNetwork::GetQuad()
{
	return ReceiveBuffer.GetQuad();
}

// Buffer and MaxLength are the span's pointer and size, the names of the macros' text (TReadBuffer::GetString).
void patella::TNetwork::GetString(patella::TSpan<char> Text)
{
	const char *const Buffer = Text.data();
	[[maybe_unused]] const int MaxLength = static_cast<int>(Text.size());
	PATELLA_ASSERT(Buffer!=nullptr);
	ReceiveBuffer.GetString(Text);
}

// The caller owns the returned string.
char *patella::TNetwork::GetString()
{
	return ReceiveBuffer.GetString();
}

// Called periodically; reconnects when the limit is passed.
void patella::TNetwork::CheckConnectionTimeout()
{
	if (TimeoutLimit != 0 && ++TimeoutCounter > TimeoutLimit) {
		TimeoutLimit = 0;
		TimeoutCounter = 0;
		SetServerError(platform::socket_error::TimedOut);
		Disconnect(false);
		Session.SetConnectionState(CONNECTION_OFFLINE);
		ConnectToLoginServer();
	}
}

// ErrorType 1 is the login server, 2 the game server.
void patella::TNetwork::GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE> MessageBuffer, int ErrorCode, int ErrorType)
{
	char *const Message = MessageBuffer.data();
	PATELLA_ASSERT(Message!=nullptr);
	TFixedString<100> Text;
	bool Offline = false;
	Message[0] = 0;
	// Every caller passes new char[1024] (ERROR_MESSAGE_SIZE); the longest message has about 300 characters.
	TStringBuffer Out(Message, MessageBuffer.size());
	switch (ErrorCode) {
	case platform::socket_error::Invalid:
	case platform::socket_error::AddressNotAvailable:
		Text.Assign("Invalid server address.");
		break;
	case platform::socket_error::MessageSize:
		Text.Assign("Connection error.");
		break;
	case platform::socket_error::NetworkDown:
	case platform::socket_error::NetworkUnreachable:
		Text.Assign("Network is unreachable.");
		break;
	case platform::socket_error::TimedOut:
		Text.Assign("Connection timeout.");
		break;
	case platform::socket_error::ConnectionRefused:
		Text.Assign("Connection refused.");
		Offline = true;
		break;
	case platform::socket_error::HostUnreachable:
		Text.Assign("Server is unreachable.");
		break;
	case platform::socket_error::HostNotFound:
		Text.Assign("Server name does not exist.");
		break;
	case platform::socket_error::TryAgain:
		Text.Assign("Could not resolve server name.");
		break;
	case platform::socket_error::NoData:
		Text.Assign("Error while resolving the server name.");
		break;
	case ERROR_LOGINSERVER_OVERLOADED:
		Text.Assign("Login server is overloaded. Please try again later.");
		break;
	case ERROR_DISCONNECTED:
		Text.Assign("Disconnected from server.");
		break;
	default:
		Text.Format("{}", ErrorCode);
		break;
	}

	if (Preferences.GetTestServerLogin()) {
		Out.Append("Cannot connect to the test server.\n\nError: ");
		Out.Append(Text);
		if (Offline) {
			Out.Append("\n\nThe test server is offline. Check ");
			Out.Append(patella::buildconfig::SiteName);
			Out.Append("\nfor more information on any current test.");
		} else {
			Out.Append("\n\nCheck ");
			Out.Append(patella::buildconfig::SiteName);
			Out.Append(" for more information on ");
			Out.Append("\nany current test.");
		}
		Out.Append("\nIf you don't want to use the test server go to");
		Out.Append("\nthe network option and uncheck \"Test Server Login\".");
	} else if (ErrorType == 1) {
		Out.Append("Cannot connect to a login server.\n\nError: ");
		Out.Append(Text);
		if (Offline) {
			Out.Append("\n\nAll login server are offline. Check ");
			Out.Append(patella::buildconfig::SiteName);
			Out.Append("\nfor more information.");
		}
	} else {
		if (ErrorType != 2)
			TERROR1("unknown error type", ErrorType);
		Out.Append("Cannot connect to the game server.\n\nError: ");
		Out.Append(Text);
		if (Offline) {
			Out.Append("\n\nThe game server is offline. Check ");
			Out.Append(patella::buildconfig::SiteName);
			Out.Append("\nfor more information.");
		}
	}
	Out.Append("\n\nFor more information take a look at the FAQs in the ");
	Out.Append("\nguide section at ");
	Out.Append(patella::buildconfig::SiteName);
	Out.Append(".");
}

// Starts an asynchronous lookup; the platform reports its end to SdlFrame.cpp's HostResolved.
bool patella::TNetwork::ResolveHostname(const char *Hostname)
{
	PATELLA_ASSERT(Hostname!=nullptr);
	HostnameRequest = Sockets.ResolveHostname(NetworkWindow, HOSTNAME_RESOLVED_MESSAGE, Hostname, HostnameResolveBuffer,
		HostnameResolveBufferSize);
	if (HostnameRequest == nullptr) {
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), Sockets.LastError(), 1);
		Session.AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
		Session.SetConnectionState(CONNECTION_OFFLINE);
		return false;
	}
	TimeoutCounter = 0;
	TimeoutLimit = 150;
	return true;
}

const char *patella::TNetwork::GetResolvedIPAddress() const
{
	PATELLA_ASSERT(HostnameResolveBuffer!=nullptr);
	return Sockets.ResolvedAddress(HostnameResolveBuffer);
}

// Returns 0 or a WinSock error code.
int patella::TNetwork::CreateSocket()
{
	if (Socket != nullptr)
		Disconnect(false);
	Socket = Sockets.New(*this);
	if (!Sockets.Create(Socket)) {
		const int Error = Sockets.LastError();
		Sockets.Delete(Socket);
		Socket = nullptr;
		Session.SetConnectionState(CONNECTION_OFFLINE);
		return Error;
	}
	return 0;
}

// Returns 0 or a WinSock error code.
int patella::TNetwork::ConnectSocket(const char *IPAddress, int Port, bool SetTimeout)
{
	PATELLA_ASSERT(IPAddress!=nullptr);
	if (!Sockets.Connect(Socket, IPAddress, Port)) {
		if (Sockets.LastError() != platform::socket_error::WouldBlock) {
			const int Error = Sockets.LastError();
			Sockets.Delete(Socket);
			Socket = nullptr;
			Session.SetConnectionState(CONNECTION_OFFLINE);
			return Error;
		}
	}
	ReceiveRead = 0;
	ReceivePacketEnd = 0;
	ReceiveDataSize = 0;
	ReceiveSkip = 0;
	if (SetTimeout) {
		TimeoutLimit = 150;
		TimeoutCounter = 0;
	}
	return 0;
}

void patella::TNetwork::SetSymmetricKey(const uint8 *Key)
{
	try {
		SymmetricKey.SetKey(TXTEA::TKey(Key, 16));
	} catch (const char *) {
		TERROR("Key initialization failed");
	}
}

void patella::TNetwork::Disconnect(bool force)
{
	if (force || Session.GetConnectionState() == CONNECTION_RESOLVING_LOGIN_SERVER) {
		Sockets.CancelResolve(HostnameRequest);
		HostnameRequest = nullptr;
	}
	if (Socket != nullptr) {
		Sockets.Shutdown(Socket, platform::TShutdown::Send);
		TFixedString<256> Buffer;
		int Count;
		do {
			Count = Sockets.Receive(Socket, Buffer, sizeof(Buffer));
			if (Count == 0)
				break;
		} while (Count != platform::SocketFailed);
		Sockets.Shutdown(Socket, platform::TShutdown::Both);
		Sockets.Close(Socket);
		Sockets.Delete(Socket);
		Socket = nullptr;
		TimeoutLimit = 0;
		TimeoutCounter = 0;
	}
}

// Tries the current login server and moves on to the next one on failure;
// recursion is bounded by the number of servers.
void patella::TNetwork::ConnectToLoginServer()
{
	RecursionProtection++;
	PATELLA_ASSERT_VALUES(RecursionProtection<=NUMBEROFSERVER+1, RecursionProtection);
	TServer *Server;
	if (!GetCurrentServer(&Server)) {
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), GetServerErrorCode(), 1);
		Session.AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
		for (int i = 0; i < NUMBEROFSERVER; i++)
			Servers[i].Available = true;
		TestServer.Available = true;
	} else if (!Server->Resolved) {
		Session.SetConnectionState(CONNECTION_RESOLVING_LOGIN_SERVER);
		ResolveHostname(Server->Address);
	} else {
		Session.SetConnectionState(CONNECTION_CONNECTING_LOGIN_SERVER);
		int Error = CreateSocket();
		if (Error == 0) {
			Error = ConnectSocket(Server->Address, Server->Port, true);
			if (Error != 0) {
				SetServerError(Error);
				ConnectToLoginServer();
			}
		} else {
			ResetServerAvailability();
		}
	}
	RecursionProtection--;
}

void patella::TNetwork::ConnectToGameServer()
{
	Session.SetConnectionState(CONNECTION_CONNECTING_GAME_SERVER);
	const int Character = Session.GetActiveCharacter();
	const char *IPAddress = Session.GetCharacterIPAddress(Character);
	Session.GetCharacterPort(Character); // result unused, as in the original
	int Error = CreateSocket();
	if (Error != 0) {
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), Error, 2);
		Session.AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
		Session.AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
	}
	Error = ConnectSocket(IPAddress, Session.GetCharacterPort(Character), false);
	if (Error != 0) {
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), Error, 2);
		Session.AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
		Session.AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
	}
}

void patella::TNetwork::OnConnect(int ErrorCode)
{
	TimeoutLimit = 0;
	TimeoutCounter = 0;
	if (ErrorCode == 0) {
		ConnectCallback();
		for (int i = 0; i < NUMBEROFSERVER; i++)
			Servers[i].Available = true;
		TestServer.Available = true;
		return;
	}
	if (Session.GetConnectionState() == CONNECTION_CONNECTING_LOGIN_SERVER) {
		// A login server refused: try the next one.
		TimeoutLimit = 0;
		TimeoutCounter = 0;
		Session.SetConnectionState(CONNECTION_OFFLINE);
		SetServerError(ErrorCode);
		ConnectToLoginServer();
		return;
	}
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader
	char *Message = new char[1024];
	GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), ErrorCode, 2);
	Session.AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
	Session.AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
	Disconnect(false);
	Session.SetConnectionState(CONNECTION_OFFLINE);
}

// Appends received bytes and calls the receive handler
// once at least one complete packet is buffered.
void patella::TNetwork::OnReceive(platform::TSocket &Receiver, int ErrorCode)
{
	if (ErrorCode != 0)
		return;
	// The free part's start as a pointer: with ReceiveData full (frames the handler has not read yet) it is the array's
	// end, which the hardened library's operator[] refuses (Receive then gets no room).
	const int Count =
		Sockets.Receive(&Receiver, ReceiveData.data() + ReceiveDataSize, sizeof(ReceiveData) - ReceiveDataSize);
	if (Count == 0 || Count == platform::SocketFailed)
		return;
	// A deliberate difference from the original: a frame larger than ReceiveData can never be complete there. Tibia.exe
	// kept its first bytes and waited: ReceiveData filled up, Receive got no room, and no later frame came through,
	// forever. Now its bytes are dropped as they come (ReceiveSkip: those still to come), a warn line goes to the
	// developer log, and the frames after it are read.
	int Kept = Count;
	if (ReceiveSkip > 0) {
		const int Skipped = Count < ReceiveSkip ? Count : ReceiveSkip;
		ReceiveSkip -= Skipped;
		Kept = Count - Skipped;
		if (Kept > 0)
			memmove(&ReceiveData[ReceiveDataSize], &ReceiveData[ReceiveDataSize + Skipped], Kept);
	}
	ReceiveDataSize += Kept;
	bool Complete = false;
	while (ReceivePacketEnd + 2 <= ReceiveDataSize) {
		TReadBuffer Header(patella::TSpan<uint8>(&ReceiveData[ReceivePacketEnd], 2));
		const int Length = Header.GetWord() + 2;
		if (Length > static_cast<int>(ReceiveData.size())) {
			patella::log::Warn("server frame dropped: its size {} does not fit the receive buffer of {} bytes", Length - 2,
				ReceiveData.size());
			ReceiveSkip = Length - (ReceiveDataSize - ReceivePacketEnd);
			ReceiveDataSize = ReceivePacketEnd;
			break;
		}
		if (ReceiveDataSize - ReceivePacketEnd < Length)
			break;
		Complete = true;
		ReceivePacketEnd += Length;
	}
	if (Complete)
		ReceiveCallback();
}

void patella::TNetwork::OnClose()
{
	Disconnect(false);
	CloseCallback();
}

void patella::TNetwork::ShutdownConnection() const
{
	if (Socket != nullptr)
		Sockets.Shutdown(Socket, platform::TShutdown::Both);
}

void patella::TNetwork::InitNetwork(void (*ConnectHandler)(), void (*ReceiveHandler)(), void (*CloseHandler)())
{
	if (!Sockets.Startup())
		throw TProgramError{PROGRAM_ERROR::SOCKET_INIT};
	PATELLA_ASSERT(ConnectHandler!=nullptr);
	PATELLA_ASSERT(ReceiveHandler!=nullptr);
	PATELLA_ASSERT(CloseHandler!=nullptr);
	ReceiveCallback = ReceiveHandler;
	ConnectCallback = ConnectHandler;
	CloseCallback = CloseHandler;
	ReceiveRead = 0;
	ReceivePacketEnd = 0;
	ReceiveDataSize = 0;
	ReceiveSkip = 0;
	Socket = nullptr;
	HostnameResolveBuffer = nullptr;
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): the original's malloc, freed in ExitNetwork
	HostnameResolveBuffer = static_cast<char *>(malloc(HostnameResolveBufferSize));
	InitServerList();
}

void patella::TNetwork::ExitNetwork()
{
	Sockets.Delete(Socket);
	Socket = nullptr;
	if (HostnameResolveBuffer != nullptr)
		// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): InitNetwork's malloc
		free(HostnameResolveBuffer);
	HostnameResolveBuffer = nullptr;
}

// The original's functions: each calls its member on the one TClient's TNetwork.

void SetTestServerPort(int Port)
{
	Network().SetTestServerPort(Port);
}

void SetConnectionTimeout(int Timeout)
{
	Network().SetConnectionTimeout(Timeout);
}

void SetNetworkWindow(patella::platform::TWindowHandle Window)
{
	Network().SetNetworkWindow(Window);
}

void SetTestServerAddress(const char *Address)
{
	Network().SetTestServerAddress(Address);
}

void SetServerError(int errorcode)
{
	Network().SetServerError(errorcode);
}

void SetServerIPAddress(const char *ipaddress)
{
	Network().SetServerIPAddress(ipaddress);
}

int GetNextCommand()
{
	return Network().GetNextCommand();
}

void CheckConnectionTimeout()
{
	Network().CheckConnectionTimeout();
}

void GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE> MessageBuffer, int ErrorCode, int ErrorType)
{
	Network().GetErrorMessage(MessageBuffer, ErrorCode, ErrorType);
}

const char *GetResolvedIPAddress()
{
	return Network().GetResolvedIPAddress();
}

void Disconnect(bool force)
{
	Network().Disconnect(force);
}

void ConnectToLoginServer()
{
	Network().ConnectToLoginServer();
}

void ConnectToGameServer()
{
	Network().ConnectToGameServer();
}

void ShutdownConnection()
{
	Network().ShutdownConnection();
}

void InitNetwork(void (*ConnectHandler)(), void (*ReceiveHandler)(), void (*CloseHandler)())
{
	Network().InitNetwork(ConnectHandler, ReceiveHandler, CloseHandler);
}

void ExitNetwork()
{
	Network().ExitNetwork();
}

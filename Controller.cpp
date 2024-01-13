#include "Networking.h"

void Controller(const udp::endpoint& thrallIp, const int localIp) {

	Connection conn(localIp);

	conn.socket.connect(thrallIp, conn.err_code);
	conn.connected = conn.err_code.value() == 0;

	if (!conn.connected) { return; }

	CaptureInput(conn);
	conn.context.run();

}

int TimeUntilNextPoll() {

	// Return how much time has passed since last time the input was polled
	static steady_clock::time_point timeSinceLastPoll = timer.now();
	
	const int targetPollRate = 1000 / pollRateHz;
	const int deltaMillis = duration_cast<milliseconds>(timer.now() - timeSinceLastPoll).count();

	const int timeRemainingMillis = targetPollRate - deltaMillis;
	
	if (timeRemainingMillis > 0) {
		return timeRemainingMillis;
	}

	timeSinceLastPoll = timer.now();

	return 0;
}

void CaptureInput(Connection& conn) {

	// Only get input if enough time has passed
	int millis = TimeUntilNextPoll();
	Sleep(millis);

	// Check if quit button is pressed
	if (GetKeyState(QUIT_KEY) & 0x800) {
		conn.Disconnect();
		return;
	}

#ifdef DEBUG
	Sleep(3000);
#endif

	vector<Data>  encodedButtons  = EncodeButtonStates();
	MousePosArray encodedMousePos = EncodeMouseDelta();

	copy(encodedMousePos.begin(), encodedMousePos.end(), conn.buf.begin());
	copy(encodedButtons.begin(), encodedButtons.end(), conn.buf.begin() + encodedMousePos.size());
	Send(conn);

}

MousePosArray EncodeMouseDelta() {

	MousePosArray encodedMouseDelta = {};

	POINT cursorPosition;
	GetCursorPos(&cursorPosition);
	
	static POINT previousCursorPos = cursorPosition;
	
	Data deltaX[sizeof(int)];
	Data deltaY[sizeof(int)];
	
	EncodeByte(deltaX, cursorPosition.x - previousCursorPos.x);
	EncodeByte(deltaY, cursorPosition.y - previousCursorPos.y);
	
	for (int i = 0; i < sizeof(int); i++) {
		encodedMouseDelta[i]               = deltaX[i];
		encodedMouseDelta[i + sizeof(int)] = deltaY[i];
	}

	previousCursorPos = cursorPosition;

	return encodedMouseDelta;

}

void WriteHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); }

	if (conn.connected) { CaptureInput(conn); }

}

void Send(Connection& conn) {
	
	auto onWrite = bind(WriteHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	size_t bytes_to_transfer = min(sizeof(int) * 2 + Buttoncode_Name_Map.size() * 2, conn.buf.size());

	conn.socket.async_send(buffer(conn.buf, bytes_to_transfer), onWrite);
}

vector<Data> EncodeButtonStates() {

	// Protocol
	//
	// | BYTE 1  | BYTE 2 |
	// | KEYCODE | STATE  |

	vector<Data> encodedButtons;
	for (const auto& [buttoncode, buttonName] : Buttoncode_Name_Map) {

		bool buttonstate = GetKeyState(buttoncode) & 0x800;

		encodedButtons.push_back(buttoncode);
		encodedButtons.push_back(buttonstate);

		debugLog << buttonName << ":\t" << buttonstate << "\n";
	}

	debugLog << "\n";

	return encodedButtons;

}
#include "Networking.h"

void Controller(const udp::endpoint& thrallIp, const int localIp) {

	Connection conn(localIp);

	conn.socket.connect(thrallIp, conn.err_code);
	conn.connected = conn.err_code.value() == 0;

	while (conn.connected) {

		Poll(conn);
		conn.context.run();
		conn.context.restart();
	}

}

bool ShouldPoll() {
	// See how much time has passed since last time the input was polled
	static steady_clock::time_point timeSinceLastPoll = timer.now();
	int deltaMilli = duration_cast<milliseconds>(timer.now() - timeSinceLastPoll).count();

	if (deltaMilli < (1000 / pollRateHz)) {
		return false;
	}

	timeSinceLastPoll = timer.now();

	return true;
}

void Poll(Connection& conn) {

	// Only poll if enough time has passed
	if (!ShouldPoll()) { return; }

	Sleep(0); // DEBUG: Give sometime before polling 

	vector<MouseData>    encodedMouse  = EncodeMouse();
	vector<KeyboardData> encodedKeys   = EncodeKeys();

	memcpy(conn.buf.data(), encodedMouse.data(), encodedMouse.size());
	memcpy(&conn.buf.data()[encodedMouse.size()], encodedKeys.data(), encodedKeys.size());

	// Disconnect
	if (GetKeyState(VK_ESCAPE) & 0x800) {
		conn.Disconnect();
		return;
	}

	Send(conn);

	return;

}

vector<MouseData> EncodeMouse() {

	vector<MouseData> mouseData;

	CURSORINFO cInfo;
	ZeroMemory(&cInfo, sizeof(cInfo));
	cInfo.cbSize = sizeof(cInfo);
	GetCursorInfo(&cInfo);
	
	MouseData positionX[sizeof(int)];
	MouseData positionY[sizeof(int)];
	
	EncodeByte(positionX, cInfo.ptScreenPos.x);
	EncodeByte(positionY, cInfo.ptScreenPos.y);
	
	for (int i = 0; i < sizeof(int); i++) {
		mouseData.push_back(positionX[i]);
		mouseData.push_back(positionY[i]);
	}

	return mouseData;

}

void WriteHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); }

}

void Send(Connection& conn) {
	
	auto onWrite = bind(WriteHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	conn.socket.async_send(boost::asio::buffer(conn.buf.data(), conn.buf.size()), onWrite);
}

vector<KeyboardData> EncodeKeys() {

	// Protocol
	//
	// | BYTE 1  | BYTE 2 |
	// | KEYCODE | STATE  |

	vector<KeyboardData> encodedKeys;
	for (const auto& keyCodeName : Keycode_Name_Map) {

		KeyboardKey keycode		= keyCodeName.first;
		bool		keystate	= true;//GetKeyState(keycode) & 0x800;

		encodedKeys.push_back(keycode);
		encodedKeys.push_back(keystate);
	}

	return encodedKeys;

}
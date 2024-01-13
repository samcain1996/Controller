#include "Networking.h"

void Controller(const udp::endpoint& thrallIp, const int localIp) {

	Connection conn(localIp);

	conn.socket.connect(thrallIp, conn.err_code);
	conn.connected = conn.err_code.value() == 0;

	Poll(conn);
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

void Poll(Connection& conn) {

	// Only poll if enough time has passed
	int millis = TimeUntilNextPoll();
	Sleep(millis);

	// Check if quit button is pressed
	if (GetKeyState(QUIT_KEY) & 0x800) {
		conn.Disconnect();
		return;
	}

#ifdef LOG
	//Sleep(3000);
#endif

	MousePosArray encodedMousePos         = EncodeMousePosition();
	vector<Data> encodedButtons   = EncodeButtons();

	copy(encodedMousePos.begin(), encodedMousePos.end(), conn.buf.begin());
	copy(encodedButtons.begin(), encodedButtons.end(), conn.buf.begin() + encodedMousePos.size());
	Send(conn);

}

MousePosArray EncodeMousePosition() {

	MousePosArray mousePosition;

	CURSORINFO cInfo;
	ZeroMemory(&cInfo, sizeof(cInfo));
	cInfo.cbSize = sizeof(cInfo);
	GetCursorInfo(&cInfo);

	static int prevX = cInfo.ptScreenPos.x;
	static int prevY = cInfo.ptScreenPos.y;
	
	Data positionX[sizeof(int)];
	Data positionY[sizeof(int)];
	
	EncodeByte(positionX, cInfo.ptScreenPos.x - prevX);
	EncodeByte(positionY, cInfo.ptScreenPos.y - prevY);
	
	for (int i = 0; i < sizeof(int); i++) {
		mousePosition[i]               = positionX[i];
		mousePosition[i + sizeof(int)] = positionY[i];
	}

	prevX = cInfo.ptScreenPos.x;
	prevY = cInfo.ptScreenPos.y;

	return mousePosition;

}

void WriteHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); }

	if (conn.connected) { Poll(conn); }

}

void Send(Connection& conn) {
	
	auto onWrite = bind(WriteHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	size_t bytes_to_transfer = min(sizeof(int) * 2 + Buttoncode_Name_Map.size() * 2, conn.buf.size());

	conn.socket.async_send(boost::asio::buffer(conn.buf.data(), bytes_to_transfer), onWrite);
}

vector<Data> EncodeButtons() {

	// Protocol
	//
	// | BYTE 1  | BYTE 2 |
	// | KEYCODE | STATE  |

	vector<Data> encodedButtons;
	for (const auto& buttonCodeName : Buttoncode_Name_Map) {

		Data buttoncode  = buttonCodeName.first;
		bool		buttonstate	= GetKeyState(buttoncode) & 0x800;

		encodedButtons.push_back(buttoncode);
		encodedButtons.push_back(buttonstate);
	}

	return encodedButtons;

}
#include "Networking.h"

void Thrall(const int port) {

	Connection conn(port);
	conn.connected = true;

	while (conn.connected) {

		Receive(conn);
		conn.context.run();
		conn.context.restart();
	}

}

void Receive(Connection& conn) {

	auto onReceive = bind(ReceiveHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	conn.socket.async_receive(boost::asio::buffer(conn.buf.data(), conn.buf.size()), onReceive);
}

void ReceiveHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); return; }

	const int mouseX = DecodeByte(conn.buf.data());
	const int mouseY = DecodeByte(&conn.buf.data()[sizeof(mouseX)]);

	KeyboardData* keys        = &conn.buf.data()[sizeof(mouseX) + sizeof(mouseY)];
	KeyStateMap   decodedKeys = DecodeKeys(keys, KEY_BUFFER_SIZE);

	Sleep(0);

	SimulateInput(decodedKeys, mouseX, mouseY);

}

void SimulateInput(const KeyStateMap& keysToInput, const int cursorX, const int cursorY) {

	INPUT input;
	vector<INPUT> inputs;

	for (const auto& [keycode, isDown] : keysToInput) {

		ZeroMemory(&input, sizeof(INPUT));

		if (!Keycode_Name_Map.contains(keycode)) { continue; }

		input.type       = INPUT_KEYBOARD;
		input.ki.wVk     = keycode;
		input.ki.dwFlags = !isDown & KEYEVENTF_KEYUP;

		inputs.push_back(input);

	}

	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));

	SetCursorPos(cursorX, cursorY);

}

KeyStateMap DecodeKeys(const KeyboardData keys[], const int keyCount) {

	// Protocol
	//
	// | BYTE 1  | BYTE 2 |
	// | KEYCODE | STATE  |
	// 
	// DECODE
	// 
	// KEYCODE -> Map Key
	// STATE   -> Map Value

	KeyStateMap decodedKeys;

	for (size_t index = 0; index < KEY_BUFFER_SIZE; index += 2) {
		decodedKeys[keys[index]] = (bool)keys[index + 1];
	}

	return decodedKeys;

}
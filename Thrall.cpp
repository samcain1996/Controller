#include "Networking.h"

void Thrall(const int port) {

	Connection conn(port);
	conn.connected = true;

	Receive(conn);
	conn.context.run();

}

void Receive(Connection& conn) {

	auto onReceive = bind(ReceiveHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	conn.socket.async_receive(boost::asio::buffer(conn.buf), onReceive);
}

void ReceiveHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); return; }

	const int mouseX = DecodeByte(conn.buf.data());
	const int mouseY = DecodeByte(&conn.buf.data()[sizeof(mouseX)]);

	const auto begin = conn.buf.begin() + sizeof(mouseX) + sizeof(mouseY);
	const auto end   = conn.buf.begin() + bytes_transferred;

	vector<Data> buttons(begin, end);
	KeyStateMap decodedKeys = DecodeKeys(buttons);

	SimulateInput(decodedKeys, mouseX, mouseY);

	if (conn.connected) { Receive(conn); }

}

void SimulateInput(const KeyStateMap& buttonsToInput, const int cursorX, const int cursorY) {

	vector<INPUT> inputs;

	for (const auto& [buttoncode, isDown] : buttonsToInput) {

		INPUT input;
		ZeroMemory(&input, sizeof(INPUT));

		if (!Buttoncode_Name_Map.contains(buttoncode)) { continue; }

		switch (buttoncode) {
		case VK_LBUTTON:
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = isDown ? MOUSEEVENTF_LEFTDOWN : NULL;
			break;
		case VK_RBUTTON:
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = isDown ? MOUSEEVENTF_RIGHTDOWN : NULL;
			break;
		default:
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = buttoncode;
			input.ki.dwFlags = isDown ? NULL : KEYEVENTF_KEYUP; 
		};

		inputs.push_back(input);

		debugLog << Buttoncode_Name_Map[buttoncode] << ":\t" << isDown << "\n";

	}

	INPUT cursorInput;
	ZeroMemory(&cursorInput, sizeof(cursorInput));
	cursorInput.type = INPUT_MOUSE;
	cursorInput.mi.dx = cursorX;
	cursorInput.mi.dy = cursorY;
	cursorInput.mi.dwFlags = MOUSEEVENTF_MOVE;

	debugLog << "\n";

	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
	SendInput(1, &cursorInput, sizeof(INPUT));

}

KeyStateMap DecodeKeys(const vector<Data>& buttons) {

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

	for (size_t index = 0; index < buttons.size(); index += 2) {
		decodedKeys[buttons[index]] = (bool)buttons[index + 1];
	}

	return decodedKeys;

}
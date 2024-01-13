#include "Networking.h"

void Thrall(const int port) {

	Connection conn(port);
	conn.connected = true;

	Receive(conn);
	conn.context.run();

}

void Receive(Connection& conn) {

	auto onReceive = bind(ReceiveHandler, ref(conn), std::placeholders::_1, std::placeholders::_2);

	conn.socket.async_receive(buffer(conn.buf), onReceive);
}

void ReceiveHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred) {

	if (!CheckSuccess(conn)) { conn.Disconnect(); return; }

	// Mouse position data is first 8 bytes
	const int mouseX = DecodeByte(conn.buf.data());
	const int mouseY = DecodeByte(&conn.buf.data()[sizeof(mouseX)]);

	const auto begin = conn.buf.begin() + sizeof(mouseX) + sizeof(mouseY);
	const auto end   = conn.buf.begin() + bytes_transferred;

	// Remaining data is(are?) button states
	const vector<Data> buttonStates(begin, end);
	const ButtonStateMap decodedKeys = DecodeButtons(buttonStates);

	SimulateInput(decodedKeys, mouseX, mouseY);

	if (conn.connected) { Receive(conn); }

}

void SimulateInput(const ButtonStateMap& buttonsToInput, const int cursorX, const int cursorY) {

	INPUT input;
	vector<INPUT> inputs;

	for (const auto& [buttoncode, isButtonDown] : buttonsToInput) {

		ZeroMemory(&input, sizeof(INPUT));

		if (!Buttoncode_Name_Map.contains(buttoncode)) { continue; }

		switch (buttoncode) {
		case VK_LBUTTON:  // Left Mouse Button
			input.type			= INPUT_MOUSE;
			input.mi.dwFlags	= isButtonDown ? MOUSEEVENTF_LEFTDOWN : NULL;
			break;
		case VK_RBUTTON:  // Right Mouse Button
			input.type			= INPUT_MOUSE;
			input.mi.dwFlags	= isButtonDown ? MOUSEEVENTF_RIGHTDOWN : NULL;
			break;
		default:		  // Keyboard keys
			input.type			= INPUT_KEYBOARD;
			input.ki.wVk		= buttoncode;
			input.ki.dwFlags	= isButtonDown ? NULL : KEYEVENTF_KEYUP; 
		};

		inputs.push_back(input);

		debugLog << Buttoncode_Name_Map[buttoncode] << ":\t" << isButtonDown << "\n";

	}

	// Input mouse movement
	INPUT cursorInput;
	ZeroMemory(&cursorInput, sizeof(cursorInput));
	cursorInput.type		= INPUT_MOUSE;
	cursorInput.mi.dx		= cursorX;
	cursorInput.mi.dy		= cursorY;
	cursorInput.mi.dwFlags	= MOUSEEVENTF_MOVE;

	debugLog << "\n";

	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
	SendInput(1, &cursorInput, sizeof(INPUT));

}

ButtonStateMap DecodeButtons(const vector<Data>& buttons) {

	// Protocol
	//
	// | BYTE 1  | BYTE 2 |
	// | KEYCODE | STATE  |
	// 
	// DECODE
	// 
	// KEYCODE -> Map Key
	// STATE   -> Map Value

	ButtonStateMap decodedKeys;

	for (size_t index = 0; index < buttons.size(); index += 2) {
		decodedKeys[buttons[index]] = (bool)buttons[index + 1];
	}

	return decodedKeys;

}
#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include "boost/asio.hpp"
#include <Windows.h>
#include <functional>

using namespace std::chrono;
using namespace boost::asio;
using namespace boost::asio::ip;

using std::vector;
using std::array;
using std::size_t;
using std::unordered_map;
using std::string;
using std::bind;
using std::ref;

static const constexpr int KEY_COUNT = 14;
static const constexpr short KEY_BUFFER_SIZE = KEY_COUNT * 2;

using KeyboardKey		= unsigned char;
using KeyboardData		= KeyboardKey;
using MouseData			= KeyboardData;
using KeyStateMap		= unordered_map<KeyboardKey, bool>;
using ConnectionBuffer	= array<KeyboardData, KEY_BUFFER_SIZE + 8>;

static const constexpr KeyboardKey QUIT_KEY = VK_ESCAPE;

static const unordered_map<KeyboardKey, string> Keycode_Name_Map = 
{ 
	{ 'W', "W" }, { 'A', "A" }, { 'S', "S" }, { 'R', "R" },
	{ 'D', "D" }, { 'E', "E" }, { 'F', "F" }, { 'C', "C" },
	{ VK_SPACE, "Space" }, { VK_RETURN, "Enter" }, { VK_TAB, "Tab" }, { VK_SHIFT, "Shift" }, 
	{ VK_CONTROL, "Control" }, { VK_CAPITAL, "Caps Lock" },
};

static const double pollRateHz = 180;
static steady_clock timer;

constexpr void EncodeByte(unsigned char encodedNumber[4], const int numberToEncode) {

	encodedNumber[3] = (unsigned char)(numberToEncode >> 24) & 0xFF;
	encodedNumber[2] = (unsigned char)(numberToEncode >> 16) & 0xFF;
	encodedNumber[1] = (unsigned char)(numberToEncode >> 8) & 0xFF;
	encodedNumber[0] = (unsigned char)(numberToEncode) & 0xFF;

}

constexpr int DecodeByte(const unsigned char encodedNumber[4]) {

	return ((int)encodedNumber[0] + ((int)encodedNumber[1] << 8) +
		((int)encodedNumber[2] << 16) + ((int)encodedNumber[3] << 24));

}

struct Connection {

	bool						connected;
	io_context					context;
	udp::socket					socket;
	ConnectionBuffer			buf;
	boost::system::error_code	err_code;

	Connection(const int port) : connected(false),
		socket(context, udp::endpoint(udp::v4(), port)) {
		buf.fill('\0');
	}

	bool IsDisconnectMessage() const;
};

void Disconnect(Connection& conn);
bool CheckSuccess(const Connection& conn);

void Controller(const udp::endpoint& thrallIp, const int localIp = 15098);
bool ShouldPoll();
void Poll(Connection& conn);
void Send(Connection& conn);

void Thrall(const int port);
void Receive(Connection& conn);
void SimulateInput(const KeyStateMap& keysToInput, const int cursorX, const int cursorY);

void ReceiveHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred);
void WriteHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred);

vector<MouseData>    EncodeMouse();
vector<KeyboardData> EncodeKeys();
KeyStateMap DecodeKeys(const KeyboardData keys[], const int keyCount);
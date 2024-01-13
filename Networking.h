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

#define LOG 1

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
using std::copy;
using std::min;

static const constexpr short BUFFER_SIZE = UCHAR_MAX;

#ifdef LOG 
static std::ostream& debugLog = std::cout;
#else
static std::ostream debugLog;
#endif

using Data				= unsigned char;
using KeyStateMap		= unordered_map<Data, bool>;
using ConnectionBuffer	= array<Data, BUFFER_SIZE>;
using MousePosArray     = array<Data, sizeof(int) * 2>;

static const constexpr Data QUIT_KEY = VK_ESCAPE;

static unordered_map<Data, string> Buttoncode_Name_Map = 
{ 
	{ 'W', "W" }, { 'A', "A" }, { 'S', "S" }, { 'R', "R" },
	{ 'D', "D" }, { 'E', "E" }, { 'F', "F" }, { 'C', "C" },
	{ VK_SPACE, "Space" }, { VK_RETURN, "Enter" }, { VK_TAB, "Tab" }, { VK_SHIFT, "Shift" }, 
	{ VK_CONTROL, "Control" }, { VK_CAPITAL, "Caps Lock" }, { VK_LBUTTON, "Left Click" }, { VK_RBUTTON, "Right Click" }
};

static const double pollRateHz = 10;
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
	void Disconnect();

};

bool CheckSuccess(const Connection& conn);

void Controller(const udp::endpoint& thrallIp, const int localIp = 15098);
int TimeUntilNextPoll();
void Poll(Connection& conn);
void Send(Connection& conn);

void Thrall(const int port);
void Receive(Connection& conn);
void SimulateInput(const KeyStateMap& buttonsToInput, const int cursorX, const int cursorY);

void WriteHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred);
void ReceiveHandler(Connection& conn, const boost::system::error_code err_code, const size_t bytes_transferred);

MousePosArray EncodeMousePosition();
vector<Data> EncodeButtons();
KeyStateMap DecodeKeys(const vector<Data>& buttons);
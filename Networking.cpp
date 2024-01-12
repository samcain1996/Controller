#include "Networking.h"

bool Connection::IsDisconnectMessage() const {
	return buf[0] == QUIT_KEY;
}

void Connection::Disconnect() {

	buf.fill('\0');
	buf[0] = QUIT_KEY;
	socket.send(boost::asio::buffer(buf.data(), buf.size()), 0, err_code);
	connected = false;

}


bool CheckSuccess(const Connection& conn) {
	return conn.err_code.value() == boost::system::errc::success && !conn.IsDisconnectMessage();
}

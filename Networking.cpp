#include "Networking.h"

bool Connection::IsDisconnectMessage() const {
	return buf[0] == QUIT_KEY;
}

void Disconnect(Connection& conn) {

	conn.buf.fill('\0');
	conn.buf[0] = QUIT_KEY;
	conn.socket.send(boost::asio::buffer(conn.buf.data(), conn.buf.size()), 0, conn.err_code);
	conn.connected = false;

}


bool CheckSuccess(const Connection& conn) {
	return conn.err_code.value() == boost::system::errc::success && !conn.IsDisconnectMessage();
}

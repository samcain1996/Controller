#pragma once

#include "Networking.h"


int main(int argc, char* argv[]) {

	SetProcessDPIAware();

	if (argc == 1) {
		Controller(udp::endpoint(address::from_string("127.0.0.1"), 30001));
	}
	else { Thrall(30001); }

	return 0;
}
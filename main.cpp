#include <iostream>
#include <thread>
#include "WebServer.h"

using namespace std;

void wServer(WebServer* wS) {
	do {
		try {
			bool stop = wS->launch();
			if(stop)
				break;//on a arrete correctement
		}
		catch (const exception& error) {
			cout << "Error: WebServer and LaserNet suddenly stopped. " << error.what() << endl
				<< "Restarting..." << endl;
		}
	} while (true);
}
//start: sudo ~/projects/LaserNet/bin/ARM/Release/LaserNet.out
int main(void) {
	WebServer wServ;

	thread wThread(wServer, &wServ);
	cout << endl << "Type something to stop the server..." << endl;
	string entry;
	cin >> entry;
	wServ.stop();
	wThread.join();
	cout << "Server stoped!" << endl;
	return 0;
}

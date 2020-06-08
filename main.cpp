#include <iostream>
#include <thread>
#include "WebServer.h"
#include <unistd.h>

#include <stdio.h> // standard input / output functions
#include <string.h> // string function definitions
#include <unistd.h> // UNIX standard function definitions
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <termios.h> // POSIX terminal control definitionss
#include <time.h>   // time calls

using namespace std;

void wServer(WebServer* wS) {
	do {
		try {
			if(wS)
				wS->launch();
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
	cout << endl << "Ctrl+c to stop the server... (Ctrl+z if nothing appened)" << endl << endl;
	//thread wThread(wServer, &wServ);
	wServer(&wServ);

	//wServ.stop();
	//wThread.join();
	cout << "Server stoped!" << endl;
	return 0;
}




//env variables:
/*std::string GetEnv(const std::string& var) {
	//https://stackoverflow.com/questions/5866134/how-to-read-linux-environment-variables-in-c#answer-5866166
	const char* val = getenv(var.c_str());
	if (val == nullptr) { // invalid to assign nullptr to std::string
		return "";
	}
	else {
		return val;
	}
}*/
	//https://bash.cyberciti.biz/guide/Getting_User_Input_Via_Keyboard

	//system("timeout 2 read stopLaserNet");//read introuvable
	//system("read -t 2 stopLaserNet");//-t introuvable
	//https://www.unix.com/shell-programming-and-scripting/252856-read-timeout.html#post302924493
	//system("TMOUT=2 read stopLaserNet");//pas de timeout
	//https://unix.stackexchange.com/questions/191293/bash-function-to-read-user-input-or-interrupt-on-timeout
	//system("sleep 2 || read stopLaserNet");//timeout mais ne prend pas l'input
	//system("timeout --foreground 2 bash -c 'select stopLaserNet in \"yes\" \"no\"; do if(($REPLY==1)); then break; fi; echo \"tip yes to leave\"; done'");

	/*cout << "stopLaserNet = \"" << GetEnv("stopLaserNet") << "\"" << endl;
	cout << "input = \"" << GetEnv("input") << "\"" << endl;
	cout << "REPLY = \"" << GetEnv("REPLY") << "\"" << endl;
	cout << "SHELL = \"" << GetEnv("SHELL") << "\"" << endl;*/

	//fonctionne mais n'exporte pas la variable (elle reste en "local"
	//timeout --foreground 2 bash -c 'select stopLaserNet in "Keep runing" "Stop"; do echo "you choose $stopLaserNet"; break; done'
	//timeout --foreground 2 bash -c 'select temp in "Keep runing" "Stop"; do export stopLaserNet=$REPLY; echo "now stopLaserNet=$stopLaserNet"; break; done'

	//si réussi répondre à:
	//https://stackoverflow.com/questions/58838412/how-to-wait-for-a-value-with-timeout
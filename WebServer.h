#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <iostream>
#include <fstream>
#include <numeric>
#include <unistd.h>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "mongoose.h"

#include "LaserNet.h"

class WebServer
{
private:
	// Mongoose event manager
	struct mg_mgr mgr;
	// Mongoose connection
	struct mg_connection *ncServ;
	// Bind Options
	struct mg_bind_opts bind_opts;
	// Err
	const char *err;
	// Port 
	std::string port;
public:
	WebServer();
	~WebServer();

	void launch();
	void stop();
};
// Struct containing setings for how to server HTTP with mongoose
static struct mg_serve_http_opts s_http_server_opts;
std::string intoString(const char* chr, int size);
// Event handler
static void ev_handler(struct mg_connection *nc, int ev, void *p);
static const std::string getAddr(struct mg_connection *nc);

static void connectionOpen(struct mg_connection *nc);
static void connectionConfirmed(struct mg_connection* nc);
static bool isConnectionConfirmed(const std::string addr);
static void connectionClosed(struct mg_connection *nc);

static void msgReceived(struct mg_connection *nc, void* p);
static void onMsgFromFriend(std::string msg);

static void sendMsg(struct mg_connection *nc, std::string type, std::string arg, bool toEveryone = false);
static void sendStatus(struct mg_connection* nc);
static void sendLaserNetStatus(struct mg_connection* nc);


// Status Methods
static double mem_usage();
static double cpu_usage();
static double temperature();
static int net_usage();
static std::vector<struct mg_connection*> ncs;
static bool isUp = true;
#endif


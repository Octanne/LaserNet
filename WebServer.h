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
	struct mg_connection *nc;
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
};
// Struct containing setings for how to server HTTP with mongoose
static struct mg_serve_http_opts s_http_server_opts;
std::string intoString(const char* chr, int size);
// Event handler
static void ev_handler(struct mg_connection *nc, int ev, void *p);
static void sendMsg(struct mg_connection* nc, char addr[32], std::string type, std::string arg);

// Status Methods
static double mem_usage();
static double cpu_usage();
static double temperature();
static int net_usage();
#endif


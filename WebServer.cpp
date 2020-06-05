#include "WebServer.h"

LASERNET LN;


WebServer::WebServer()
{
	// Init mongoose
	mg_mgr_init(&mgr, NULL);

	//SSL Secure Connection
	memset(&bind_opts, 0, sizeof(bind_opts));
	bind_opts.ssl_cert = "/home/pi/Desktop/certificates/certificate.pem"; //Error normal
	bind_opts.ssl_key = "/home/pi/Desktop/certificates/key.pem"; //Error normal
	bind_opts.error_string = &err;

	// Set port
	port = "8000";

}


WebServer::~WebServer()
{

}

void WebServer::launch() {
	
	std::cout << "Starting WebServer on port " << port << std::endl;
	
	// Start Web Server
	nc = mg_bind_opt(&mgr, port.c_str(), ev_handler, bind_opts);

	// If connection fails
	if (nc == NULL) {
		std::cout << "Failed to create listener : " << err << std::endl;
		return;
	}

	// Set up HTTP server options
	mg_set_protocol_http_websocket(nc);

	s_http_server_opts.document_root = "/home/pi/Desktop/WebInterface";
	s_http_server_opts.enable_directory_listing = "no";

	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}

	//Free up all memory allocated
	mg_mgr_free(&mgr);

	std::cout << LN.setStateCmd("WebServer") << std::endl;
}

std::string intoString(const char* a, int size)
{
	int i;
	std::string s = "";
	for (i = 0; i < size; i++) {
		s = s + a[i];
	}
	return s;
}

// Event handler
static void ev_handler(struct mg_connection *nc, int ev, void *p) {
	switch (ev) {
		case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
			/* New websocket connection. */
			char addr[32];
			mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
				MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
			std::cout << "[WebSocket] connection with " << addr << " has been started." << std::endl;
			break;
		}
		case MG_EV_CLOSE: {
			/* Disconnect WSocket connection */
			if (nc->flags & MG_F_IS_WEBSOCKET) {
				char addr[32];
				mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
					MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
				std::cout << "[WebSocket] connection with " << addr << " has been ended." << std::endl;
			}
			break;
		}
		case MG_EV_WEBSOCKET_FRAME: {
			struct websocket_message *wm = (struct websocket_message *) p;
			/* New websocket message read & send an answer. */
			char addr[32];
			struct mg_str d = { (char *)wm->data, wm->size };
			mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
				MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
			std::cout << "[WebSocket] a new request receive of " << addr <<  " :" << std::endl;

			rapidjson::Document jsonDoc;
			jsonDoc.Parse(intoString(d.p, d.len).c_str());
			rapidjson::Value& secretKey = jsonDoc["secretKey"];
			rapidjson::Value& date = jsonDoc["date"];
			rapidjson::Value& order = jsonDoc["order"];
			rapidjson::Value& args = jsonDoc["args"];

			if (!secretKey.IsNull()) {
				std::cout << "---------------------------------------" << std::endl;
				std::cout << "secretKey : " << secretKey.GetString() << std::endl;
				std::cout << "date : " << date.GetString() << std::endl;
				std::cout << "order : " << order.GetString() << std::endl;
				std::cout << "args : " << args.GetString() << std::endl;
				std::cout << "---------------------------------------" << std::endl;
			}

			if (std::string(secretKey.GetString()) == "secretKey") {
				std::cout << "[WebSocket] SecretKey is correct !" << std::endl;
				if (std::string(order.GetString()) == "status") {

					// Document
					rapidjson::Document jsonDocSend;
					const char* jsonTxt = "{\"type\":\"sysStatus\",\"temp\":\"\",\"cpuUsage\":\"\",\"ramUsage\":\"\",\"netUsage\":\"\",\"webStatus\":\"\",\"syncStatus\":\"\"}";
					jsonDocSend.Parse(jsonTxt);

					// Type
					rapidjson::Value& typeV = jsonDocSend["type"];
					typeV.SetString("sysStatus");
					// Temperature
					rapidjson::Value& tempV = jsonDocSend["temp"];
					tempV.SetDouble(temperature());
					// CPU Usage // A revoir
					rapidjson::Value& cpuPrctV = jsonDocSend["cpuUsage"];
					cpuPrctV.SetDouble(cpu_usage());
					// RAM Usage
					rapidjson::Value& ramPrctV = jsonDocSend["ramUsage"];
					ramPrctV.SetDouble(mem_usage());
					// NET Usage
					rapidjson::Value& netPrctV = jsonDocSend["netUsage"];
					netPrctV.SetDouble(net_usage());
					// Status Lasernet @Todo
					rapidjson::Value& webStatV = jsonDocSend["webStatus"];
					webStatV.SetBool(false);
					rapidjson::Value& syncStatV = jsonDocSend["syncStatus"];
					syncStatV.SetBool(false);

					// Stringify the DOM
					rapidjson::StringBuffer buffer;
					rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
					jsonDocSend.Accept(writer);
					
					mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
					std::cout << "[WebSocket] SysStatus message send to " << addr << " !" << std::endl;
				}
				else if (std::string(order.GetString()) == "command") {

					if (args.GetString() == "help" || args.GetString() == "?") {
						sendMsg(nc, addr, "info", LN.getStateInfo());
					}
					else
						sendMsg(nc, addr, "info", LN.setStateCmd(args.GetString()));
					
				}
				else { // Invalid Order
					sendMsg(nc, addr, "error", "You have send an invalid order!");
				}
			}
			else {
				std::cout << "[WebSocket] SecretKey isn't correct !" << std::endl;
				// Document
				rapidjson::Document jsonDocSend;
				const char* jsonTxt = "{\"type\":\"error\",\"msg\":\"\"}";
				jsonDocSend.Parse(jsonTxt);

				// Type
				rapidjson::Value& typeV = jsonDocSend["type"];
				typeV.SetString("error");
				// Temperature
				rapidjson::Value& tempV = jsonDocSend["msg"];
				tempV.SetString("You have send an invalid secretKey!");

				// Stringify the DOM
				rapidjson::StringBuffer buffer;
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				jsonDocSend.Accept(writer);

				mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
				std::cout << "[WebSocket] Error message send to " << addr << " !" << std::endl;
				// SEND Error => bad key
			}
			break;
		}
		// If event is a http request
		case MG_EV_HTTP_REQUEST: {
			mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
			break;
		}
	}
}

void sendMsg(struct mg_connection* nc, char addr[32], std::string type, std::string arg)
{
	// Document
	rapidjson::Document jsonDocSend;
	const char* jsonTxt = std::string("{\"type\":\"" + type + "\",\"msg\":\"\"}").c_str();
	jsonDocSend.Parse(jsonTxt);

	// Type
	rapidjson::Value& typeV = jsonDocSend["type"];
	typeV.SetString(rapidjson::StringRef(type.c_str()));
	// Temperature
	rapidjson::Value& tempV = jsonDocSend["msg"];
	tempV.SetString(rapidjson::StringRef(arg.c_str()));

	// Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	jsonDocSend.Accept(writer);

	mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
	std::cout << "[WebSocket] " + type + " message send to " << addr << std::endl;
}

// Mem Usage
static double mem_usage() {
	unsigned int memTotal, memFree, buffers, cached;
	char id[18], kb[2];

	std::ifstream file("/proc/meminfo");

	file >> id >> memTotal >> kb \
		>> id >> memFree >> kb \
		>> id >> buffers >> kb \
		>> id >> buffers >> kb \
		>> id >> cached >> kb;

	file.close();

	return (memTotal - memFree) * 100 / memTotal;
}
// CPU Usage
static double cpu_usage() {
	std::ifstream  file("/proc/loadavg");

	char line[5];
	file.getline(line, 5);

	float prct = std::stof(line) * 100 / 4;
	return prct;
}
// Temperature
static double temperature() {
	std::string temp;
	std::ifstream ifile("/sys/class/thermal/thermal_zone0/temp");
	ifile >> temp; ifile.close();
	temp.insert(2, 1, '.'); temp.pop_back(); temp.pop_back();
	return std::stod(temp);
}
// Network Usage => RX per ct
static int net_usage() {
	std::string packet_rx = "0", packet_tx = "0";

	std::ifstream ifiler("/sys/class/net/laser0/statistics/rx_packets");
	ifiler >> packet_rx; ifiler.close();
	std::ifstream ifilet("/sys/class/net/laser0/statistics/tx_packets");
	ifilet >> packet_tx; ifilet.close();

	if((std::stoi(packet_rx) + std::stoi(packet_tx)) == 0) return 0;
	return std::stoi(packet_rx)*100/(std::stoi(packet_rx)+std::stoi(packet_tx));
}
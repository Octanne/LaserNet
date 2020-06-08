#include "WebServer.h"

LASERNET *LN = nullptr;
bool debug = false;


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
WebServer::~WebServer() {
	if (LN) {
		LN->stop();
		delete LN;
	}
	LN = nullptr;
}


void WebServer::launch() {
	
	if(!LN)
		LN = new LASERNET(onMsgFromFriend);

	std::cout << "Starting WebServer on port " << port << std::endl;
	
	// Start Web Server
	ncServ = mg_bind_opt(&mgr, port.c_str(), ev_handler, bind_opts);

	// If connection fails
	if (ncServ == NULL) {
		std::cout << "Failed to create listener : " << err << std::endl;
		mg_mgr_free(&mgr);//Free up all memory allocated
		return;
	}

	// Set up HTTP server options
	mg_set_protocol_http_websocket(ncServ);

	s_http_server_opts.document_root = "/home/pi/Desktop/WebInterface";
	s_http_server_opts.enable_directory_listing = "no";


	std::cout << LN->setStateCmd("WebServer") << std::endl;

	while(isUp)
		mg_mgr_poll(&mgr, 1000);

	//Free up all memory allocated
	mg_mgr_free(&mgr);
}

void WebServer::stop() { isUp = false; }

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
			connectionOpen(nc);// New websocket connection.
			break;
		}
		case MG_EV_CLOSE: {
			connectionClosed(nc);// Disconnect WSocket connection
			break;
		}
		case MG_EV_WEBSOCKET_FRAME: {
			msgReceived(nc, p);// New websocket message read & send an answer.
			break;
		}
		case MG_EV_HTTP_REQUEST: {// If event is a http request (load the page)
			mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
			break;
		}
	}
}

const std::string getAddr(struct mg_connection *nc)
{
	char addr[32];
	mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
	return addr;//TODO: bug ?
}

void connectionOpen(struct mg_connection* nc)
{
	std::cout << "[WebSocket] connection with " << getAddr(nc) << " has been started." << std::endl;
}

void connectionConfirmed(struct mg_connection* nc)
{
	//appellée quand on a reçu un message et que la secret key est valide
	const std::string addr = getAddr(nc);
	if (isConnectionConfirmed(addr)) {
		std::cout << "[WebSocket] connection with " << addr << " is already confirmed." << std::endl;
		return;
	}

	std::cout << "[WebSocket] connection with " << addr << " has been confirmed." << std::endl;
	for (size_t i = 0; i < ncs.size(); i++) {
		const std::string currentAddr = getAddr(ncs.at(i));
		if (currentAddr == addr)
			return;
	}
	ncs.push_back(nc);

	sendMsg(nc, "managment", "connected");

	sendMsg(nc, "connection", "New client is here (total:" + std::to_string(ncs.size()) + ")", true);

	sendMsg(nc, "answer", LN->getStateInfo());//donne l'état de LaserNet actuel
	std::cout << "[WebSocket] Answer message send to " << addr << std::endl;
}

bool isConnectionConfirmed(const std::string addr)
{
	for (size_t i = 0; i < ncs.size(); i++)
		if (getAddr(ncs.at(i)) == addr)
			return true;
	return false;
}

void connectionClosed(struct mg_connection* nc)
{
	const std::string addr = getAddr(nc);
	if (nc->flags & MG_F_IS_WEBSOCKET) {
		std::cout << "[WebSocket] connection with " << addr << " has been ended." << std::endl;
	}
	if (!isConnectionConfirmed(addr))
		return;
	int clientRemoved = 0;
	for (size_t i = ncs.size(); i > 0 ; i--) {
		if (getAddr(ncs.at(i-1)) == addr) {
			ncs.erase(ncs.begin() + i - 1);
			clientRemoved++;
		}
	}
	sendMsg(nc, "managment", "disconnected");//aucun destinnataire en temps normal
	sendMsg(nc, "connection", "Client had leaved (total:" + std::to_string(ncs.size()) + ")", true);
	std::cout << "connectionClosed: " << clientRemoved << " client removed" << std::endl;
}


void msgReceived(struct mg_connection* nc, void* p)
{
	const std::string addr = getAddr(nc);

	struct websocket_message* wm = (struct websocket_message*) p;
	struct mg_str d = { (char*)wm->data, wm->size };

	rapidjson::Document jsonDoc;
	jsonDoc.Parse(intoString(d.p, d.len).c_str());

	if (jsonDoc["type"].IsNull()) {
		sendMsg(nc, "error", "You have send an invalid message!");
		if (debug)
			std::cout << "[WebSocket] Error message send by " << addr << " : Invalid message" << std::endl;
		if (!jsonDoc["order"].IsNull())
			std::cout << "[WebSocket] Error message send by " << addr << " : You are using an old version" << std::endl;
		return;
	}

	if (std::string("auth") == jsonDoc["type"].GetString()) {
		if (debug)
			std::cout << "[WebSocket] New request received from " << addr << " : authentification" << std::endl;

		if (isConnectionConfirmed(addr)) {
			sendMsg(nc, "connection", "Authentification is already confirmed!");
			return;
		}
		if (debug) {
			std::cout << "---------------------------------------" << std::endl;
			std::cout << "auth with : " << addr << std::endl;
			std::cout << "secretKey : " << jsonDoc["key"].GetString() << std::endl;
			std::cout << "date : " << jsonDoc["date"].GetString() << std::endl;
			std::cout << "---------------------------------------" << std::endl;
		}
		if (std::string("secretKey") == jsonDoc["key"].GetString()) {//secretKey
			connectionConfirmed(nc);
			sendMsg(nc, "connection", "Authentification confirmed!");
		}
		else {
			if (debug)
				std::cout << "[WebSocket] SecretKey isn't correct for " << addr << std::endl;
			sendMsg(nc, "connection", "You have send an invalid secretKey!");
		}

		return;
	}

	if (!isConnectionConfirmed(addr)) {
		sendMsg(nc, "answer", "You are not authentified.");
		return;//connection invalide
	}

	std::string time = jsonDoc["date"].GetString();
	std::string type = jsonDoc["type"].GetString();
	std::string args = jsonDoc["args"].GetString();


	if (debug && type != "status") {
		std::cout << "[WebSocket] New request received from " << addr
			<< " : " << type
			<< " at " << time
			<< " (args: " << args << ")" << std::endl;
	}
	if (type == "status") {
		sendStatus(nc);
	}
	else if (type == "command") {
		if (args == "help" || args == "?") {
			sendMsg(nc, "answer",
				"help or ?: display this help\n"
				"status: display the current status of LaserNet\n"
				"chat [msg]: send a message to everyone\n"
				"debug (enable/disable): display/edit the current status of debuging c++ part\n"
				"upload (enable/disable): display/edit the current status of LaserNet's Upload\n"
				"download (enable/disable): display/edit the current status of LaserNet's Download\n"
				"stop: stop LaserNet and WebServer\n"
				"restart: restart LaserNet");
		}
		else if (args == "status") {
			sendLaserNetStatus(nc);
		}
		else if (args.rfind("chat ", 0) == 0) {//si il est en pos 0
			LN->sendMsgToFriend(args);
			args = args.replace(0, 5, "");//5 (espace compris)
			sendMsg(nc, "chat", args, true);
		}
		else if (args.rfind("debug", 0) == 0) {
			args.replace(0, 5, "");//5
			if (args.size() > 0) {
				if(args.rfind(" ", 0) == 0)
					args.replace(0, 1, "");
				debug = (args == "enabled" || args == "enable" || args == "true" || args == "on");
			}
			sendMsg(nc, "answer", std::string("Current state for debug: ") + (debug ? "enabled" : "disabled"));
		}
		else if (args == "stop") {
			std::cout << "[WebSocket] Stopped by " << addr << std::endl;
			sendMsg(nc, "answer", "WebServer is stopping...", true);//si on reçoit ce message 'est qu'il s'est pas arreté
			isUp = false;
		}
		else if (args == "restart") {
			std::cout << "[WebSocket] LaserNet restarted by " << addr << std::endl;
			LN->reset();
			std::cout << LN->setStateCmd("WebServer") << std::endl;

			sendMsg(nc, "answer", "LaserNet had been restarted", true);
			sendLaserNetStatus(nc);
		}
		else {
			std::string retour = LN->setStateCmd(args);
			if (retour == "") {
				sendLaserNetStatus(nc);
			}
			else {
				sendMsg(nc, "answer", retour, true);
				std::cout << "[WebSocket] Answer message send to " << addr << std::endl;
			}
		}
	}
	else { // Invalid type
		sendMsg(nc, "error", "You have send an invalid type!");
		if (debug)
			std::cout << "[WebSocket] Error message send to " << addr << " : Invalid type" << std::endl;
	}
}

void onMsgFromFriend(std::string msg)
{
	if (msg.rfind("chat", 0) == 0) {
		msg = msg.replace(0, 4, "");//5-1
		sendMsg(nullptr, "chat", msg, true);
	}
	else
		std::cout << "[WebSocket] message received from Friend unknow: \"" << msg << "\"" << std::endl;
}


void sendMsg(struct mg_connection* nc, std::string type, std::string arg, bool toEveryone)
{
	// Document
	rapidjson::Document jsonDocSend;
	jsonDocSend.Parse("{\"type\":\"unset\",\"msg\":\"\"}");

	// Type
	jsonDocSend["type"].SetString(rapidjson::StringRef(type.c_str()));

	// Content
	jsonDocSend["msg"].SetString(rapidjson::StringRef(arg.c_str()));

	// Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	jsonDocSend.Accept(writer);

	if (toEveryone) {
		for (size_t i = 0; i < ncs.size(); i++) {
			mg_send_websocket_frame(ncs.at(i), WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
		}
		if(nc && !isConnectionConfirmed(getAddr(nc)))//si il fait pas partit de la boucle d'avant
			mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
	}
	else
		mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
}

void sendStatus(mg_connection* nc)
{
	// Document
	rapidjson::Document jsonDocSend;
	const char* jsonTxt = "{\"type\":\"sysStatus\","
		"\"temp\":\"\",\"cpuUsage\":\"\",\"ramUsage\":\"\",\"webStatus\":\"\","
		"\"wiringPiStatus\":\"\",\"tinsStatus\":\"\",\"workingTime\":\"\","
		"\"uploadStatus\":\"\",\"downloadStatus\":\"\",\"totalUpload\":\"\",\"totalDownload\":\"\"}";
	jsonDocSend.Parse(jsonTxt);

	
	jsonDocSend["type"].SetString("sysStatus");// Type

	jsonDocSend["temp"].SetDouble(temperature());// Temperature
	jsonDocSend["cpuUsage"].SetDouble(cpu_usage());// CPU Usage
	jsonDocSend["ramUsage"].SetDouble(mem_usage());// RAM Usage
	jsonDocSend["webStatus"].SetBool(false);// Connected to the Web TODO

	// LaserNet Status
	jsonDocSend["wiringPiStatus"].SetBool(LN->isWiringPiOk());
	jsonDocSend["tinsStatus"].SetBool(LN->isTinsOk());
	jsonDocSend["workingTime"].SetInt(LN->workingTime());

	jsonDocSend["uploadStatus"].SetBool(LN->isUploadEnabled());
	jsonDocSend["downloadStatus"].SetBool(LN->isDownloadEnabled());
	jsonDocSend["totalUpload"].SetInt(LN->totalUpload());
	jsonDocSend["totalDownload"].SetInt(LN->totalDownload());

	// Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	jsonDocSend.Accept(writer);

	mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buffer.GetString(), buffer.GetSize());
	if (debug)
		std::cout << "[WebSocket] SysStatus message send to " << getAddr(nc) << " !" << std::endl;
}

void sendLaserNetStatus(mg_connection* nc)
{
	sendMsg(nc, "answer", LN->getStateInfo(), true);
	std::cout << "[WebSocket] Answer message send to everyone (by " << getAddr(nc) << ")" << std::endl;
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
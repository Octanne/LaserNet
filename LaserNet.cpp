#include "LaserNet.h"

int stat_totalSizePkt = 0;
bool WiringPiOk = false;
bool TinsOk = false;
int pinC = 0;
int pinL = 0;
Tins::NetworkInterface iface = Tins::NetworkInterface::default_interface();

std::string printAllInterfaces(Tins::NetworkInterface ifaceDefault) {
	//debug de chaques interfaces
	std::string retour = "Network Interface :\n";
	std::vector<Tins::NetworkInterface> interfaces = Tins::NetworkInterface::all();
	for (size_t i = 0; i < interfaces.size(); i++) {
		if (ifaceDefault.friendly_name() == interfaces.at(i).friendly_name())
			retour += "> ";
		else
			retour += "  ";
		retour += printInterface(interfaces.at(i));
	}
	return retour;
}
std::string printInterface(Tins::NetworkInterface iface)
{
	std::string retour = toString(iface.friendly_name())
		+ ", ipv4:" + iface.ipv4_address().to_string()
		+ ", ipv6:{";
	std::vector<Tins::NetworkInterface::IPv6Prefix> ipv6 = iface.ipv6_addresses();
	for (size_t i = 0; i < ipv6.size(); i++) {
		if (i > 0)
			retour += ", ";
		retour += ipv6.at(i).address.to_string();
	}
	retour += "}, hw:" + iface.hw_address().to_string() + "\n";
	return retour;
}
std::string toString(std::wstring wstr) { return std::string(wstr.begin(), wstr.end()); }

bool checkPins() {
	WiringPiOk = false;
	std::cout << (PinLib::pinPToWP(pinC) == -1 ? "[ERREUR]" : "[OK]")
		<< " Pin du capteur : " << PinLib::pinPToWP(pinC) << "/" << pinC << std::endl;
	std::cout << (PinLib::pinPToWP(pinL) == -1 ? "[ERREUR]" : "[OK]")
		<< " Pin du laser : " << PinLib::pinPToWP(pinL) << "/" << pinL << std::endl;

	if (pinC == pinL) {
		std::cout << "[ERREUR] Les pins du capteur et du laser sont les memes" << std::endl;
		return WiringPiOk;//false
	}
	WiringPiOk = (PinLib::pinPToWP(pinC) != -1 && PinLib::pinPToWP(pinL) != -1);//les 2 doivent etre valide
	return WiringPiOk;
}
bool checkNetwork() {
	TinsOk = false;

	try { iface.name(); }
	catch (const std::exception& error)
	{
		std::cout << "[ERREUR] Impossible de se connecter a internet avec " << toString(iface.friendly_name()) << " : " << error.what() << std::endl;
		return false;
	}

	try
	{
		Tins::Sniffer sniffTemp = Tins::Sniffer(iface.name());
		sniffTemp.stop_sniff();//arrete par securitée
	}
	catch (const std::exception& error)
	{
		std::cout << "[ERREUR] Impossible de creer des Sniffers avec " << toString(iface.friendly_name()) << " : " << error.what();
#ifdef __linux__ //ERROR SNIFFER
		std::cout << ", verifiez que vous avez execute avec sudo :\n"
			<< "sudo ~/projects/LaserNet_J/bin/ARM/Release/LaserNet_J.out";
#else
		std::cout << ", verifiez que le programme a les dll et a les autorisations.";
#endif //ERROR SNIFFER
		std::cout << std::endl;
		return TinsOk;//false
	}


	std::cout << "[OK] Interface actuelle: "; printInterface(iface);
	TinsOk = true;
	return TinsOk;
}

//thread Sniffer-Laser
LASERNET_LASER::LASERNET_LASER(int pinP, Tins::NetworkInterface& iface, Tins::SnifferConfiguration config)
{
	askToStop = false;
	try {
		sniffer = new Tins::Sniffer(iface.name(), config);
		sniffer->set_timeout(500);
	}
	catch (std::exception& error) {
		std::cout << "[LaserNet_J/laser/init] error creation sniffer " << error.what() << std::endl;
	}

	las = new LASER(pinP);
	if (las->isConnected())
		std::cout << "[LaserNet_J/laser/init] laser cree sur le pin " << pinP << std::endl;
	else//on le lance quant meme
		std::cout << "[LaserNet_J/laser/init] impossible de creer le laser sur le pin " << pinP << std::endl;
	//TODO: mettre le start avec la LaserNet_Transfert
}
LASERNET_LASER::~LASERNET_LASER()
{
	std::cout << "[LaserNet_J/laser] Closing of a thread..." << std::endl;
	askToStop = true;

	if (thread) {
		thread->join();
		delete thread;
	}
	if (sniffer)
		delete sniffer;
	if (las)
		delete las;
	std::cout << "[LaserNet_J/laser] Closed" << std::endl;
}
void LASERNET_LASER::start()
{
	isRunning = true;
	askToStop = false;
	if (thread != nullptr)
		delete thread;
	thread = new std::thread(processLaser, this);
}

bool LASERNET_LASER::send(uint8_t data)
{
	if (!las || askToStop || !las->isValid())
		return false;

	packet::POut pout;
	pout << uint8_t(0);
	pout << data;

	las->sendPkt(pout, &askToStop);
	return !askToStop;
}
bool LASERNET_LASER::send(std::string msg)
{
	if (!las || askToStop || !las->isValid())
		return false;

	packet::POut pout;
	pout << uint8_t(1);
	pout << msg;

	las->sendPkt(pout, &askToStop);
	return !askToStop;//on continue ? => tant qu'on a pas demandé de stop
}

bool LASERNET_LASER::send(const Tins::Packet& pkt)
{
	if (!las || askToStop || !las->isValid())
		return false;

	packet::POut pout;
	pout << uint8_t(2);//type pkt

	pout << (uint16_t)pkt.pdu()->pdu_type();
	Tins::PDU* pdu = pkt.pdu()->clone();
	if (!pdu) {
		std::cout << "[LaserNet_J/laser/send] pdu introuvable" << std::endl;
		return !askToStop;
	}
	std::vector<uint8_t> data = pdu->serialize();
	delete pdu;
	pout << data;
	pout << (uint64_t)pkt.timestamp().seconds();//time_t =>8 byte
	pout << (uint32_t)pkt.timestamp().microseconds();//suseconds =>4 byte

	if(counterPacket % 100 == 0)
		std::cout << "[LaserNet_J/laser/send] envoye du packet " << counterPacket << " (size:" << pout.size() << ")" << std::endl;
	las->sendPkt(pout, &askToStop);
	//TODO: if debug
	//std::cout << "[LaserNet_J/laser/send] packet " << counterPacket << " renvoye " << pout << std::endl;
	stat_totalSizePkt += pout.size();
	return !askToStop;
}

void LASERNET_LASER::process()
{
	isRunning = true;
	if (sniffer == nullptr) {//pas besoin de s'attarder
		isRunning = false;
		return;
	}

	las->sleepUs(2000000);//2sec le temps que l'autre ai eu le temps de démarrer
	counterPacket = 0;
	stat_totalSizePkt = 0;
	while (!askToStop) {//tant qu'il demande pas de stop
		try {
			const Tins::Packet pkt = sniffer->next_packet();
			if (pkt) {
				send(pkt);
				counterPacket++;
			}
		}
		catch (std::exception& error) {
			std::cout << "[LaserNet_J/laser/process] Error: " << error.what() << std::endl;
		}
	}
	std::cout << "[LaserNet_J/laser] thread termine apres " << counterPacket << " packets\n";
	isRunning = false;
}

//thread Capteur-Sender
LASERNET_CAPTEUR::LASERNET_CAPTEUR(int pinP, Tins::NetworkInterface iface, void (*onMsgFromFriend)(std::string))
{
	askToStop = false;
	this->onMsgFromFriend = onMsgFromFriend;
	try {
		sender = new Tins::PacketSender(iface.name());
	}
	catch (std::exception& error) {
		std::cout << "[LaserNet_J/capteur/init] error creation sender " << error.what() << std::endl;
	}

	if (PinLib::pinPToWP(pinP) != -1) {//si il est valide
		cap = new CAPTEUR(pinP);
		std::cout << "[LaserNet_J/capteur/init] capteur cree sur le pin " << pinP << std::endl;
	}
	else
		std::cout << "[LaserNet_J/capteur/init] impossible de creer le capteur sur le pin " << pinP << std::endl;
}
LASERNET_CAPTEUR::~LASERNET_CAPTEUR()
{
	std::cout << "[LaserNet_J/capteur] Closing of a thread..." << std::endl;
	askToStop = false;
	thread->join();
	if (thread)
		delete thread;
	if (sender)
		delete sender;
	if (cap)
		delete cap;
	std::cout << "[LaserNet_J/capteur] Closed" << std::endl;
}
void LASERNET_CAPTEUR::start()
{
	askToStop = false;
	if (thread != nullptr)
		delete thread;
	thread = new std::thread(processCapteur, this);
}

void LASERNET_CAPTEUR::process()
{
	isRunning = true;
	if (!cap || !cap->isValid()) {
		isRunning = false;
		return;
	}
	counterPacket = 0;
	while (!askToStop) {
		try {
			bool ok = true;
			packet::PIn pkt = cap->getPkt(&askToStop, &ok);// receiveData(&ok);
			if (!ok)
				continue;
			//TODO: if debug
			//std::cout << "[LaserNet_J/capteur/process] packet recu " << counterPacket << " (size:" << pkt.size() << "): " << pkt << std::endl;
			uint8_t type = 0;
			pkt >> type;//change la taille, ne pas oublier de dire que 8 bits sont en moins !
			if (type == 0) {
				uint8_t data1 = 0;
				pkt >> data1;
				std::cout << "[LaserNet_J/capteur/process] uint8_t recu : " << data1 << std::endl;
			}
			else if (type == 1) {
				std::string data1 = "";
				pkt >> data1;
				std::cout << "[LaserNet_J/capteur/process] string recu : " << data1 << std::endl;
				if (onMsgFromFriend != nullptr)
					onMsgFromFriend(data1);
			}
			else if (type == 2) {
				sendPkt(pkt);
				std::cout << "[LaserNet_J/capteur/process] packet " << counterPacket << " renvoye avec Tins" << std::endl;
			}

		}
		catch (std::exception& error) {
			if(error.what() != "60 seconds of inactivity")
				std::cout << "[LaserNet_J/capteur/process] Error: " << error.what() << std::endl;
			//todo_bug parfois :
			//pin contient 32bits mais il etait prevu d'y en avoir 0.
			//[LaserNet_J / capteur / process] Error: position too high
		}
		counterPacket++;
	}
	std::cout << "[LaserNet_J/capteur] thread termine apres " << counterPacket << " packets\n";
	isRunning = false;
}

void LASERNET_CAPTEUR::sendPkt(packet::PIn& pkt)
{
	uint16_t pduType;
	pkt >> pduType;
	std::vector<uint8_t> data;
	pkt >> data;

	uint64_t seconds;
	pkt >> seconds;
	uint32_t microseconds;
	pkt >> microseconds;
	timeval tv;
	tv.tv_sec = __time_t(seconds);
	tv.tv_usec = microseconds;
	Tins::Timestamp timestamp(tv);


	switch (pduType) {
	case Tins::PDU::ETHERNET_II:
		sendPktPDU<Tins::EthernetII>(data, timestamp);
		break;
	default:
		std::cout << "LaserNet_J error : pduType inconue : " << pduType << std::endl;
		break;
	}
}
template<typename T>
inline void LASERNET_CAPTEUR::sendPktPDU(const std::vector<uint8_t> data, Tins::Timestamp timestamp)
{//envoyer un packet (via ethernet) à partir d'un buffer
	if (!sender)
		return;//ça sert à rien

	uint8_t buffer[uint32_t(data.size())];
	for (size_t i = 0; i < data.size(); i++) {
		buffer[i] = uint8_t(data.at(i));
	}
	T pdu(buffer, uint32_t(data.size()));

	sender->send(pdu);
}


//appel via thread
void processLaser(LASERNET_LASER* las) { if (las) las->process(); }
void processCapteur(LASERNET_CAPTEUR* cap) { if (cap) cap->process(); }


//partie Transfert (classique)
LaserNet_Transfert::LaserNet_Transfert(void (*onMsgFromFriend)(std::string))
{
	isRunning = false;

	Tins::SnifferConfiguration config;
	//config.set_filter("ip dst " + iface.ipv4_address().to_string());//destinés à moi
	std::cout << "filter: '" << "ip dst " + iface.ipv4_address().to_string() << "'" << std::endl;
	//peut etre ipv6 ? (ou hw)
	threadLas = new LASERNET_LASER(pinL, iface, config);
	threadCap = new LASERNET_CAPTEUR(pinC, iface, onMsgFromFriend);
}
LaserNet_Transfert::~LaserNet_Transfert()
{
	if (threadLaser != nullptr)
		delete threadLaser;
	if (threadTins != nullptr)
		delete threadTins;
	if (threadLas != nullptr)
		delete threadLas;
	if (threadCap != nullptr)
		delete threadCap;
}
void LaserNet_Transfert::exec()
{
	std::cout << "[LaserNet_Transfert] Demarrage du transfert..." << std::endl;
	if (!checkTransfert())
		return;

	isRunning = true;

	threadLas->start();
	threadCap->start();
}
bool LaserNet_Transfert::checkTransfert()
{
	//check des pins et de libtin pour que tout soit parfait
	return WiringPiOk && TinsOk;
}
void LaserNet_Transfert::stop()
{
	//on attend que les 2 threads soient off
	if (threadLas)
		threadLas->close();
	if (threadCap)
		threadCap->close();
	bool threadLasOn = true;
	bool threadCapOn = true;
	int64_t nextActuLog = CAPTEUR::getCurrentus() + 1000000;//1 sec evite de l'activer tout de suite

	while (threadLasOn || threadCapOn) {
		SENSOR::sleepUs(1000);//1ms
		threadLasOn = (threadLas && threadLas->getIsRunning());
		threadCapOn = (threadCap && threadCap->getIsRunning());

		if (nextActuLog < CAPTEUR::getCurrentus()) {//actualise le message de statut
			nextActuLog = CAPTEUR::getCurrentus() + 2000000;//2 sec
			std::cout << "En attente de thread :";
			if (threadLasOn) {
				std::cout << " threadLas (LASERNET_LASER)";
				if (threadCapOn)
					std::cout << " et";
			}
			if (threadCapOn)
				std::cout << " threadCap (LASERNET_CAPTEUR)";
			std::cout << std::endl;
		}
	}
	std::cout << "[LaserNet_Transfert] stopped" << std::endl;
}

void LaserNet_Transfert::sendMsgToFriend(std::string msg)
{ threadLas->send(msg); }



LASERNET::LASERNET(void (*onMsgFromFriend)(std::string)) {
	this->onMsgFromFriend = onMsgFromFriend;
}

LASERNET::~LASERNET()
{
	if (LNT != nullptr) {
		delete LNT;
		LNT = nullptr;
	}
}

void LASERNET::stop() {
	if(LNT != nullptr)
		LNT->stop();
}


std::string LASERNET::setStateCmd(std::string command)
{
	switch (state)
	{
	case states::INIT:
#ifdef __linux__ //WELCOM
		iface = Tins::NetworkInterface("wlan0");//par défaut, l'interface est wlan0, celle qu'on veut
		if (wiringPiSetup() < 0) {
			return "setup wiring pi failed";
		}
#else
		return "This system is not defined for LaserNet";
#endif //WELCOM
		state = states::ASK_CAPTEUR;
		return "[LaserNet] Init was done with success";
	case states::ASK_CAPTEUR:
		std::cout << "[LaserNet] state: ASK_CAPTEUR" << std::endl;
		pinC = PinLib::pinPValid(command, 36);
		std::cout << "[LaserNet] pinC: " << pinC << std::endl;
		state = states::ASK_LASER;
		break;
	case states::ASK_LASER:
		std::cout << "[LaserNet] state: ASK_LASER" << std::endl;
		pinL = PinLib::pinPValid(command, 38);
		std::cout << "[LaserNet] pinL:" << pinL << std::endl;
		if (!checkPins()) {
			state = states::ASK_CAPTEUR;
			return "Error: invalids pins, try again";
		}
		state = states::ASK_INTERFACE;
		break;
	case states::ASK_INTERFACE: {
		std::vector<Tins::NetworkInterface> interfaces = Tins::NetworkInterface::all();
		for (size_t i = 0; i < interfaces.size(); i++)
			if (toString(interfaces.at(i).friendly_name()) == command) {
				iface = interfaces.at(i);
				break;
			}
		if (!checkNetwork()) {
			return "Error: invalid interface, try again";
		}

		state = states::READY;
		LNT = new LaserNet_Transfert(onMsgFromFriend);
		LNT->exec();
		break;
		}
	case states::READY:
		break;//we can't do nothing
	}
	return getStateInfo(false);//question suivante
}

std::string LASERNET::getStateInfo(bool complet) const
{
	switch (state)
	{
	case states::INIT:
		return "[LaserNet] initialisation...";
	case states::ASK_CAPTEUR:
		return "[LaserNet] Entrer les pins PHYSIQUES (default:WP/Physic)\nPin du Capteur (defaut:36):";
	case states::ASK_LASER:
		return "[LaserNet] Entrer les pins PHYSIQUES (default:WP/Physic)\nPin du Laser (defaut:38):";
	case states::ASK_INTERFACE:
		return printAllInterfaces() + "Quelle interface ? ('non' pour celle par defaut)";
	case states::READY:
		if (complet) {
			return std::string("The transmisison is OK (" + WiringPiOk) + std::string("," + TinsOk) + ")\n" +
				"	Pin Capteur: " + std::to_string(pinC) + "\n" +
				"	Pin Laser: " + std::to_string(pinL) + "\n" +
				"	Interface: " + toString(iface.friendly_name()) + "\n" +
				"	Stats pkt size count:" + std::to_string(stat_totalSizePkt);
		}
		else
			return "The transmission is OK, help to have more informations";
	}
	return "Error: states unknow: " + state;
}

void LASERNET::sendMsgToFriend(std::string msg)
{
	if(LNT)
		LNT->sendMsgToFriend(msg);
}


#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <tins/tins.h>//ne JAMAIS mettre avec <windows.h>
#include <tins/loopback.h>
#include <tins/pktap.h>
#include <pcap/pcap.h>//meme chose que pcap.h
#ifdef __linux__
#define wiringPi//on peut le désactiver quant on veut
#ifdef wiringPi
#include <wiringPi.h>
#endif //wiringPi
#include <unistd.h>
#endif // __linux__
#include <stdio.h>
#include <stdlib.h>

#include "sensor.h"
#include "ByteArray.h"
#include "WebServer.h"


bool checkPins();
bool checkNetwork();
std::string printAllInterfaces(Tins::NetworkInterface ifaceDefault = Tins::NetworkInterface::default_interface());
std::string printInterface(Tins::NetworkInterface iface);
std::string toString(std::wstring wstr);



/*format des pkts :
   0 = state (int)
   1 = msg (string)
   2 = pkt (Tins::PDU)
*/
class LASERNET_LASER
{
public:
	LASERNET_LASER(int pinP, Tins::NetworkInterface& iface, Tins::SnifferConfiguration config);
	~LASERNET_LASER();
	void start();
	void close() { askToStop = true; };
	void join() { if(thread) thread->join(); }
	bool getIsRunning() const { return isRunning; };
	bool send(uint8_t data);
	bool send(std::string msg);
	bool send(const Tins::Packet& pkt);

	void process();
private:
	std::thread* thread = nullptr;
	bool isRunning = false;//continue de tourner
	bool askToStop = false;//demande d'arret
	Tins::Sniffer* sniffer = nullptr;
	LASER* las = nullptr;
	int counterPacket = 0;
};
class LASERNET_CAPTEUR
{
public:
	LASERNET_CAPTEUR(int pinP, Tins::NetworkInterface iface, void (*onMsgFromFriend)(std::string));
	~LASERNET_CAPTEUR();
	void start();
	void close() { askToStop = true; };
	void join() { if (thread) thread->join(); }
	bool getIsRunning() { return isRunning; };
	void process();
private:
	std::thread* thread = nullptr;
	bool isRunning = false;//continue de tourner
	bool askToStop = false;//demande d'arret
	Tins::PacketSender* sender = nullptr;
	CAPTEUR* cap = nullptr;
	int counterPacket = 0;
	void (*onMsgFromFriend)(std::string) = nullptr;

	//packet::PIn receiveData(bool *ok);//attend et donne un packet complet
	void sendPkt(packet::PIn& pkt);//envoyer les pkts de Tins
	template <typename T> void sendPktPDU(const std::vector<uint8_t> data, Tins::Timestamp timestamp);//sous fonction de sendPkt
};

void processLaser(LASERNET_LASER* las);
void processCapteur(LASERNET_CAPTEUR* cap);

class LaserNet_Transfert {
public:
	LaserNet_Transfert(void (*onMsgFromFriend)(std::string));
	~LaserNet_Transfert();
	void exec();
	bool checkTransfert();
	void stop();
	void sendMsgToFriend(std::string msg);
	bool enableUpload(bool enable);
	bool enableDownload(bool enable);
	bool isUploadEnabled() const { if (threadLas) return threadLas->getIsRunning(); return false; }
	bool isDownloadEnabled() const { if (threadCap) return threadCap->getIsRunning(); return false; }

private:
	bool isRunning = true;

	int counterPacketC = 0;
	int counterPacketL = 0;

	std::thread* threadLaser = nullptr;//laser => tins
	std::thread* threadTins = nullptr;//tins => laser
	bool threadLaserOn = false;
	bool threadTinsOn = false;
	LASERNET_LASER* threadLas = nullptr;
	LASERNET_CAPTEUR* threadCap = nullptr;

};


class LASERNET {
public:
	LASERNET(void (*onMsgFromFriend)(std::string));
	~LASERNET();
	void reset();
	void stop();
	std::string setStateCmd(std::string command);
	std::string getStateInfo(bool complet = true) const;
	void sendMsgToFriend(std::string msg);
	int totalUpload() const;
	int totalDownload() const;
	bool isWiringPiOk() const;
	bool isTinsOk() const;
	int64_t workingTime() const;
	bool isUploadEnabled() const { if (LNT) return LNT->isUploadEnabled(); return false; }
	bool isDownloadEnabled() const { if (LNT) return LNT->isDownloadEnabled(); return false; }
	
private:
	enum states {
		INIT,
		ASK_CAPTEUR,
		ASK_LASER,
		ASK_INTERFACE,
		READY
	};
	int state = 0;
	LaserNet_Transfert *LNT = nullptr;
	void (*onMsgFromFriend)(std::string) = nullptr;
};

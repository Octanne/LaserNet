#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#ifdef __linux__
	#define wiringPi//on peut le désactiver quant on veut
	#ifdef wiringPi
		#include <wiringPi.h>
	#endif // wiringPi
	#include <unistd.h>
#else
	#include <Windows.h>
#endif //__linux__
#include "ByteArray.h"


#include <iomanip>
#include <thread>




//objet pour gerer les pins (utilisé par capteur)
class PinLib {
public:
	static int pinPToWP(int physical);//convertit un pin physical en pin WiringPi
	static int pinWPToP(int pinWP);
	//disposition des pins :
	//https://www.aranacorp.com/wp-content/uploads/raspberrypi-gpio-wiringpi-pinout.png
	//https://fr.pinout.xyz/pinout/wiringpi
	//Commande RaspberryPi: gpio readall
	//http://wiringpi.com/

	static int pinPValid(std::string pin, int pinDefault = -1);
	static int askNewPin(int pinDefault, std::string nameSensor);
};



class moyenne {
public:
	moyenne() { values = std::vector<int64_t>(); };
	~moyenne() { clear(); };
	void clear() { values.clear(); };
	void addValue(int64_t value) { values.push_back(value); };
	void operator+=(int64_t value) { addValue(value); };
	void debug() const;
	int64_t getMoyenne() const;
	int64_t getMin() const;
	int64_t getMax() const;
	size_t getNbValue() const { return values.size(); };
private:
	std::vector<int64_t> values;
};

//gère les instruments (laser ou capteur)
class SENSOR
{
	public:
		SENSOR(bool modeOut, int pinP);
		~SENSOR() { disconnect(); }
		bool setPin(int pinP);
		inline bool setPinWP(int pinWP) { return setPin(PinLib::pinWPToP(pinWP)); };
		inline int getPin() const { return pinP; }
		inline int getPinWP() const { return pinWP; }
		inline bool isValid() const { return pinWP != -1; };//le pin est valide
		inline bool isConnected() const { return connected; }//le pin est connecté pour le laser ou le capteur

		const static int64_t safeTime = 500;//us : temps de pause par bit = 0.5ms
		static int64_t getCurrentus();//µs since epoch
		//static int64_t getCurrentns();//ns since system start
		void startSafe() { lastus = getCurrentus(); };//initialise le chrono pour le safePause
		int safePause(int64_t timePause = safeTime);//fait une pause et gère le temps écoulé dans le code
		static void sleepUs(int64_t time);//sleep with µs
		//static void sleepNs(int64_t time);//sleep with ns
		int waitToNextBit(int64_t safeTime = safeTime);

		void debugTimeSleep() const;
		//moyenne moyenneSleep = moyenne();//les temps écoulé hors sleep (temps requis)
		//moyenne moyenneBeforeSleep = moyenne();
		//moyenne moyenneBeforeSleep2 = moyenne();
		moyenne moyenneDecaleSleepWithTime = moyenne();


		bool getState() const;//pour capteur ou laser
		void setState(bool stateUp);//pour laser uniquement
		bool setStateConfirm(bool stateUp);//setState and return if state have changed
		int64_t getLastUS() const { return lastus; };

	protected:
		void setMode(bool modeOut) { SENSOR::modeOut = modeOut; actuConnect(); }//in = capteur
		bool getMode() const { return modeOut; }

		//private state for internal function
		bool getStateP() const;
		void setStateP(bool stateUp);

		int pinP;
		int pinWP;
		bool modeOut = false;//capteur ou laser?
		int64_t lastus = getCurrentus();//derniere execution du safePause, faire un startSafe() avant chaque bloc.

		bool actuConnect();
		bool connected = false;
		void disconnect();
};

class CAPTEUR : public SENSOR
{
	public:
		CAPTEUR(int pinP);
		void setMode(bool modeOut) { throw std::invalid_argument("CAPTEUR can't change his mode"); }

		bool waitChange(const bool *askToStop);//waitBit : attend un changement de bit
		packet::PIn getPkt(const bool *askToStop, bool *ok);
};
class LASER : public SENSOR
{
	public:
		LASER(int pinP);
		void setMode(bool modeOut) { throw std::invalid_argument("LASER can't change his mode"); }

		void sendPkt(const packet::POut &pkt, const bool *askToStop);

		void testLaser();//clignote
};
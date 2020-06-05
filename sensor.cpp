#include "sensor.h"

const int pinsWP[40] = {//liste des WiringPi en disposition Physical
		-1, -1,
		8, -1,
		9, -1,
		7, 15,
		-1, 16,
		0, 1,
		2, -1,
		3, 4,
		-1, 5,
		12, -1,
		13, 6,
		14, 10,
		-1, 11,
		30, 31,
		21, -1,
		22, 26,
		23, -1,
		24, 27,
		25, 28,
		-1, 29
};
int PinLib::pinPToWP(int pinP)
{//convertit un pin physical en pin WiringPi
	pinP--;
	if (pinP < 0 || 40 <= pinP)
		return -1;
	return pinsWP[pinP];
}
int PinLib::pinWPToP(int pinWP)
{
	if (pinWP == -1)//si c'est indéfini
		return 0;
	for (int iP = 0; iP < 40; iP++)
		if (pinsWP[iP] == pinWP)//on l'a trouvé
			return iP + 1;
	return 0;
}
int PinLib::pinPValid(std::string pin, int pinDefault)
{
	try
	{
		int pinCap = std::stoi(pin);
		if (PinLib::pinPToWP(pinCap) == -1)
			throw std::invalid_argument("not a WiringPi pin");//marche pas
		return pinCap;//sinon : tout va bien
	}
	catch (const std::exception &error)
	{ std::cout << "impossible de trouver le pin \"" << pin << "\" : " << error.what() << std::endl; }
	if (PinLib::pinPToWP(pinDefault) == -1)
		return 0;//autant dire qu'on a rien (pin 0 n'existe pas en physique)
	return pinDefault;
}
int PinLib::askNewPin(int pinDefault, std::string nameSensor)
{
	std::string input = "";
	std::cout << "pin de " << nameSensor << " ? (default:" << PinLib::pinPToWP(pinDefault) << "/" << pinDefault << ") ";
	std::cin >> input;
	int pin = PinLib::pinPValid(input, pinDefault);
	std::cout << "pin de " << nameSensor << " : " << PinLib::pinPToWP(pin) << "/" << pin << std::endl;
	return pin;
}



void moyenne::debug() const
{
	std::cout << "moyenne{ x" << getNbValue()
		<< " [" << getMin() << "-" << getMoyenne() << "-" << getMax() << "] }"
		<< std::endl;
}
int64_t moyenne::getMoyenne() const
{
	if (getNbValue() <= 0) return 0;
	int64_t total = 0;
	for (size_t i = 0; i < getNbValue(); i++) {
		total += values.at(i);
	}
	return total / getNbValue();
}
int64_t moyenne::getMin() const
{
	if (getNbValue() <= 0) return 0;
	int64_t min = values.at(0);
	for (size_t i = 0; i < getNbValue(); i++)
		if(values.at(i) < min)
			min = values.at(i);
	return min;
}
int64_t moyenne::getMax() const
{
	if (getNbValue() <= 0) return 0;
	int64_t max = values.at(0);
	for (size_t i = 0; i < getNbValue(); i++)
		if (max < values.at(i))
			max = values.at(i);
	return max;
}


SENSOR::SENSOR(bool modeOut, int pinP)
{
	SENSOR::modeOut = modeOut;
	setPin(pinP);
}
bool SENSOR::setPin(int pinP)
{
	if (getPin() == pinP)
		return actuConnect();//recalcul de validitée

	disconnect();

	SENSOR::pinP = pinP;
	SENSOR::pinWP = PinLib::pinPToWP(pinP);

	if (!isValid())
		std::cout << "pin " << getPin() << " is not a WiringPi pin" << std::endl;

	return actuConnect();
}



//gestion des pauses safe
int SENSOR::safePause(int64_t timePause)//appeler avant l'est changements pour avoir un timing parfait
{
	//int64_t currentus = getCurrentus();
	//entre currentus et lastus, il doit y avoir 'timePause' µs (objectif)
	//le temps déja écoulé :
	//int64_t dejaEcoule = currentus - lastus;
	//le temps qu'il reste à passer en sleep (pour atteindre l'objectif)
	//int64_t time = timePause - dejaEcoule;
	//sécrit aussi useconds_t tempsRestant = lastMicrosec + timePause - currentusec;
	//(temps objectif (à atteindre) moins le temps actuel)
	
	//optimisation :
	lastus += timePause;//on veut absolument x temps après le précédent pour etre bien callé
	int64_t time = lastus - getCurrentus();//le temps de pose moins la différnce entre les 2 temps (inversé avec --)

	if (time < 0) {
		int bitSkipped = int(-time / timePause);//si time < timePause alors bitSkipped = ~0
		lastus += timePause * bitSkipped;
		if (timePause < safeTime)//on est sur un "fastSleep"
			return bitSkipped + 1;
		//cout en block:
		/*std::string retour = (modeOut ? "laser " : "capteur ") + std::to_string(getPin()) + " a prit un retard de " + std::to_string((-time)) + " ns\n"
			+ "  info: timePause=" + std::to_string(timePause) + ", dejaEcoule=" + std::to_string(dejaEcoule) + ", bit skipped: " + std::to_string(bitSkipped);*/
		std::cout << std::string((modeOut ? "laser " : "capteur ") + std::to_string(getPin()) + " a prit un retard de " + std::to_string((-time)) + " us") << std::endl;
		
		return bitSkipped;//nombres totales de bits qui n'ont pas pu etre passé + celui actuel
	}

	//moyenne des temps déja passé
	//moyenneSleep.addValue(dejaEcoule);
	//moyenneBeforeSleep += (getCurrentus() - currentus);
	//moyenneBeforeSleep2 += (getCurrentus() - currentus);
	sleepUs(time);

	//moyenneDecaleSleepWithTime += (getCurrentus() - lastus);//différence entre lastus et getCurrentus();
	
	return 1;//1 bit a été passé
}
int64_t SENSOR::getCurrentus()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()
		).count();
}
//int64_t SENSOR::getCurrentns() { return std::chrono::steady_clock::now().time_since_epoch().count(); }

void SENSOR::sleepUs(int64_t time)
{
#ifdef __linux__
	usleep((useconds_t)time);
#elif _WIN32
	Sleep(time / 1000);
#else
	std::cout << "sleep n'est pas supporté" << std::endl;
#endif //__linux__
}
//void SENSOR::sleepNs(int64_t time) { std::this_thread::sleep_for(std::chrono::nanoseconds(time)); }

int SENSOR::waitToNextBit(int64_t safeTime)
{
	int64_t t = getCurrentus();
	int bitSkipped = int((t - lastus) / safeTime);
	//le temps en trop par le temps d'un bit = nb de bits skipped
	
	if (bitSkipped > 0) {
		lastus += bitSkipped * safeTime;
		return bitSkipped + waitToNextBit();
	}
	else {
		lastus += safeTime;
		while (getCurrentus() + 1 < lastus)
			sleepUs(1);//+1us car getCurrentus prend 8us et que la suite sera surrement retardée
		return 0;
	}
}

void SENSOR::debugTimeSleep() const
{
	//std::cout << "moyenneSleep: "; moyenneSleep.debug();
	//std::cout << "moyenneBeforeSleep: "; moyenneBeforeSleep.debug();
	//std::cout << "moyenneBeforeSleep2: "; moyenneBeforeSleep2.debug();
	std::cout << "moyenneDecaleSleepWithTime: "; moyenneDecaleSleepWithTime.debug();
}


bool SENSOR::getState() const
{
	if (!isConnected())
		return false;
	return getStateP();
}
void SENSOR::setState(bool stateUp)
{
	if (!modeOut || !isConnected())
		return;//this n'est pas un laser / n'est pas valide
	setStateP(stateUp);
}
bool SENSOR::setStateConfirm(bool stateUp)
{
	if (!modeOut || !isConnected())
		return false;//this n'est pas un laser valide (on veut le savoir si c'est vraiment false)
	setStateP(stateUp);
	return stateUp == getStateP();//on a bien changé ? sinon on est en READ et modeOut est pas bon...
}

bool SENSOR::getStateP() const
{
#ifdef wiringPi
	return digitalRead(pinWP) != 0;
#else
	return false;
#endif // wiringPi
}
void SENSOR::setStateP(bool stateUp)
{
	//std::cout << "setStateP at " << lastus << std::endl;
#ifdef wiringPi
	digitalWrite(pinWP, stateUp ? HIGH : LOW);
#endif // wiringPi
}



bool SENSOR::actuConnect()
{
	connected = false;
	if (!isValid())//si lui est pas valide
		return isConnected();//false
	try
	{
		pinMode(getPinWP(), modeOut ? OUTPUT : INPUT);
		connected = true;
	}
	catch (const std::exception&)
	{
		std::cout << "[CAPTEUR/actuConnect] impossible de creer des sensor sur le pin " << getPin() << std::endl;
	}
	return isConnected();//true si ça a marché, false sinon
}
void SENSOR::disconnect()
{
	if (modeOut && isConnected()) {//laser
		setState(0);
		pinMode(getPinWP(), INPUT);
	}
	connected = false;
}





CAPTEUR::CAPTEUR(int pinP) : SENSOR(false, pinP) {}

bool CAPTEUR::waitChange(const bool *askToStop)
{
	if (askToStop && *askToStop)
		return false;
	if (!isConnected())
		return false;

	bool state = getStateP();
	int64_t end = CAPTEUR::getCurrentus() + 60000000;//60 sec
	startSafe();
	while (state == getStateP()) {
		if (askToStop && *askToStop)
			return false;
		if (CAPTEUR::getCurrentus() >= end) {
			std::cout << "[LaserNet_J/CAPTEUR/waitChange] error 60 seconds of inactivity" << std::endl;
			throw std::runtime_error("60 seconds of inactivity");
		}
		safePause(safeTime / 10);//pause 10 fois plus courte
	}
	return true;
}
packet::PIn CAPTEUR::getPkt(const bool *askToStop, bool *ok)
{
	if (ok) *ok = false;//faux par défaut
	packet::PIn pkt;//construit pour retour vide
	if (!isConnected())
		return pkt;

	if (getStateP() == 1) {//déja ça commence mal
		std::cout << "[LaserNet_J/CAPTEUR] erreur un packet est deja lance" << std::endl;
#if true
		waitChange(askToStop);//attendre que le bit revienne à la normale
#else
		waitForInterrupt(getPinWP(), 10000);//10000ms
		//wiringPiISR(int pin, int edgeType, void(*function)(void));
#endif
		return pkt;
	}
	if (!waitChange(askToStop)) {//on attend le premier bit
		if(askToStop && !(*askToStop))//si on a pas demandé le askToStop, ça viens d'autre chose
			std::cout << "[LaserNet_J/CAPTEUR] erreur aucun packet detecte" << std::endl;
		return pkt;
	}
	
	startSafe();//on est au premier bit (de maintenance)
	lastus += safeTime / 2;//5/10 du temps pour pas etre pile au début (etre un peu au milieu
	int64_t start = getCurrentus();

	while (!pkt.isComplete()) {//on récupère le packet
		if (askToStop && *askToStop)
			return pkt;
		int bitSkipped = waitToNextBit();
		bool state = getStateP();
		if (bitSkipped != 0) {
			std::cout << "retard dans le capteur: skipped " << bitSkipped << " bits at " << pkt.size() << std::endl;
		}
		while (bitSkipped > 0) {
			pkt << 0;//considéré comme éteint, ça fait moins de bugs (size)
			bitSkipped--;
		}
		pkt << state;
	}
	int64_t stop = getCurrentus();
	std::cout << "getPkt fini apres " << (stop-start) << " us et " << pkt.size() << " bits = " << ((stop-start)/pkt.size()) << " us/bit" << std::endl;
	if (ok) *ok = true;//il est complet
	return pkt;
}


LASER::LASER(int pinP) : SENSOR(true, pinP) { setState(0); }

void LASER::sendPkt(const packet::POut &pkt, const bool *askToStop)
{
	if (askToStop && *askToStop)//valide et qu'on a demandé de stop
		return;
	if (!isConnected())
		return;
	int64_t start = getCurrentus();
	startSafe();//init
	setStateP(1);//bit de départ

	for (size_t i = 0; i < pkt.size(); i++) {
		if (askToStop && *askToStop)
			return; 
		int bitSkipped = waitToNextBit();
		if(bitSkipped != 0) {
			i += bitSkipped;//si on a du retard
			if (i >= pkt.size())
				break;
		}
		setStateP(pkt.bitAt(i));
		if (bitSkipped != 0) {
			std::cout << "retard dans le laser: skipped " << bitSkipped << " bits at " << i << std::endl;
		}
	}
	int64_t stop = getCurrentus();
	std::cout << "sendPkt fini apres " << (stop-start) << " us et " << pkt.size() << " bits = " << ((stop-start)/pkt.size()) << " us/bit" << std::endl;
	waitToNextBit();
	setStateP(0);//sécuritée remetre à 0
	waitToNextBit();
}
void LASER::testLaser()
{
	if (!isConnected())
		return;
	startSafe();
	for (int i = 0; i < 4; i++)
	{
		setStateP(1);
		safePause(100000);//100 ms
		setStateP(0);
		safePause(400000);//400 ms
	}
}



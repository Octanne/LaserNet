#include "ByteArray.h"


using namespace std;
using namespace packet;



template<typename out>
out packet::groupUint(vector<uintByte> v)
{
	if (v.size() == 0) return 0;//pas besoin de calculs

	if (sizeof(out) < v.size() * sizeof(uintByte)) {//si le vecteur est trop grand pour out
		if (v.size() == 1) {
			if (pow(2, sizeof(out) >= v.at(0)))//mais si la valeure de v.at(0) pourrait finallement tenir dans out
				return out(v.at(0));//warning de perte mais c'est normal

		}
		else {//on a le droit de faire 'lerreur
			throw invalid_argument("Too much arg for this base");
			return 0;
		}
	}

	double valueMaxIn = pow(2, sizeof(uintByte) * 8);//8 bits par octet
	out retour = 0;
	for (int i = v.size() - 1; i >= 0; i--) {
		retour = (out)(
			(int)retour
			+ (int)v.at(i)
			*(int)pow(valueMaxIn, (double)v.size() - (double)i - 1.0)
			);
		//retour += v.at(i) * pow(valueMaxIn, v.size() - i - 1);
		//convertit la case i en base out
	}
	return retour;
}
template<typename in>
vector<uintByte> packet::repartUint(in v)
{
	vector<uintByte> retour(packet::BytesForType<in>(), 0);
	//on rempli de 0 pour que ça fasse le nombre de byte qui correspond
	if (v == 0) return retour;//pas besoin de calculs
	if (sizeof(v) <= sizeof(uintByte)) {
		retour[retour.size() - 1] = uintByte(v);
		return retour;
	}
	const in valueMaxOut = in(pow(2, sizeof(uintByte) * 8));

	for (size_t i = retour.size() - 1; v > 0; i--) {
		if (i < 0) {
			throw length_error("packet::repartUint length error");
			return vector<uintByte>(packet::BytesForType<in>(), 0);
		}
		retour[i] = uintByte(v % valueMaxOut);
		v /= valueMaxOut;
	}
	return retour;
}




//gestion de Byte
packet::Byte::Byte(uintByte v)
{
	d = Byte::UIntToByte(v).getByte();
}
packet::Byte::Byte(const vector<bool>::const_iterator& begin)
{
	d.clear();
	d.insert(d.begin(), begin, begin + packet::bits);
}
vector<bool>::reference packet::Byte::operator[](int i)
{
	if (i < 0) {
		throw out_of_range("negative position");
		return vector<bool>()[0];
	}
	if (i >= int(d.size())) {
		throw out_of_range("position too high");
		return vector<bool>()[0];
	}
	//si c'est dans le tableau
	return d[i];
}
bool packet::Byte::at(int i) const
{
	if (i < 0) {
		throw out_of_range("negative position");
		return false;
	}
	if (i >= int(d.size())) {
		throw out_of_range("position too high");
		return false;
	}
	//si c'est dans le tableau
	return d.at(i);
}
uintByte packet::Byte::getValue() const { return ByteToUInt(*this); }


uintByte packet::Byte::ByteToUInt(const packet::Byte& b)
{
	uintByte retour = 0;
	vector<bool> bits = b.getByte();
	for (int i = bits.size(); i > 0; i--) {//on parours des plus petits au plus grands
		if (bits.at(i - 1)) {//si c'est un 1
			int puiss = int(bits.size()) - i;
			retour = (uintByte)(retour + pow(2, puiss));//on ajoute la puissance de 2 qui correspond à la case
		}
	}
	return retour;
}
packet::Byte packet::Byte::UIntToByte(uintByte v)
{
	Byte retour;
	//uintByte vsave = v;//pour le debug
	for (int i = packet::bits; i > 0; i--)
	{
		retour[i - 1] = v % 2;//c'est 1 ou 0 pour vrai ou faux
		v /= 2;
	}
	return retour;
}





//gestion de ByteArray

bool packet::ByteArray::bitAt(size_t i) const
{
	if (i < 0) {
		throw out_of_range("negative position");
		return false;
	}
	if (i >= size()) {
		std::cout << "position too high at:" << i << " size:" << size() << std::endl;
		throw out_of_range("position too high");
		return false;
	}
	return d.at(i);
}

inline size_t packet::ByteArray::sizeSaved() const
{
	if (size() < sizeOfSize * bits)
		return size_t(sizeOfSize * bits);//minimum !
	return packet::groupUint<size_t>(bytesValueAt(0, sizeOfSize));
}

packet::Byte packet::ByteArray::byteAt(int posByte) const
{
	//on recupere le byte n°posByte
	if (posByte < 0) {
		throw out_of_range("negative position");
		return packet::Byte();
	}
	if (posByte >= nbOfByte()) {
		throw out_of_range("position too high");
		return packet::Byte();
	}
	return Byte(d.begin() + byteStart(posByte));
}
vector<Byte> packet::ByteArray::bytesAt(int posByte, int nbByte) const
{
	vector<Byte> retour;
	for (int i = 0; i < nbByte; i++)
		retour.push_back(byteAt(posByte + i));
	return retour;
}
vector<uintByte> packet::ByteArray::bytesValueAt(int posByte, int nbByte) const
{
	vector<uintByte> retour;
	vector<Byte> bytes = bytesAt(posByte, nbByte);
	for (int i = 0; i < int(bytes.size()); i++)
		retour.push_back(Byte::ByteToUInt(bytes.at(i)));
	return retour;
}






void packet::ByteArray::insertByte(const packet::Byte& b, int posByte)
{//on insere un byte
	if (posByte < 0) posByte = nbOfByte();//si négatif, on ajoute à la fin
	if (posByte > nbOfByte()) posByte = nbOfByte();//pareil

	int bStart = byteStart(posByte);
	vector<bool> bits = b.getByte();
	d.insert(d.begin() + bStart,
		bits.begin(), bits.end());
}
void packet::ByteArray::replaceByte(const packet::Byte &b, int posByte)
{
	if (posByte < 0) {
		throw out_of_range("edit negative pos");
		return;
	}
	if (posByte < nbOfByte())//si il existe on l'fface
		removeByte(posByte);
	insertByte(b, posByte);//et on rajoute le nouveau
}
void packet::ByteArray::removeByte(int posbyteStart, int nbBytes)
{
	if (posbyteStart < 0) {
		throw out_of_range("remove negative pos");
		return;
	}
	if (posbyteStart >= nbOfByte()) {
		throw out_of_range("remove too high pos");
		return;
	}
	if (nbBytes <= 0) {
		clog << "warning : remove negative number of bytes" << endl;
		return;
	}
	if (nbOfByte() <= posbyteStart + nbBytes - 1)//si on souhaite supprimer des byte inhexistants
		nbBytes = nbOfByte() - posbyteStart;//si 4 cases (0 à 3): start=2 donc de 2 à 3 : 4-start=2
	d.erase(d.begin() + byteStart(posbyteStart),
		d.begin() + byteEnd(posbyteStart + nbBytes - 1) + 1);
	//on efface (posbyteStart+nbBytes-1 est la position du dernier byte à suppr, erase a besoin de la case suivante)
}

void packet::ByteArray::insertBit(bool v, int posBit)
{
	if (posBit < 0)
		d.insert(d.end(), v);
	else
		d.insert(d.begin() + posBit, v);
}








packet::PIn::PIn() : ByteArray() {}
packet::PIn& packet::PIn::operator<<(bool bit)
{
	if (!isComplete())
		insertBit(bit);
	return *this;
}
bool packet::PIn::isComplete() const
{
	if (size() < sizeOfSize * packet::bits)
		return false;//si on a meme pas les cases pour la taille
	return sizeSaved() <= size();
}
template<typename T>
T PIn::nextValue()
{
	vector<uintByte> bytes;//récupère les bytes en fonction de la taille du type T
	for (int i = 0; i < packet::BytesForType<T>(); i++) {
		bytes.push_back(0);
		bytes[i] = giveNextByte().getValue();
	}
	//combine les bytes
	return packet::groupUint<T>(bytes);
}

Byte packet::PIn::giveNextByte()
{
	Byte b = byteAt(sizeOfSize);//on donne la valeur du byte 1
	removeByte(sizeOfSize);//on retire le byte 1
	return b;
}





packet::POut::POut() : ByteArray()
{
	//ByteArray::d = vector<bool>(packet::bits * packet::sizeOfSize, false);
	calcSize();
}


template<typename T>
POut & packet::POut::insertValue(POut & out, const T & t)
{
	vector<uintByte> t2 = packet::repartUint<T>(t);
	for (int i = 0; i < packet::BytesForType<T>(); i++)
		out.insertByte(Byte(t2.at(i)));
	return out;
}

void packet::POut::insertByte(const Byte & b, int posByte)
{
	if (posByte < 0 && nbOfByte() < sizeOfSize)//si la taille est pas la
		calcSize();//il y a peut etre eu un bug
	if (posByte < 0) posByte = nbOfByte();
	ByteArray::insertByte(b, posByte);
	if (posByte >= packet::sizeOfSize)
		calcSize();
}
void packet::POut::replaceByte(const Byte & b, int posByte)
{
	ByteArray::replaceByte(b, posByte);
	if (posByte >= packet::sizeOfSize)
		calcSize();
}
void packet::POut::removeByte(int posByteStart, int nbBytes)
{
	ByteArray::removeByte(posByteStart, nbBytes);
	if (posByteStart >= packet::sizeOfSize)
		calcSize();
}
void packet::POut::insertBytes(const vector<Byte>& b, int posByte)
{
	if (posByte < 0) posByte = nbOfByte();
	for (int i = 0; i < int(b.size()); i++)
		insertByte(b.at(i), i);//on insere au fur et à mesure
}
void packet::POut::replaceBytes(const vector<Byte>& b, int posByte)
{
	if(posByte < ByteArray::nbOfByte())
		removeByte(posByte, b.size());
	insertBytes(b, posByte);
}

void packet::POut::calcSize()
{
	//on recupere la taille sous plusieurs byte
	vector<uintByte> values = repartUint<size_t>(size());
	if (values.size() > sizeOfSize) {
		throw length_error("array is too long");
	}
	std::vector<Byte> bytes;
	for (int i = 0; i < sizeOfSize; i++)
		bytes.insert(bytes.begin() + i, values.at(i));
	replaceBytes(bytes, 0);
}






std::ostream &packet::operator<<(std::ostream &os, const packet::ByteArray &ba)
{
	std::string retour = "";
	
	//on parcours les bytes
	for (size_t i = 0; i < ba.size(); i++) {
		if (i % packet::bits == 0 && i != 0)//si on est au début d'un byte et que c'est pas le premier
			retour += ";";//on sépare
		retour += (ba.bitAt(i) ? "1" : "0");
	}
	os << "ByteArray["+retour+"]";
	return os;
}

PIn &packet::operator>>(PIn &in, uint8_t &t)
{
	t = in.nextValue<uint8_t>();
	return in;
}
PIn &packet::operator>>(PIn &in, uint16_t &t)
{
	t = in.nextValue<uint16_t>();
	return in;
}
PIn &packet::operator>>(PIn &in, uint32_t &t)
{
	t = in.nextValue<uint32_t>();
	return in;
}
PIn &packet::operator>>(PIn &in, uint64_t &t)
{
	t = in.nextValue<uint64_t>();
	return in;
}
PIn &packet::operator>>(PIn &in, time_t &t)
{
	t = in.nextValue<time_t>();
	return in;
}
PIn &packet::operator>>(PIn &in, char &t)
{
	t = in.nextValue<char>();
	return in;
}
PIn &packet::operator>>(PIn &in, string &t)
{
	size_t size = 0;
	in >> size;
	if (in.size() < size) {
		std::cout << "ByteArray not complete or bad argument" << std::endl;
		size = in.size();
	}
	//le str est plus grand que le packet (car le packet s'est fait couper)
	t = string(" ", size);
	for (size_t i = 0; i < t.length(); i++)
		in >> t[i];//ajoute les charactères
	return in;
}
PIn &packet::operator>>(PIn &in, std::vector<uint8_t> &t)
{
	size_t size = 0;
	in >> size;
	t = std::vector<uint8_t>(size);
	for (size_t i = 0; i < t.size(); i++)
		in >> t[i];
	return in;
}

POut &packet::operator<<(POut &out, const uint8_t &t)
{
	return packet::POut::insertValue<uint8_t>(out, t);
}
POut &packet::operator<<(POut &out, const uint16_t &t)
{
	return packet::POut::insertValue<uint16_t>(out, t);
}
POut & packet::operator<<(POut & out, const uint32_t & t)
{
	return packet::POut::insertValue<uint32_t>(out, t);
}
POut & packet::operator<<(POut & out, const uint64_t & t)
{
	return packet::POut::insertValue<uint64_t>(out, t);
}
POut & packet::operator<<(POut & out, const time_t & t)
{
	return packet::POut::insertValue<time_t>(out, t);
}
POut & packet::operator<<(POut & out, const char & t)
{
	return packet::POut::insertValue<char>(out, t);
}
POut & packet::operator<<(POut & out, const string & t)
{
	out << t.length();
	for (size_t i = 0; i < t.length(); i++)
		out << t.at(i);
	return out;
}
POut & packet::operator<<(POut & out, const std::vector<uint8_t> & t)
{
	out << t.size();
	for (size_t i = 0; i < t.size(); i++)
		out << t.at(i);
	return out;
}
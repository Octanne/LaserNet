#ifndef PACKET_H
#define PACKET_H
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>//pow



namespace packet {
	using namespace packet;
	using namespace std;


	//constantes et fonctions de conversions d'un int en plusieurs uintByte et inversement
	typedef uint8_t uintByte;//un Byte correspond à un nombre de type uintByte
							//typedef customs : https://openclassrooms.com/forum/sujet/interet-de-typedef-40306#message-4229703
	const int bits = sizeof(uintByte) * 8;//bits in a byte
	const int sizeOfSize = sizeof(size_t) / sizeof(uintByte);//nb de byte utilisés pour la size dans la ByteArray

	//regroupe/degroupe un nombre (out ou in) et plusieurs sous nombres (uintByte)
	template <typename out>
	out groupUint(vector<uintByte> v);//groupe de uintByte en un 'out' (des nombres)
		//4 uintByte avec out de type int donneront un seul int ayant la valeur selont les uintByte (system de base 'uintByte')
	template <typename in>
	vector<uintByte> repartUint(in v);//un 'in' en groupe de uintByte (des nombres)
		//un in de type int pourra donner 4 uintByte ayant la valeur de in répartit dans chacuns (system de bases 'uintByte')

	//nombre de bytes du type T pour le ByteArray
	template <typename T>
	int BytesForType() {
		return (int)ceil((double)sizeof(T) / (double)sizeof(uintByte));
		//uintByte est normalement un uint8_t donc il a une size de 1 (octet)
		//si on a un T de type int, il fait 4 octets, il faudra donc 4 cases pour l'enregistrer
	};




	//calsse générallement temporaire (pas enregistrée quelque part) qui gère plusieurs bits en même temps
	//contient 'packet::bits' bits
	class Byte
	{
	public:
		//les différentes créations (utilisées par ByteArray)
		inline Byte() { d = vector<bool>(packet::bits); };
		Byte(uintByte v);
		Byte(const vector<bool>::const_iterator& begin);
		inline Byte(const Byte &b) { d = b.d; };

		vector<bool>::reference operator[](int i);//un bit (en reference)
		bool at(int i) const;//un bit
		inline vector<bool> getByte() const { return d; };//le byte
		uintByte getValue() const;//le byte en nombre

		static uintByte ByteToUInt(const Byte& b);//byte vers un nombre
		static Byte UIntToByte(uintByte v);//nombre vers un byte
	private:
		vector<bool> d;
	};


	//ByteArray, pour tout stocker sous bits
	//format : [dataSize1;dataSize2;dataSize3;dataSize4;data1;data2;...] (size prend 4 bytes car BytesForType<size_t>()=4)
	//chaques cases sont des Byte (de packet::Byte) enregistrés en bits.
	//ces Byte ont une valeur maximale pouvant etre stockée dans un uintByte
	class ByteArray
	{
		public:
			//gestion autorisées pour des trucs de type debug
			inline size_t size() const { return d.size(); };
			bool bitAt(size_t i) const;

		protected:
			//copie et création d'un byte array
			inline ByteArray() { d = vector<bool>(0); };
			inline ByteArray(const ByteArray &ba) { d = ba.d; };
			inline ByteArray &operator=(const ByteArray &b) { d = b.d; return *this; }


			//inutiles pour un utilisateur lambda, surtout pour les classes POut et PIn
			inline bool isEmpty() const { return size() < packet::bits; };//il y a moins d'un packet
			size_t sizeSaved() const;//taille enregistrée dans les premieres cases du packet pour annoncer la taille complete du packet
			inline int nbOfByte() const { return size() / packet::bits; }//nombre de bytes au total dans l'array (on arrondit à l'entier inférieur)

			Byte byteAt(int posByte) const;
			vector<Byte> bytesAt(int posByte, int nbByte) const;
			vector<uintByte> bytesValueAt(int posByte, int nbByte) const;

			void insertByte(const Byte& b, int posByte = -1);
			void replaceByte(const Byte &b, int posByte);
			void removeByte(int posbyteStart, int nbBytes = 1);
			void insertBit(bool v, int posBit=-1);//pas de recalcul de size parce que c'est reception du packet...

			inline static int byteStart(int posbyte) { return packet::bits * posbyte; };//le début (position du vector) du byte
			inline static int byteEnd(int posbyte) { return byteStart(posbyte + 1) - 1; };//la fin (position du vector) du byte
			inline static int getposByteAt(int posBits) { return posBits / packet::bits; }//le byte à la position du vector
		private:
			vector<bool> d;
	};




	//PIn pour packet in : convertit un stream de bits en packet
	class PIn : public ByteArray
	{
		public:
			PIn();
			PIn& operator<<(bool bit);//stream de bit (inserer manuellement ou avec une boucle)
			bool isComplete() const;//une fois qu'il est complet, on peut le vider

			//utile pour les operator>> :
			template<typename T>//nombre comme int, uint16_t, ou size_t...
			T nextValue();//retourne la valeur dans t avec tous les bytes en fonction de la taille de T 
		protected:
			//inutile pour l'utilisateur lambda
			Byte giveNextByte();//retourne le prochain byte du packet
	};
	//POut pour packet out : convertit des objets en groupe de bits
	class POut : public ByteArray
	{
		public:
			POut();
			template<typename T>//nombre comme int, uint16_t, ou size_t...
			static POut& insertValue(POut& out, const T &t);//insert la valeur t dans des bytes en fonciton de la taille de T
		private:
			void insertByte(const Byte& b, int posByte = -1);//ajoute un byte dans le packet (-1 pour ajouter à la fin)
			void replaceByte(const Byte &b, int posByte);//remplace un byte dans le packet
			void removeByte(int posByteStart, int nbBytes = 1);//retire un/plusieurs byte dans le packet
			void insertBytes(const vector<Byte>& b, int posByte = -1);//ajoute plusieurs bytes
			void replaceBytes(const vector<Byte>& b, int posByte);//remplace plusieurs bytes
			void calcSize();//calcul la taille au début du packet pour annoncer la taille du packet
	};

	//fonctions d'input et output
	std::ostream &operator<<(std::ostream &os, const packet::ByteArray &ba);
	PIn& operator>>(PIn &ba, uint8_t &t);
	PIn& operator>>(PIn &ba, uint16_t &t);
	PIn& operator>>(PIn &ba, uint32_t &t);
	PIn& operator>>(PIn &ba, uint64_t &t);
	PIn& operator>>(PIn &ba, time_t &t);
	PIn& operator>>(PIn &ba, char &t);
	PIn& operator>>(PIn &ba, string &t);
	PIn& operator>>(PIn &ba, std::vector<uint8_t> &t);
	POut& operator<<(POut& out, const uint8_t &t);
	POut& operator<<(POut& out, const uint16_t &t);
	POut& operator<<(POut& out, const uint32_t &t);
	POut& operator<<(POut& out, const uint64_t &t);
	POut& operator<<(POut& out, const time_t &t);
	POut& operator<<(POut& out, const char &t);
	POut& operator<<(POut& out, const string &t);
	POut& operator<<(POut& out, const std::vector<uint8_t> &t);
	
}

#endif //PACKET_H
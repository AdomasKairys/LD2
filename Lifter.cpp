#include "Lifter.h"
#include "SHA256.h"

using namespace std;

Lifter::Lifter(string name, int weightClass, double total)
{
	this->name = name;
	this->weightClass = weightClass;
	this->total = total;
	this->hash = "";
}

//https://github.com/System-Glitch/SHA256
void Lifter::generateHash()
{
    string str = name + to_string(weightClass) + to_string(total);

	SHA256 sha;
	sha.update(str);
	uint8_t* digest = sha.digest();

	this->hash = SHA256::toString(digest).substr(0,10);

	delete[] digest;
}


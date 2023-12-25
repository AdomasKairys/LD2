#include "Lifter.h"
#include "json.hpp"
#include "SHA256.h"

using namespace std;
using namespace nlohmann;

Lifter::Lifter()
{
	this->name = "";
	this->weightClass = 0;
	this->total = 0;
	this->hash = "";
}

Lifter::Lifter(string name, int weightClass, double total)
{
	this->name = name;
	this->weightClass = weightClass;
	this->total = total;
	this->hash = "";
}
string Lifter::to_json()
{
	json j;
	j["name"] = name;
	j["weightClass"] = weightClass;
	j["total"] = total;
	j["hash"] = hash;
	return j.dump();
}
Lifter Lifter::from_json(std::string json)
{
	auto lifterJson = json::parse(json);
	this->name = lifterJson["name"].get<string>();
	this->weightClass = lifterJson["weightClass"].get<int>();
	this->total = lifterJson["total"].get<double>();
	this->hash = lifterJson["hash"].get<string>();
	return *this;
}

//https://github.com/System-Glitch/SHA256
void Lifter::generate_hash()
{
	string str = name + to_string(weightClass) + to_string(total);
	cout << str << endl;
	SHA256 sha;
	sha.update(str);
	uint8_t* digest = sha.digest();

	this->hash = SHA256::toString(digest).substr(0,10);

	delete[] digest;
}




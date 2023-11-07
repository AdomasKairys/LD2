#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

class Lifter
{
public:
	std::string name;
	int weightClass;
	double total;
	std::string hash;
	Lifter();
	Lifter(std::string name, int weightClass, double total);
	std::string to_json();
	Lifter from_json(std::string json);
	void generate_hash();
};


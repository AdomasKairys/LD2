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
};


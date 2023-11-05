#include <iostream>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Lifter.h"
#include "json.hpp"
#include "SHA256.h"

using json = nlohmann::json; 
using namespace std;

const size_t DATA_ARRAY_SIZE = 10;
const size_t ROOT_PROCESS = 0;
const size_t DATA_PROCESS = 1;
const size_t RESULT_PROCESS = 2;
const size_t TAG_DATA = 0;
const size_t TAG_REQUEST = 1;
const size_t TAG_DATA_REMAINING = 2;

vector<Lifter> read_lifters_from_file(const string& filePath);
string generateHash(string stringField, int intField, double doubleField);

vector<Lifter> read_lifters_from_file(const string& file_path) {
    vector<Lifter> Lifters;
    ifstream stream(file_path);
    json all_lifters_json = json::parse(stream);
    auto all_lifters = all_lifters_json["wlifter"];

    for (const json& new_Lifter : all_lifters) 
        Lifters.push_back({ new_Lifter["name"],new_Lifter["weightClass"],new_Lifter["total"]});
    
    return Lifters;
}
//https://github.com/System-Glitch/SHA256
string generateHash(string str)
{
	SHA256 sha;
	sha.update(str);
	uint8_t* digest = sha.digest();

	string hash = SHA256::toString(digest).substr(0,10);

	delete[] digest;

    return hash;
}
int main(){

    MPI::Init();
    
    auto rank = MPI::COMM_WORLD.Get_rank();
    const size_t totalProcesses = MPI::COMM_WORLD.Get_size();
    
    if (totalProcesses < 4){
        if (rank == 0)
            cerr << "need to have more than 4 proc" << endl;
        MPI::COMM_WORLD.Abort(1);
    }

    
    switch (rank)
    {
        case ROOT_PROCESS:
        {
            vector<Lifter> lifters = read_lifters_from_file("IF11_KairysA_LD1_dat2.json");
            size_t N = lifters.size();  
            for(size_t i = 0; i < N; i++)
            {
                string data = lifters[i].name + to_string(lifters[i].weightClass) + to_string(lifters[i].total);
                MPI::COMM_WORLD.Send(data.c_str(), data.size(), MPI_CHAR, DATA_PROCESS, TAG_DATA);
                size_t remaining = N-1-i;
                MPI::COMM_WORLD.Send(&remaining, 1, MPI_INT, DATA_PROCESS, TAG_DATA_REMAINING);
            }
            break;
        }
        case DATA_PROCESS:
        {
            string processArray[DATA_ARRAY_SIZE];
            bool isFinished = false;
            bool isArrayEmpty = true;
            while (!isFinished || !isArrayEmpty)
            {
                for(size_t i = 0; i<sizeof(processArray); i++)
                {
                    if(processArray[i] != "")
                        continue;
                    
                    MPI::Status status;
                    MPI::COMM_WORLD.Probe(ROOT_PROCESS, TAG_DATA, status);

                    char buffer[status.Get_count(MPI_CHAR)];
                    MPI::COMM_WORLD.Recv(&buffer, sizeof(buffer), MPI_CHAR, status.Get_source(), status.Get_tag());

                    string data(buffer);

                    processArray[i] = data;
                }

                MPI::Status status;
                MPI::COMM_WORLD.Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, TAG_REQUEST, status);

                size_t remaining;
                MPI::COMM_WORLD.Recv(&remaining, 1, MPI_INT, DATA_PROCESS, TAG_DATA_REMAINING);
                isFinished = remaining == 0;

                size_t index = 0;
                isArrayEmpty = true;

                for(size_t i = 0; i<sizeof(processArray); i++)
                {
                    if(processArray[i] == "")
                        continue;
                    isArrayEmpty = false;
                    index = i;
                    break;
                }
                int response = isArrayEmpty && isFinished ? -1 : (isArrayEmpty ? 0 : 1); // -1 means finished and no more data, 0 means no more data currently, 1 mean data is available


                MPI::COMM_WORLD.Send(&response, 1, MPI_INT, status.Get_source(), status.Get_tag());

                if(response == 1)
                {
                    const char* data = processArray[index].c_str();
                    MPI::COMM_WORLD.Send(data, strlen(data), MPI_CHAR, status.Get_source(), status.Get_tag());
                }

            }
        }
            break;
        case RESULT_PROCESS:
            break;
        default: //worker proccess
            break;
    }
    MPI::Finalize();
    return 0;
}
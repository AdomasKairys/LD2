#include <iostream>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Lifter.h"
#include "json.hpp"

using json = nlohmann::json; 
using namespace std;

const int ROOT_PROCESS = 0;
const int DATA_PROCESS = 1;
const int RESULT_PROCESS = 2;
const int TAG_WEIGHT_CLASS = 0;
const int TAG_TOTAL = 1;
const int TAG_NAME = 2;

vector<Lifter> read_lifters_from_file(const string& filePath);
void data_proccess_task(Lifter (&proccessArray)[],  size_t* startIndex, size_t endIndex, const size_t totalProcesses, const size_t arraySize );

vector<Lifter> read_lifters_from_file(const string& file_path) {
    vector<Lifter> Lifters;
    ifstream stream(file_path);
    json all_lifters_json = json::parse(stream);
    auto all_lifters = all_lifters_json["wlifter"];

    for (const json& new_Lifter : all_lifters) 
        Lifters.push_back({ new_Lifter["name"],new_Lifter["weightClass"],new_Lifter["total"]});
    
    return Lifters;
}

void data_proccess_task(Lifter (&proccessArray)[], size_t* startIndex, size_t endIndex, const size_t totalProcesses, const size_t arraySize)
{
    size_t numOfElements = 0;
    size_t numOfRemovedElements = 0;
    if(endIndex == *startIndex)
        return;

    numOfElements = arraySize-*startIndex;

    if(endIndex < *startIndex)
        numOfElements += endIndex;

    if(numOfElements < (totalProcesses -3))
        numOfRemovedElements = numOfElements;
    else
        numOfRemovedElements = totalProcesses - 3;

    *startIndex = (*startIndex + numOfRemovedElements) % arraySize;
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

    vector<Lifter> lifters;
    size_t N;
    if (rank == ROOT_PROCESS){
        lifters = read_lifters_from_file("IF11_KairysA_LD1_dat2.json");
        size_t N = lifters.size();
    }
    MPI::COMM_WORLD.Bcast(&N, 1, MPI_INT, ROOT_PROCESS);
    switch (rank)
    {
        case ROOT_PROCESS:
        {
            
            break;
        }
        case DATA_PROCESS:
        {
            size_t startIndex = 0;
            size_t endIndex = 0;
            Lifter processArray[N/2];
            while (true)
            {
                if(processArray[endIndex].isEmpty)
                {
                    int weightClass;
                    double total;

                    MPI::Status status;
                    MPI::COMM_WORLD.Probe(ROOT_PROCESS, TAG_NAME, status);

                    char buffer[status.Get_count(MPI_CHAR)];

                    MPI::COMM_WORLD.Recv(&weightClass, 1, MPI_INT, ROOT_PROCESS, TAG_WEIGHT_CLASS );
                    MPI::COMM_WORLD.Recv(&total, 1, MPI_DOUBLE, ROOT_PROCESS, TAG_TOTAL );
                    MPI::COMM_WORLD.Recv(&buffer, sizeof(buffer), MPI_CHAR, status.Get_source(), status.Get_tag());

                    string name(buffer);

                    processArray[endIndex] = Lifter(name, weightClass, total);

                    size_t _endIndex = (endIndex+1)%sizeof(processArray);
                    if(_endIndex != startIndex)
                        endIndex = _endIndex;
                }

                data_proccess_task(processArray, &startIndex, endIndex, totalProcesses, sizeof(processArray));


                break;

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
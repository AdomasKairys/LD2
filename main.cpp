#include <iostream> 
#include <vector>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Lifter.h"
#include "json.hpp"

using namespace nlohmann; 

const size_t DATA_ARRAY_SIZE = 10;
const size_t ROOT_PROCESS = 0;
const size_t DATA_PROCESS = 1;
const size_t RESULT_PROCESS = 2;
const size_t TAG_DATA = 0;
const size_t TAG_DATA_AMMOUNT = 1;
const size_t TAG_STATUS = 2;
const size_t TAG_STATUS_TOTAL = 3;
const size_t TAG_REQUEST = 4;

std::vector<Lifter> read_lifters_from_file(const std::string& filePath);
void sortedAdd(std::vector<Lifter> *lifters, Lifter lifter);

std::vector<Lifter> read_lifters_from_file(const std::string& file_path) {
    std::vector<Lifter> Lifters;
    std::ifstream stream(file_path);
    json all_lifters_json = json::parse(stream);
    auto all_lifters = all_lifters_json["wlifter"];
    
    for (const json& new_Lifter : all_lifters) 
        Lifters.push_back({ new_Lifter["name"],new_Lifter["weightClass"],new_Lifter["total"]});
    
    return Lifters;
}

void sortedAdd(std::vector<Lifter> *lifters, Lifter lifter)
{
    lifters->push_back(lifter);
    for (size_t i = lifters->size() - 1; i >= 1; i--) {
        if (((*lifters)[i].weightClass < (*lifters)[i - 1].weightClass) || 
            (((*lifters)[i].weightClass == (*lifters)[i - 1].weightClass) && (*lifters)[i].total < (*lifters)[i - 1].total)) {
            break;
        }
        std::swap((*lifters)[i], (*lifters)[i - 1]);
    }
}


int main(){

    MPI::Init();
    
    auto rank = MPI::COMM_WORLD.Get_rank();
    const size_t totalProcesses = MPI::COMM_WORLD.Get_size();
    if (totalProcesses < 4){
        if (rank == 0)
            std::cerr << "need to have more than 4 proc" << std::endl;
        MPI::COMM_WORLD.Abort(1);
    }

    switch (rank)
    {
        case ROOT_PROCESS:
        {
            std::cout << "number of proc " << totalProcesses << std::endl;
            std::vector<Lifter> lifters = read_lifters_from_file("IF11_KairysA_LD1_dat2.json");
            size_t N = lifters.size();  
            for(size_t i = 0; i < N; i++)
            {
                std::string data = lifters[i].to_json();
                MPI::COMM_WORLD.Send(data.c_str(), data.size(), MPI_CHAR, DATA_PROCESS, TAG_DATA);
                int remaining = N-1-i;
                MPI::COMM_WORLD.Send(&remaining, 1, MPI_INT, DATA_PROCESS, TAG_DATA_AMMOUNT);
            }
            int incomingDataSize;
            MPI::COMM_WORLD.Recv(&incomingDataSize, 1, MPI_INT, RESULT_PROCESS, TAG_DATA_AMMOUNT);
            std::ofstream outputFile("IF11_KairysA_LD1_res.txt");
            for(size_t i = 0; i < incomingDataSize; i++)
            {
                MPI::Status status;
                MPI::COMM_WORLD.Probe(RESULT_PROCESS, TAG_DATA, status);
                char buffer[status.Get_count(MPI_CHAR)];
                MPI::COMM_WORLD.Recv(buffer, status.Get_count(MPI_CHAR), MPI_CHAR, RESULT_PROCESS, TAG_DATA);
                std::string results(buffer, status.Get_count(MPI_CHAR));
                Lifter lifter;
                lifter.from_json(results);
                outputFile << "| " << std::left << std::setw(20) << lifter.name << " |" << std::left << std::setw(5) << lifter.weightClass << " |" << std::left << std::setw(6) << lifter.total << " |" << std::left << std::setw(10) << lifter.hash + " |" << std::endl;
            }
            outputFile<<"Number of results: " << incomingDataSize << std::endl;
            outputFile.close();
            break;
        }
        case DATA_PROCESS:
        {
            std::string processArray[DATA_ARRAY_SIZE];
            size_t index = 0;
            int remaining = 10; // can be any arbitrary number greater than 0
            while ((remaining + index) > 0)
            {
                if(index < DATA_ARRAY_SIZE && remaining > 0)
                {
                    MPI::Status status;
                    MPI::COMM_WORLD.Probe(ROOT_PROCESS, TAG_DATA, status);
                    char buffer[status.Get_count(MPI_CHAR)];
                    MPI::COMM_WORLD.Recv(buffer, status.Get_count(MPI_CHAR), MPI_CHAR, status.Get_source(), status.Get_tag());
                    std::string data(buffer, status.Get_count(MPI_CHAR));
                    processArray[index++] = data;
                    MPI::COMM_WORLD.Recv(&remaining, 1, MPI_INT, ROOT_PROCESS, TAG_DATA_AMMOUNT);
                }
                MPI::Status status;
                bool isRequested = MPI::COMM_WORLD.Iprobe(MPI_ANY_SOURCE, TAG_REQUEST, status);
                if(isRequested){
                    MPI::COMM_WORLD.Recv(NULL, 0, MPI_INT, status.Get_source(), TAG_REQUEST);
                    bool isNotEmpty = index != 0; //0 means no more data currently, 1 mean data is available
                    MPI::COMM_WORLD.Send(&isNotEmpty, 1, MPI_CXX_BOOL, status.Get_source(), TAG_STATUS);
                    bool isFinished = (isNotEmpty && (index + remaining - 1) >= (totalProcesses - 3)) || (!isNotEmpty && (index + remaining) >= (totalProcesses - 3)) ; //checks if worker should continue to wait for data
                    MPI::COMM_WORLD.Send(&isFinished, 1, MPI_CXX_BOOL, status.Get_source(), TAG_STATUS_TOTAL);
                    if(isNotEmpty)
                    {
                        const char* data = processArray[--index].c_str();
                        MPI::COMM_WORLD.Send(data, strlen(data), MPI_CHAR, status.Get_source(), TAG_DATA);
                    }
                }
            }
            break;
        }
        case RESULT_PROCESS:
        {
            size_t workingProc = totalProcesses - 3;
            std::vector<Lifter> outputLifters;
            while(workingProc > 0){
                bool isNotEmpty;
                MPI::Status status;
                MPI::COMM_WORLD.Probe(MPI_ANY_SOURCE, TAG_STATUS , status);
                MPI::COMM_WORLD.Recv(&isNotEmpty, 1 , MPI_CXX_BOOL, status.Get_source(), TAG_STATUS);

                if(isNotEmpty)
                {
                    MPI::COMM_WORLD.Probe(MPI_ANY_SOURCE, TAG_DATA , status);
                    char buffer[status.Get_count(MPI_CHAR)];
                    MPI::COMM_WORLD.Recv(buffer, status.Get_count(MPI_CHAR), MPI_CHAR, status.Get_source(), status.Get_tag());
                    std::string result(buffer, status.Get_count(MPI_CHAR));
                    Lifter lifter;
                    lifter.from_json(result);
                    sortedAdd(&outputLifters, lifter);
                }
                MPI::COMM_WORLD.Probe(MPI_ANY_SOURCE, TAG_STATUS_TOTAL, status);
                MPI::COMM_WORLD.Recv(&isNotEmpty, 1, MPI_CXX_BOOL, status.Get_source(), TAG_STATUS_TOTAL);
                if(!isNotEmpty)
                {
                    workingProc--;
                    continue;
                }
            }
            int dataSize = outputLifters.size();
            MPI::COMM_WORLD.Send(&dataSize, 1, MPI_INT, ROOT_PROCESS, TAG_DATA_AMMOUNT);
            if(outputLifters.empty())
                break;
            
            for(auto lifter : outputLifters)
            {
                std::string returnVal = lifter.to_json();
                MPI::COMM_WORLD.Send(returnVal.c_str(), returnVal.size(), MPI_CHAR, ROOT_PROCESS, TAG_DATA);
            }

            break;
        }
        default: //worker proccess
        {
            while(true)
            {
                bool isNotEmpty;
                bool isFinished;
                MPI::COMM_WORLD.Send(NULL, 0, MPI_INT, DATA_PROCESS, TAG_REQUEST);
                MPI::COMM_WORLD.Recv(&isNotEmpty, 1, MPI_CXX_BOOL, DATA_PROCESS, TAG_STATUS);
                MPI::COMM_WORLD.Recv(&isFinished, 1, MPI_CXX_BOOL, DATA_PROCESS, TAG_STATUS_TOTAL);
                if(isNotEmpty)
                {
                    MPI::Status status;
                    MPI::COMM_WORLD.Probe(DATA_PROCESS, TAG_DATA , status);
                    char buffer[status.Get_count(MPI_CHAR)];
                    MPI::COMM_WORLD.Recv(buffer, status.Get_count(MPI_CHAR), MPI_CHAR, status.Get_source(), status.Get_tag());
                    std::string data(buffer, status.Get_count(MPI_CHAR));
                    Lifter lifter;
                    lifter.from_json(data);
                    lifter.generate_hash();
                    
                    if(!isdigit(lifter.hash[0]))
                    {
                        MPI::COMM_WORLD.Send(&isNotEmpty, 1, MPI_CXX_BOOL, RESULT_PROCESS, TAG_STATUS);
                        std::string result = lifter.to_json();
                        MPI::COMM_WORLD.Send(result.c_str(), result.size(), MPI_CHAR, RESULT_PROCESS, TAG_DATA);
                    }
                    else
                    {
                        isNotEmpty = false;
                        MPI::COMM_WORLD.Send(&isNotEmpty, 1, MPI_CXX_BOOL, RESULT_PROCESS, TAG_STATUS);
                    }
                }
                else
                {
                    MPI::COMM_WORLD.Send(&isNotEmpty, 1, MPI_CXX_BOOL, RESULT_PROCESS, TAG_STATUS);
                }
                
                
                
                MPI::COMM_WORLD.Send(&isFinished, 1 , MPI_CXX_BOOL, RESULT_PROCESS, TAG_STATUS_TOTAL);
                if(!isFinished)
                    break;
            }
            break;
        }
    }
    MPI::Finalize();
    return 0;
}
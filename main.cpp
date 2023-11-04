#include <iostream>
#include <mpi.h>
const int ROOT_PROCCESS = 0;
const int DATA_PROCCESS = 1;
const int RESULT_PROCCESS = 2;
int main(){
    MPI::Init();
    auto rank = MPI::COMM_WORLD.Get_rank();
    auto totalProcesses = MPI::COMM_WORLD.Get_size();
    if (totalProcesses < 3){
        std::cout << "need to have more than 3 proc" << std::endl;
        return -1;
    }
    switch (rank)
    {
        case ROOT_PROCCESS:
            break;
        case DATA_PROCCESS:
            break;
        case RESULT_PROCCESS:
            break;
        default: //worker proccess
            break;
    }
    MPI::Finalize();
    return 0;
}
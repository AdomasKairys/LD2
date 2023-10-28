#include <iostream>
#include <mpi.h>

int main(){
    MPI::Init();
    auto rank = MPI::COMM_WORLD.Get_rank();
    auto totalProcesses = MPI::COMM_WORLD.Get_size();
    if(rank == 0){
        std::cout << "Proccess count " << totalProcesses << std::endl;
        char name[MPI::MAX_PROCESSOR_NAME];
        int name_length = 0;
        MPI::Get_processor_name(name, name_length);
        std::cout << "Processor name " << name << std::endl;
        int sent_message = 0, received_message = 0;
        MPI::COMM_WORLD.Send(&sent_message, 1, MPI::INT, 1, 1);
        std::cout << "Sent message " << sent_message << std::endl;
        MPI::COMM_WORLD.Recv(&received_message, 1, MPI::INT, 1, 1);
        std::cout << "Received message " << received_message << std::endl;
    } else {
        int sent_message = 1, received_message = 1;
        MPI::COMM_WORLD.Recv(&received_message, 1, MPI::INT, 0, 1);
        std::cout << "Received message " << received_message << std::endl;
        MPI::COMM_WORLD.Send(&sent_message, 1, MPI::INT, 0, 1);
        std::cout << "Sent message " << sent_message << std::endl;
    }
    MPI::Finalize();
    return 0;
}
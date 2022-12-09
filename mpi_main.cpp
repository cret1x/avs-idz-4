#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <time.h>

using namespace std;

struct talker_t {
    int to;
    int from;
    int status;
};

int main (int argc, char *argv[]) {
    int world_rank, world_size;
    int tag = 42;
    int control_tag = 1337;
    MPI_Status status;
    // Init MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size (MPI_COMM_WORLD, &world_size);
    MPI_Barrier( MPI_COMM_WORLD );
    bool run;
    // Create custon type for talker_t
    const int nitems=3;
    int          blocklengths[3] = {1,1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_talker_t;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(talker_t, to);
    offsets[1] = offsetof(talker_t, from);
    offsets[2] = offsetof(talker_t, status);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_talker_t);
    MPI_Type_commit(&mpi_talker_t);
    // 0 thread is main thread, share talkers with other threads
    if (world_rank == 0) {
        printf ("[thread] I am main thread\n");
        printf ("[info] Press 'q' to exit\n");
        vector<talker_t> talkers = vector<talker_t>(world_size - 1);
        for (int i = 0; i < world_size - 1; i++) {
            talkers[i] = talker_t {
                -1, -1, 0
            };
            MPI_Send(&talkers[i], 1, mpi_talker_t, i + 1, tag, MPI_COMM_WORLD);
        }
    }
    // sync here
    MPI_Barrier( MPI_COMM_WORLD );
    talker_t my_talker;
    // for each other thread recive talker
    if (world_rank > 0) {
        run = true;
        MPI_Recv(&my_talker,  1, mpi_talker_t, 0, tag, MPI_COMM_WORLD, &status);
        printf ("[thread] #%d Reciever talker: {%d %d %d}\n", world_rank, my_talker.to, my_talker.from, my_talker.status);
    }
    // sync here
    MPI_Barrier( MPI_COMM_WORLD );
    // sender thread section
    if (world_rank > 0 && world_rank < (world_size - 1) / 2 + 1) {
        printf ("[thread] I am sender thread #%d\n", world_rank);

        while(run) {
            int thread_count = (world_size-1)/2;
            srand(time(nullptr));
            int index = rand() % (world_size - thread_count - 1) + 1 + thread_count;
            while (index == world_rank) {
                printf ("[#%d] trying index %d\n", world_rank, index);
                index = rand() % (world_size - (world_size-1)/2) + (world_size-1) / 2;
            }
            if (my_talker.status == 0) {
                talker_t send {
                index, world_rank, 1
                };
                // sending message to index thread
                MPI_Send(&send, 1, mpi_talker_t, index, tag, MPI_COMM_WORLD);
                bool confirm = false;
                // if success, change data
                MPI_Recv(&confirm,  1, MPI_C_BOOL, index, tag, MPI_COMM_WORLD, &status);
                if (confirm) {
                    my_talker.to = index;
                    my_talker.from = world_rank;
                    my_talker.status = 3;
                    talker_t recv;
                    // when finished the call, change data
                    MPI_Recv(&recv,  1, mpi_talker_t, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
                    my_talker.to = -1;
                    my_talker.from = -1;
                    my_talker.status = 0;
                }
            }
            
            sleep(rand() % 5);
        }
    } else if (world_rank > 0) {
        printf ("[thread] I am reciever thread #%d\n", world_rank);
        while(run) {
            talker_t recv;
            // listen for any call
            MPI_Recv(&recv,  1, mpi_talker_t, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
            printf ("[recv] #%d from %d: {%d %d %d}\n", world_rank, status.MPI_SOURCE, recv.to, recv.from, recv.status);
            if (my_talker.status == 0) {
                bool confirm = true;
                // if can accept call, send data
                MPI_Send(&confirm, 1, MPI_C_BOOL, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
                my_talker.to = world_rank;
                my_talker.from = status.MPI_SOURCE;
                my_talker.status = 3;
                printf ("[start] %d->%d\n", status.MPI_SOURCE, world_rank);
                sleep(rand() % 5);
                my_talker.to = -1;
                my_talker.from = -1;
                my_talker.status = 0;
                printf ("[end] %d->%d\n", status.MPI_SOURCE, world_rank);
                talker_t send {
                    -1, -1, 0
                };
                // send end data
                MPI_Send(&send, 1, mpi_talker_t, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
            } else {
                bool confirm = false;
                MPI_Send(&confirm, 1, MPI_C_BOOL, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
            }
            sleep(rand() % 5);
        }
        
        my_talker.to = world_rank;
        my_talker.from = status.MPI_SOURCE;
        my_talker.status = 3;
    }
    
    if (world_rank == 0) {
        // monitor for q press to exit the program
        while(cin.get() != 'q');
        MPI_Abort(MPI_COMM_WORLD, 0);
        run = false;
        MPI_Bcast (&run, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
        for (int i = 1; i < world_size; i++) {
            talker_t send {
                0, 0, -2
            };
            MPI_Send(&send, 1, mpi_talker_t, i, tag, MPI_COMM_WORLD);
        }
    }
    // cleanup
    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Type_free(&mpi_talker_t);
    MPI_Finalize();
    return 0;
}

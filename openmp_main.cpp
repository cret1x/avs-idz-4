// c++ main.cpp -fopenmp
#include <omp.h>

#include <iostream>

#include <vector>

#include <unistd.h>


using namespace std;

struct talker_t {
    int to;
    int from;
    int status;
};

int main() {
    int n;
    bool run = true;
    cout << "Input number of talkers: ";
    cin >> n;
    vector < talker_t > talkers = vector < talker_t > (n);

    cout << "[info] Startring program with #" << n << " talkers\n";
    cout << "[info] Press 'q' to stop the program\n";
    sleep(1);
    int threads = n * 2 + 2;
    int value = 0;
    for (int i = 0; i < n; i++) {
        talkers[i] = talker_t {
            -1, -1, 0
        };
    }
    #pragma omp parallel num_threads(threads)
    {
        auto num = omp_get_thread_num();

        if (num < n) {                                                              // section for sender threads
            std::cout << "[thread] I am sender thread #" << num << std::endl;
            while (run) {
                int index = rand() % n;
                while (index == num) {
                    index = rand() % n;
                }
                #pragma omp critical                                                // critical section, initiating the call
                {
                    if (talkers[index].status == 0 && talkers[num].status == 0) {
                        talkers[index].from = num;
                        talkers[index].to = index;
                        talkers[index].status = 1;
                        talkers[num].from = num;
                        talkers[num].to = index;
                        talkers[num].status = 2;
                    }
                }
                sleep(rand() % 5);
            }

        } else if (num < 2 * n) {                                                    // section for reciever threads
            int number = num - n;
            std::cout << "[thread] I am reciever thread #" << number << std::endl;
            while (run) {
                #pragma omp critical                                                // critical section, starting the call
                {
                    if (talkers[number].status == 1) {
                        cout << "[start] " << talkers[number].from << " -> " << talkers[number].to << endl;
                        talkers[number].status = 3;
                        talkers[talkers[number].from].status = 3;
                    }
                }
                sleep(rand() % 5);
                #pragma omp critical                                                // critical section, ending the call
                {
                    if (talkers[number].status == 3) {
                        cout << "[end] " << talkers[number].from << " -> " << talkers[number].to << endl;
                        talkers[talkers[number].from].from = -1;
                        talkers[talkers[number].from].to = -1;
                        talkers[talkers[number].from].status = 0;
                        talkers[number].from = -1;
                        talkers[number].to = -1;
                        talkers[number].status = 0;
                    }
                }
                sleep(5);
            }
        } else if (num < 2 * n + 1) {                                               // section for observer thread
            std::cout << "[thread] I am observer thread #0" << std::endl;
            while(run) {
                #pragma omp critical                                                // critical section, printing vector of talkers
                {
                    cout << "[status] "; 
                    for (int i = 0; i < talkers.size(); i++) {
                        cout << "(" << talkers[i].to << " " << talkers[i].from << " " << talkers[i].status << ") " << flush;
                    }
                    cout << endl;
                    cout.flush();
                }
                sleep(3);
            }
        } else {                                                                    // section to monitor 'q' press and exit
            std::cout << "[thread] I am keypress thread #0" << std::endl;
            while(cin.get() != 'q');
            run = false;
            cout << "[info] Waiting for sleep functions to end...\n";
        }
    }
    cout << "[info] Exiting the program\n";
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <sstream>
#include <stdexcept>

using namespace std;

// status: 0 - ready to call; 1 - is calling; 2 - has call; 3 - during the call
// to: who am i calling now
// from: who i am getting call from

struct talker_t {
    int to;
    int from;
    int status;
};

// mutex: global mutex
// number: thread index
// count: count of threads

struct args_struct_t {
    pthread_mutex_t mutex;
    int number;
    int count;
};


// gloval vector of talkers
static vector<talker_t> talkers;
static vector<string> logs;
static bool run = true;
static bool isFileInput = false;


// thread function for initiating phone calls
void* ringing(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    int count = arg->count;                                                 
    pthread_mutex_t mutex = arg->mutex;
    int number = arg->number;

    pthread_mutex_lock(&mutex);
    if (isFileInput) {
        stringstream ss;
        ss << "[thread] I im sender thread #" << number << endl; 
        logs.push_back(ss.str());
    } else {
        cout << "[thread] I im sender thread #" << number << endl;
        cout.flush();   
    }
    pthread_mutex_unlock(&mutex);

    while (true) {
        int index = rand() % count;
        while (index == number) {
            index = rand() % count;
        }
        pthread_mutex_lock(&mutex);                                             
        if (!run) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (talkers[number].status == 0 && talkers[index].status == 0) {    // if both talkers are free, init call
            talkers[index].from = number;
            talkers[index].to = index;
            talkers[index].status = 1;
            talkers[number].to = index;
            talkers[number].from = number;
            talkers[number].status = 2;
            pthread_mutex_unlock(&mutex);
            sleep(rand() % 10);
        } else {
            pthread_mutex_unlock(&mutex);
            sleep(5);
        }
    }

    return nullptr;
}


// thread function for answering phone calls
void* answering(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    int count = arg->count;
    pthread_mutex_t mutex = arg->mutex;
    int number = arg->number;

    pthread_mutex_lock(&mutex);
    if (isFileInput) {
        stringstream ss;
        ss << "[thread] I am reciever thread #" << number << endl; 
        logs.push_back(ss.str());
    } else {
        cout << "[thread] I am reciever thread #" << number << endl;
        cout.flush();   
    }
    pthread_mutex_unlock(&mutex);


    while (true) {
        pthread_mutex_lock(&mutex);
        if (!run) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (talkers[number].status == 1) {                                  // if i am recieving the call, answer it
            if (isFileInput) {
                stringstream ss;
                ss << "[start] " << talkers[number].from << " -> " << talkers[number].to << endl;
                logs.push_back(ss.str());
            } else {
                cout << "[start] " << talkers[number].from << " -> " << talkers[number].to << endl;
                cout.flush();   
            }
            talkers[number].status == 3;
            talkers[talkers[number].from].status == 3;
            cout.flush();
            pthread_mutex_unlock(&mutex);
            sleep(rand() % 10);
            pthread_mutex_lock(&mutex);                                     // after some time end the call
            if (isFileInput) {
                stringstream ss;
                ss << "[end] " << talkers[number].from << " -> " << talkers[number].to << endl;
                logs.push_back(ss.str());
            } else {
                cout << "[end] " << talkers[number].from << " -> " << talkers[number].to << endl;
                cout.flush();   
            }
            talkers[talkers[number].from].from = -1;
            talkers[talkers[number].from].to = -1;
            talkers[talkers[number].from].status = 0;
            talkers[number].from = -1;
            talkers[number].to = -1;
            talkers[number].status = 0;
            pthread_mutex_unlock(&mutex);
        } else {
            pthread_mutex_unlock(&mutex);
            sleep(1);
        }
    }
    return nullptr;
}

// function to monitor for key press 'q' to exit hte program
void* key_press_thread(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    pthread_mutex_t mutex = arg->mutex;
    while(cin.get() != 'q');
    pthread_mutex_lock(&mutex);
    run = false;
    pthread_mutex_unlock(&mutex);
}

//function to periodically print the vector of talkers
void* observe(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    pthread_mutex_t mutex = arg->mutex;
    while(true) {
        pthread_mutex_lock(&mutex);
        if (!run) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (isFileInput) {
                stringstream ss;
                ss << "[status] ";
                for (int i = 0; i < talkers.size(); i++) {
                    ss << "(" << talkers[i].to << " " << talkers[i].from << " " << talkers[i].status << ") ";
                }
                ss << endl;
                logs.push_back(ss.str());
            } else {
                cout << "[status] "; 
                for (int i = 0; i < talkers.size(); i++) {
                    cout << "(" << talkers[i].to << " " << talkers[i].from << " " << talkers[i].status << ") " << flush;
                }
                cout << endl;
                cout.flush();
            }
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
}


int main(int argc, char const *argv[]) {
    // general args checks
    if (argc < 3) {
        cout << "Invalid args count!\n";
        return 0;
    }
    int n;
    if (strcmp(argv[1], "-c") == 0) {
        try {
            n = atoi(argv[2]);
        } catch (exception e) {
            cout << "Invalid talkers number!\n";
            return 0;
        }
        if (n < 2) {
            cout << "Invalid talkers number!\n";
            return 0;
        }
    } else if (strcmp(argv[1], "-f") == 0) {
        if (argc < 4) {
            cout << "Invalid args count!\n";
            return 0;
        }
        ifstream fin;
        fin.open(argv[2]);
        if (!fin) {
            cout << "Invalid input file!\n";
            return 0;
        }
        fin >> n;
        fin.close();
        if (n < 2) {
            cout << "Invalid talkers number!\n";
            return 0;
        }
        isFileInput = true;
    } else if (strcmp(argv[1], "-r") == 0) {
        if (argc < 4) {
            cout << "Invalid args count!\n";
            return 0;
        }
        try {
            int low = atoi(argv[2]);
            int high = atoi(argv[3]);
            if (low < 2 || low >= high) {
                throw invalid_argument("");
            }
            n = rand() % (high - low) + low;
        } catch (exception e) {
            cout << "Invalid limits for random!\n";
            return 0;
        }
    } else {
        cout << "Invalid flag!\n";
        return 0;
    }


    srand(0);
    if (isFileInput) {
        stringstream ss;
        ss << "[info] Startring program with #" << n << " talkers\n"; 
        logs.push_back(ss.str());
    } else {
        cout << "[info] Startring program with #" << n << " talkers\n";
        cout.flush();   
    }
    cout << "[info] Press 'q' to stop the program\n";
    sleep(1);

    // creating threads 2*talkers + 2 threads for printing vector and keypress
    pthread_t threads[n * 2];
    pthread_t observer;
    pthread_t controller;
    // using the mutex to control access to shared variables
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    logs = vector<string>();
    talkers = vector<talker_t>(n);

    for (int i = 0; i < n; i++) {
        talker_t t {-1, -1, 0};
        talkers[i] = t;
    }

    args_struct_t param_pack[n];
    args_struct_t param_obs;
    param_obs.mutex = mutex;

    for (int i = 0; i < n; i++) {
        param_pack[i].mutex = mutex;
        param_pack[i].number = i;
        param_pack[i].count = n;
        pthread_create(&threads[i], nullptr, ringing, &param_pack[i]);
    }
    for (int i = n; i < n * 2; i++) {
        pthread_create(&threads[i], nullptr, answering, &param_pack[i - n]);
    }

    pthread_create(&controller, nullptr, key_press_thread, &param_obs);
    pthread_create(&observer, nullptr, observe, &param_obs);

    // join only controller, beacuse it is most important
    pthread_join(controller, nullptr);
    // when it dies, run = false, exit the program
    pthread_mutex_lock(&mutex);
    run = false;                
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    // if file, save all logs to file
    if (isFileInput) {
        cout << "[info] Saving logs to file...\n";
        ofstream fout;
        fout.open(argv[3]);
        for (string line: logs) {
            fout << line;
        }
        fout.close();
    }
    cout << "[info] Exiting the program...\n";
    return 0;
}
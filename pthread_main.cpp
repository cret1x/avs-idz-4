#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <vector>

using namespace std;


// status: 0 - ready to call; 1 - is calling; 2 - has call; 3 - during the call
// to: who am i calling now
// from: who i am getting call from

typedef struct talker_struct {
    int to;
    int from;
    int status;
} talker_t;

// mutex: global mutex
// number: thread index
// count: count of threads

typedef struct args_struct {
    pthread_mutex_t mutex;
    int number;
    int count;
} args_struct_t;

// gloval vector of talkers
static vector<talker_t> talkers;


// thread function for initiating phone calls
void* ringing(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    int count = arg->count;                                                 
    pthread_mutex_t mutex = arg->mutex;
    int number = arg->number;

    pthread_mutex_lock(&mutex);
    cout << "I im sender thread #" << number << endl;
    cout.flush();   
    pthread_mutex_unlock(&mutex);

    while (true) {
        int index = rand() % count;
        while (index == number) {
            index = rand() % count;
        }
        pthread_mutex_lock(&mutex);
        if (talkers[number].status == 0 && talkers[index].status == 0) {
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
    cout << "I am reciever thread #" << number << endl;
    cout.flush();
    pthread_mutex_unlock(&mutex);


    while (true) {
        pthread_mutex_lock(&mutex);
        if (talkers[number].status == 1) {
            cout << "[start] " << talkers[number].from << " -> " << talkers[number].to << endl;
            talkers[number].status == 3;
            talkers[talkers[number].from].status == 3;
            cout.flush();
            pthread_mutex_unlock(&mutex);
            sleep(rand() % 10);
            pthread_mutex_lock(&mutex);
            cout << "[end] " << talkers[number].from << " -> " << talkers[number].to << endl;
            cout.flush();
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

void* observe(void *args) {
    args_struct_t *arg = (args_struct_t*) args;
    pthread_mutex_t mutex = arg->mutex;
    while(true) {
        pthread_mutex_lock(&mutex);
        cout << "[status] " << flush;
        for (int i = 0; i < talkers.size(); i++) {
            cout << "(" << talkers[i].to << " " << talkers[i].from << " " << talkers[i].status << ") " << flush;
        }
        cout << endl;
        cout.flush();
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
}


int main() {
    srand(0);
    cout << "Input talkers count: ";
    int n;
    cin >> n; 

    pthread_t threads[n * 2];
    pthread_t observer;

    talkers = vector<talker_t>(n);
    for (int i = 0; i < n; i++) {
        talker_t t {-1, -1, 0};
        talkers[i] = t;
    }
    size_t i;
    args_struct_t param_pack[n];
    args_struct_t param_obs;
    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex, nullptr);

    for (i = 0; i < n; i++) {
        param_pack[i].mutex = mutex;
        param_pack[i].number = i;
        param_pack[i].count = n;
        pthread_create(&threads[i], nullptr, ringing, &param_pack[i]);
    }
    for (i = n; i < n * 2; i++) {
        pthread_create(&threads[i], nullptr, answering, &param_pack[i - n]);
    }

    param_obs.mutex = mutex;

    pthread_create(&observer, nullptr, observe, &param_obs);
    pthread_join(observer, nullptr);
    for (i = 0; i < n * 2; i++) {
        pthread_join(threads[i], nullptr);
    }
    
    pthread_mutex_destroy(&mutex);
    return 0;
}
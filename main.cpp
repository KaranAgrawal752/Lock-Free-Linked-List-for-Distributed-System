#include "lock_free_linked_list.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <pthread.h> // Include pthread header
#include <unistd.h>  // for sleep
#include <chrono>
#include <fstream>
#include <random>


using namespace std;
std::mutex mtx;

void printAtomically(string message) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout <<message<< std::endl;
}
struct ThreadArgs {
    LockFreeLinkedList* linkedList;
    int id;
    int oprn;
    keytype key;
};

void* threadFunction(void * args_ptr) {
    // cout<<"Debug: entered in thread function\n";
    ThreadArgs* args = static_cast<ThreadArgs*>(args_ptr);
    // Accessing univ_obj and invoc_obj from args
    LockFreeLinkedList* linkedList = args->linkedList;
    int id=args->id;
    int oprn=args->oprn;
    keytype key=args->key;
    // cout<<"Debug started the oprn of thread:"<<id<<"\n";
    if(oprn){
        // cout<<"Debug: thread:"<<id<<" going to insert the node\n";
        Node * node=linkedList->Insert(key,key);
        // cout<<"Debug: thread:"<<id<<" inserted the node\n";
        string message;
        if(node){
            message="Thread "+to_string(id)+" successfully inserted the key "+to_string(key)+".";
        }
        else{
            message="Thread "+to_string(id)+" failed to inserted the key "+to_string(key)+" beause it was already inserted.";
        }
        printAtomically(message);
    }
    else{
        Node* node=linkedList->Delete(key);
        string message;
        if(node){
            message="Thread "+to_string(id)+" successfully deleted the key "+to_string(key)+".";
        }
        else{
            message="Thread "+to_string(id)+" failed to delete the key "+to_string(key)+" beause it was not present in list.";
        }
        printAtomically(message);
    }
    return nullptr;
}

int main() {
    cout<<"main started\n";
    int N= 100;
    LockFreeLinkedList* linkedList=new LockFreeLinkedList();
    vector<pthread_t> myThread(N);
    vector<int> oprns(N);//{1,1,1,1,0,0,0,1,1,0};
    vector<keytype> keys(N);//{1,2,3,4,2,5,2,3,9,9};
    random_device rd;
    mt19937 gen(rd());
    for(int i=0;i<N;i++)
    {
        uniform_int_distribution<> dis(0,1); 
        int random_number = dis(gen);
        oprns[i]=random_number;
    }
    for(int i=0;i<N;i++)
    {
        uniform_int_distribution<> dis(1,10); 
        int random_number = dis(gen);
        keys[i]=random_number;
    }
    // cout<<"Debug: Thread calling started\n";
    for (int i = 0; i < N; i++) {
        ThreadArgs* args = new ThreadArgs{linkedList,i,oprns[i],keys[i]};
        if (pthread_create(&myThread[i], nullptr, threadFunction, args)) {
            cerr << "Error creating thread." << endl;
            return 1;
        }
        // cout<<"Debug: the thread:"<<i<<" has been called\n ";
    }

    for (int i = 0; i < N; i++) {
        if (pthread_join(myThread[i], NULL)) {
            cerr << "Error joining thread." << endl;
            return 1;
        }
    }
    return 0;
}

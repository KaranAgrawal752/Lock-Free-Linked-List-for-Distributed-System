#ifndef LOCK_FREE_LINKED_LIST_HPP
#define LOCK_FREE_LINKED_LIST_HPP

#include<stdio.h>
#include<utility>
#include <mutex>
#include<iostream>
#include<cstdint>

#define keytype double
#define eps 0.001
// std::mutex mtx;

// User-defined atomic compare-and-swap function

struct Successor{
    public:
    struct Node* right=nullptr;
    bool mark=0;
    bool flag=0;
    // Successor(Node* ptr,int mrk, int flg){
    //     right=ptr;
    //     mark=mrk;
    //     flag=flg;
    // }
    // Successor(){}
    // bool operator==(const Successor other) const {
    //     return (this->right==other.right && this->mark==other.mark && this->flag==other.flag);
    // }
};


// template<typename T>
// T compareAndSwap(Successor *value, T oldValue, T newValue) {
//     std::lock_guard<std::mutex> lock(mtx);
//     T expected = oldValue;
//     if (value->compare_exchange_strong(expected, newValue)) {
//         return expected; // Return the old value if the swap was successful
//     }
//     return *value; // Return the current value if the swap failed
// }
struct Node {
    public:
    keytype key;
    keytype element;
    Node* backlink;
    Successor succ;
    Node(keytype k, keytype e)
    {
        key=k;element=e;
    }
    Node(){}
};


template<typename T>
T compareAndSwap(T* address, T expected, T newValue) {
    T result;
    asm  volatile(
        "lock cmpxchg %2, %1;"  // Compare the value at *address with expected, if they are equal, set *address to newValue
        : "=a" (result), "+m" (*address)
        : "r" (newValue), "0" (expected)
        : "memory"
    );
    return result;
}

class LockFreeLinkedList {
public:
    LockFreeLinkedList() {
        // Create head node with key of negative infinity
        head = new Node();
        head->key = std::numeric_limits<int>::min();
        head->succ=Successor();
        head->backlink=nullptr;

        // Create tail node with key of positive infinity
        tail = new Node();
        tail->key = std::numeric_limits<int>::max();
        tail->succ=Successor();
        tail->backlink=nullptr;

        // Connect head and tail
        head->succ.right=tail;
    }

    Node* Search(keytype k);
    Node* Insert(keytype k, keytype e);
    Node* Delete(keytype k);


private:
    Node* head;
    Node *tail;
    // std::mutex mtx;

    std::pair<Node*, Node*> SearchFrom(keytype k, Node* currNode);
    std::pair<Node*, bool> TryFlag(Node* prevNode, Node* targetNode);
    void HelpFlagged(Node* prevNode, Node* delNode);
    void TryMark(Node* delNode);
    void HelpMarked(Node* prevNode, Node* delNode);
};

#endif

#ifndef LOCK_FREE_LINKED_LIST_HPP
#define LOCK_FREE_LINKED_LIST_HPP

#include<stdio.h>
#include<utility>
#include <mutex>
#include<iostream>
#include<cstdint>
#include<tuple>

#define keytype double
#define eps 0.001
// std::mutex mtx;

// User-defined atomic compare-and-swap function

struct Successor{
    public:
    struct Node* right=nullptr;
    bool mark=0;
    bool flag=0;
    Successor(Node* ptr,int mrk, int flg){
        right=ptr;
        mark=mrk;
        flag=flg;
    }
    Successor(){}
    // bool operator==(const Successor other) const {
    //     return (this->right==other.right && this->mark==other.mark && this->flag==other.flag);
    // }
};

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

#include <tuple>

template<typename T1, typename T2, typename T3>
std::tuple<T1, T2, T3> compareAndSwap2(T1* val1, T2* val2, T3* val3, T1 ov1, T2 ov2, T3 ov3, T1 nv1, T2 nv2, T3 nv3) {
    T1 old_val1 = ov1;
    T2 old_val2 = ov2;
    T3 old_val3 = ov3;
    bool success = false;

    // Attempt to perform compare-and-swap operation atomically for all three values
    asm volatile (
        "lock cmpxchg %7, %0\n"  // Compare the value at val1 with ov1, if equal, set val1 to nv1
        "je .val2\n"
        "jmp .end\n"
        ".val2:\n"
        "lock cmpxchg %8, %1\n"  // Compare the value at val2 with ov2, if equal, set val2 to nv2
        "je .val3\n"
        "mov %4, %0\n"           // If val2 swap failed, restore val1 to its original value
        "jmp .end\n"
        ".val3:\n"
        "lock cmpxchg %9, %2\n"  // Compare the value at val3 with ov3, if equal, set val3 to nv3
        "je .done\n"
        "mov %4, %0\n"           // If val3 swap failed, restore val1 to its original value
        "mov %5, %1\n"           // If val3 swap failed, restore val2 to its original value
        "jmp .end\n"
        ".done:\n"
        "mov $1, %3\n"           // Set success flag to 1 if all swaps succeeded
        ".end:\n"
        : "+m" (*val1), "+m" (*val2), "+m" (*val3), "+r" (success)
        : "r" (ov1), "r" (ov2), "r" (ov3), "r" (nv1), "r" (nv2), "r" (nv3)
        : "cc", "memory"
    );

    // If all compare-and-swap operations were successful, return old values, else return current values
    if (success) {
        std::cout<<"*******************************";
        std::cout<<*val1<<"  a "<<ov1<<"  b  "<<nv1<<"  c  ";
        std::cout<<*val2<<"  a "<<ov2<<"  b  "<<nv2<<"  c  ";
        std::cout<<*val3<<"  a "<<ov3<<"  b  "<<nv3<<"  c  ";
        return std::make_tuple(ov1, ov2, ov3);
    } else {
        // std::cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&";
        // std::cout<<*val1<<"  a "<<ov1<<"  b  "<<nv1<<"  c  ";
        return std::make_tuple(*val1, *val2, *val3);
    }
}

inline
Successor compareAndSwap(Successor* address, Successor expected, Successor newValue) {
    Successor result;
    std::tuple t=compareAndSwap2(&(address->right),&(address->mark),&(address->flag),expected.right,expected.mark,expected.flag,newValue.right,newValue.mark,newValue.flag);
    result.right= std::get<0>(t);
    result.mark= std::get<1>(t);
    result.flag= std::get<2>(t);
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

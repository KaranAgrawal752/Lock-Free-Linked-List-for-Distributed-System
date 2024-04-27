#ifndef LOCK_FREE_LINKED_LIST_HPP
#define LOCK_FREE_LINKED_LIST_HPP

#include<stdio.h>
#include<utility>
#include <mutex>
#include<iostream>
#include<cstdint>
#include<cstdlib>
#include<tuple>

#define keytype double
#define eps 0.001
#define Level int
#define INT_MIN std::numeric_limits<int>::min()
#define INT_MAX std::numeric_limits<int>::max()

enum Status{
    DELETED,
    IN
};


struct Successor{
    public:
    unsigned long long packed_succ=0;
    void set_right(struct Node *right){

        unsigned long long val;
        val=(unsigned long long)right&(~(((unsigned long long)0xffff)<<48));
        packed_succ|=val;
    }
    void set_mark(bool mark){
        if(mark){
            packed_succ|=(((unsigned long long)0x0001)<<48);
        }else{
            packed_succ&=(~(((unsigned long long)0x0001)<<48));
        }
    }
    void set_flag(bool flag){
         if(flag){
            packed_succ|=(((unsigned long long)0x0002)<<48);
        }else{
            packed_succ&=(~(((unsigned long long)0x0002)<<48));
        }
    }
    struct Node* get_right()
    {
        unsigned long long val=packed_succ&(~(((unsigned long long)0xffff)<<48));
        return (struct Node*)val;
    }
    bool get_mark()
    {
        if(packed_succ & (((unsigned long long)0x0001)<<48)) return true;
        else return false;
    }
    bool get_flag()
    {
        if(packed_succ & (((unsigned long long)0x0002)<<48)) return true;
        else return false;
    }

    Successor(Node* ptr,int mrk, int flg){
        set_right(ptr);
        set_flag(flg);
        set_mark(mrk);
    }
    Successor(){}
};

struct Node {
    public:
    keytype key;
    keytype element;
    Node* backlink;
    Successor succ;
    Node * up;
    Node * down;
    Node* tower_root;
    Node(keytype k, keytype e=-1)
    {
        key=k;element=e;
    }
    Node(){}
};


template<typename T>
T CAS(T* address, T expected, T newValue) {
    T result;
    asm volatile (
        "mov %2, %%rax;"
        "lock cmpxchg %3, %1;"   // Compare and exchange oldVal with the value at address
        "mov %%rax, %0;"               // Set result to 1 if successful (ZF flag is set), 0 otherwise
        : "=r" (result), "+m" (*address) // Output operands (result, address)
        : "r" (expected), "r" (newValue)    // Input operands (oldVal, newVal)
        : "rax", "memory"          // Clobbers (condition codes and memory)
    );
    return result;
}

class LockFreeSkipList {
public:
    LockFreeSkipList(int level) {
        Node *curHead, *curTail;
        Node *lastHead=nullptr, * lastTail=nullptr;
        for(int i=level;i>=1;i--)
        {
            curHead=new Node();
            curHead->key=INT_MIN;
            curHead->succ=Successor();
            curHead->backlink=nullptr;

            curTail=new Node();
            curTail->key=INT_MAX;
            curTail->succ=Successor();
            curTail->backlink=nullptr;

            curHead->succ.set_right(curTail);

            if(lastHead)
                curHead->up=lastHead;
            if(lastTail)
                curTail->up=lastTail;
            lastHead=curHead;
            lastTail=curTail;
        }
        maxLevel=level;
        head=lastHead;
        while(curHead)
        {
            curTail=curHead->succ.get_right();
            lastHead=curHead->up;
            lastTail=curTail->up;
            if(lastHead){
                lastHead->down=curHead;
                lastTail->down=curTail;
            }
            curHead=lastHead;
        }
    }

    Node* Search_SL(keytype k);
    Node* Insert_SL(keytype k, keytype e);
    Node* Delete_SL(keytype k);


private:
    Node* head;
    Node *tail;
    int maxLevel;
    // std::mutex mtx;

    std::pair<Node*, Node*> SearchToLevel_SL(keytype k, Level v);
    std::pair<Node*,Level> FindStart_SL(Level v);
    std::pair<Node*, Node*> SearchRight(keytype k, Node* currNode);

    std::pair<Node*,Node*> InsertNode(Node* newNode, Node* prevNode, Node* nextNode);
    Node* DeleteNode(Node* prevNode, Node* delNode);

    std::tuple<Node*,Status, bool> TryFlagNode(Node* prevNode, Node* targetNode);
    void HelpFlagged(Node* prevNode, Node* delNode);
    void TryMark(Node* delNode);
    void HelpMarked(Node* prevNode, Node* delNode);
};

#endif

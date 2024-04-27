#include "lock_free_linked_list.hpp"
#include <tuple>

// template<typename T1, typename T2, typename T3> 
// std::tuple<T1, T2, T3> compareAndSwap2(T1* val1, T2* val2, T3* val3, T1 ov1, T2 ov2, T3 ov3, T1 nv1, T2 nv2, T3 nv3) {
//     T1 old_val1;
//     T2 old_val2;
//     T3 old_val3;
//     bool result;

//     // Attempt to perform compare-and-swap operation atomically for all three values
//     asm volatile (
//         "mov %4, %%eax\n"
//         "lock cmpxchg %7, %0\n"  // Compare the value at val1 with ov1, if equal, set val1 to nv1
//         "je .val2\n"
//         "mov $0,%3\n"
//         "jmp .end\n"
//         ".val2:\n"
//         "mov %5, %%eax\n"
//         "lock cmpxchg %8, %1\n"  // Compare the value at val2 with ov2, if equal, set val2 to nv2
//         "je .val3\n"
//         "mov %4, %0\n"           // If val2 swap failed, restore val1 to its original value
//         "mov $0,%3\n"
//         "jmp .end\n"
//         ".val3:\n"
//         "mov %6, %%eax\n"
//         "lock cmpxchg %9, %2\n"  // Compare the value at val3 with ov3, if equal, set val3 to nv3
//         "je .done\n"
//         "mov %4, %0\n"           // If val3 swap failed, restore val1 to its original value
//         "mov %5, %1\n"           // If val3 swap failed, restore val2 to its original value
//         "mov $0,%3\n"
//         "jmp .end\n"
//         ".done:\n"
//         "mov $1, %3\n"           // Set success flag to 1 if all swaps succeeded
//         ".end:\n"
//         : "+m" (*val1), "+m" (*val2), "+m" (*val3), "+r"(result)
//         : "r" (ov1), "r" (ov2), "r" (ov3), "r" (nv1), "r" (nv2), "r" (nv3)
//         : "cc", "memory"
//     );

//     // If all compare-and-swap operations were successful, return old values, else return current values
//     if (result) {
//         std::cout<<"*******************************";
//         // std::cout<<*val1<<"  a "<<ov1<<"  b  "<<nv1<<"  c  ";
//         // std::cout<<*val2<<"  a "<<ov2<<"  b  "<<nv2<<"  c  ";
//         // std::cout<<*val3<<"  a "<<ov3<<"  b  "<<nv3<<"  c  ";
//         return std::make_tuple(ov1, ov2, ov3);
//     } else {
//         std::cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&";
//         // std::cout<<*val1<<"  a "<<ov1<<"  b  "<<nv1<<"  c  ";
//         return std::make_tuple(*val1, *val2, *val3);
//     }
//     return std::make_tuple(old_val1,old_val2,old_val3);
// }

// inline
// Successor compareAndSwap(Successor* address, Successor expected, Successor newValue) {
//     Successor result;
//     std::tuple t=compareAndSwap2(&(address->right),&(address->mark),&(address->flag),expected.right,expected.mark,expected.flag,newValue.right,newValue.mark,newValue.flag);
//     result.right= std::get<0>(t);
//     result.mark= std::get<1>(t);
//     result.flag= std::get<2>(t);
//     return result;
// }


Successor compareAndSwap(Successor* address, Successor expected, Successor newValue)
{
    unsigned long long result=CAS(&address->packed_succ,expected.packed_succ,newValue.packed_succ);
    Successor temp_succ=Successor();
    temp_succ.packed_succ=result;
    return temp_succ;
}



std::pair<Node*, Node*> LockFreeLinkedList::SearchFrom(keytype k, Node* currNode) {
    // std::cout<<"Debug inside the SearchFrom and calling\n";
    Node* nextNode = currNode->succ.get_right();
    // std::cout<<"Debug:"<< currNode<<" inside the SearchFrom and got nextNode\n";
    // std::cout<<"Debug:"<< nextNode<<" inside the SearchFrom and got nextNode\n";
    // std::cout<<"Debug:"<< nextNode->key<<" inside the SearchFrom and got nextNode\n";
    while (nextNode->key <= k) {
    // std::cout<<"Debug inside the SearchFrom and inside loop\n";
        while (nextNode->succ.get_mark() == 1 && (currNode->succ.get_mark() == 0 || currNode->succ.get_right() != nextNode)) {
            if (currNode->succ.get_right() == nextNode) {
                HelpMarked(currNode, nextNode);
            }
            nextNode = currNode->succ.get_right();
        }
        if (nextNode->key <= k) {
            currNode = nextNode;
            nextNode = currNode->succ.get_right();
        }
    }
    // std::cout<<"Debug inside the SearchFrom and outside loop\n";
    return std::make_pair(currNode, nextNode);
}

Node* LockFreeLinkedList::Search(keytype k) {
    std::pair<Node*,Node*> pair=SearchFrom(k-eps,head);
    Node* currNode=pair.first;
    Node* nextNode=pair.second;
    if (currNode->key == k) {
        return currNode;
    } else {
        return nullptr; // NO_SUCH_KEY
    }
}

void LockFreeLinkedList::HelpMarked(Node* prevNode, Node* delNode) {
    Node* nextNode=delNode->succ.get_right();
    Successor sc1=Successor(); sc1.set_right(delNode);sc1.set_mark(0);sc1.set_flag(1);
    Successor sc2=Successor(); sc2.set_right(nextNode);sc2.set_mark(0);sc2.set_flag(0);
    compareAndSwap(&(prevNode->succ),sc1,sc2);
}

Node* LockFreeLinkedList::Delete(keytype k) {
    std::pair<Node*,Node*> pair=SearchFrom(k-eps,head);
    Node* prevNode=pair.first;
    Node* delNode=pair.second;
    if(delNode->key!=k){
        return nullptr;
    }
    bool result;
    std::pair<Node*,bool> pair2=TryFlag(prevNode,delNode);
    prevNode=pair2.first;
    result=pair2.second;
    if(prevNode!=nullptr){
        HelpFlagged(prevNode,delNode);
    }   
    if(result==false){
        return nullptr;
    }
    return delNode;
}

void LockFreeLinkedList::HelpFlagged(Node* prevNode, Node* delNode) {
    delNode->backlink=prevNode;
    if(delNode->succ.get_mark()==0){
        TryMark(delNode);
    }
    HelpMarked(prevNode,delNode);
}

void LockFreeLinkedList::TryMark(Node* delNode) {
    do{
        auto nextNode=delNode->succ.get_right();
        // auto result=delNode->succ;
        auto result=compareAndSwap(&(delNode->succ),Successor(nextNode,0,0),Successor(nextNode,1,0));
        if(result.get_flag()==1){
            HelpFlagged(delNode,result.get_right());
        }
    }while(delNode->succ.get_mark()==0);
}

std::pair<Node*, bool> LockFreeLinkedList::TryFlag(Node* prevNode, Node* targetNode) {
    while (true) {
        if (prevNode->succ.get_flag() == 1) {  //Predecessor is already flagged
            return std::make_pair(prevNode, false);
        }
        // auto result =prevNode->succ;
        auto result=compareAndSwap(&(prevNode->succ), Successor(targetNode, 0, 0), Successor(targetNode, 0, 1));
        if (result.get_right()==targetNode && result.get_mark()==0 && result.get_flag()==0) {   //Successful flagging
            return std::make_pair(prevNode, true);
        }
        if (result.get_right()==targetNode && result.get_mark()==0 && result.get_flag()==1) {   //Successful flagging
            return std::make_pair(prevNode, true);
        }
        while (prevNode->succ.get_mark() == 1) {
            prevNode = prevNode->backlink;
        }
        auto searchResult = SearchFrom(targetNode->key - eps, prevNode);
        if (searchResult.second != targetNode) {
            return std::make_pair(nullptr, false);
        }
    }
}

Node* LockFreeLinkedList::Insert(keytype k, keytype e) {
    // int z=7;
    // int y=compareAndSwap(&z,7,6);
    // std::cout<<y<<" "<<z<<std::endl;
    // int val1=3, val2=4,val3=5;
    // std::tuple t=compareAndSwap2(&val1,&val2,&val3, 3,4,5,8,9,10);
    // std::cout<<std::get<0>(t)<<" "<<std::get<1>(t)<<" "<<std::get<2>(t)<<" "<<val1<<" "<<val2<<" "<<val3<<std::endl;
    // std::cout<<"debug Inside the insert and calling searchFrom\n";
    std::pair<Node*,Node*> pair=SearchFrom(k,head);
    // std::cout<<"debug Inside the insert and called searchFrom\n";
    Node* prevNode=pair.first;
    Node* nextNode=pair.second;
    // std::cout<<"prevNode "<<prevNode->key<<" nextNode "<<nextNode->key<<"\n";
    if(prevNode->key==k){
        return nullptr;
    }
    auto newNode= new Node(k,e);
    while(true)
    {
        if(prevNode->succ.get_flag()==1){
            HelpFlagged(prevNode,prevNode->succ.get_right());
        }
        else{
            newNode->succ=Successor(); newNode->succ.set_right(nextNode);newNode->succ.set_mark(0);newNode->succ.set_flag(0);
            // auto result=newNode->succ;
            auto result=compareAndSwap(&(prevNode->succ),Successor(nextNode,0,0),Successor(newNode,0,0));
            // std::cout<<"prevNode "<<prevNode->succ.right<<" nextNode "<<nextNode<<" result "<<result.right<<" newNode "<<newNode<<"\n";
            if(result.get_right()==nextNode && result.get_mark()==0 && result.get_flag()==0){
                return newNode;
            }
            else{
                if(result.get_flag()==1){
                    HelpFlagged(prevNode,result.get_right());
                }
                while(prevNode->succ.get_mark()==1){
                    prevNode=prevNode->backlink;
                }
            }
        }
        std::pair<Node*,Node*> pair=SearchFrom(k,head);
        Node* prevNode=pair.first;
        Node* nextNode=pair.second;
        if(prevNode->key==k){
            free(newNode);
            return nullptr;
        } 
    }
}





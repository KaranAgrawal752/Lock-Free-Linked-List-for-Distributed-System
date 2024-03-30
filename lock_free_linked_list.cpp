#include "lock_free_linked_list.hpp"

std::pair<Node*, Node*> LockFreeLinkedList::SearchFrom(keytype k, Node* currNode) {
    Node* nextNode = currNode->succ.right;
    while (nextNode->key <= k) {
        while (nextNode->succ.mark == 1 && (currNode->succ.mark == 0 || currNode->succ.right != nextNode)) {
            if (currNode->succ.right == nextNode) {
                HelpMarked(currNode, nextNode);
            }
            nextNode = currNode->succ.right;
        }
        if (nextNode->key <= k) {
            currNode = nextNode;
            nextNode = currNode->succ.right;
        }
    }
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
    Node* nextNode=delNode->succ.right;
    Successor sc1=Successor(); sc1.right=delNode;sc1.mark=0;sc1.flag=1;
    Successor sc2=Successor(); sc2.right=nextNode;sc2.mark=0;sc2.flag=0;
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
    if(delNode->succ.mark==0){
        TryMark(delNode);
    }
    HelpMarked(prevNode,delNode);
}

void LockFreeLinkedList::TryMark(Node* delNode) {
    do{
        auto nextNode=delNode->succ.right;
        auto result=nextNode->succ;//compareAndSwap(&(delNode->succ),Successor(nextNode,0,0),Successor(nextNode,1,0));
        if(result.flag==1){
            HelpFlagged(delNode,result.right);
        }
    }while(delNode->succ.mark==0);
}

std::pair<Node*, bool> LockFreeLinkedList::TryFlag(Node* prevNode, Node* targetNode) {
    while (true) {
        if (prevNode->succ.flag == 1) {  //Predecessor is already flagged
            return std::make_pair(prevNode, false);
        }
        auto result = prevNode->succ;//compareAndSwap(&(prevNode->succ), Successor(targetNode, 0, 0), Successor(targetNode, 0, 1));
        if (result.right==targetNode && result.mark==0 && result.flag==0) {   //Successful flagging
            return std::make_pair(prevNode, true);
        }
        if (result.right==targetNode && result.mark==0 && result.flag==1) {   //Successful flagging
            return std::make_pair(prevNode, true);
        }
        while (prevNode->succ.mark == 1) {
            prevNode = prevNode->backlink;
        }
        auto searchResult = SearchFrom(targetNode->key - eps, prevNode);
        if (searchResult.second != targetNode) {
            return std::make_pair(nullptr, false);
        }
    }
}

Node* LockFreeLinkedList::Insert(keytype k, keytype e) {
    std::pair<Node*,Node*> pair=SearchFrom(k,head);
    Node* prevNode=pair.first;
    Node* nextNode=pair.second;
    // std::cout<<"prevNode "<<prevNode->key<<" nextNode "<<nextNode->key<<"\n";
    if(prevNode->key==k){
        return nullptr;
    }
    auto newNode= new Node(k,e);
    while(true)
    {
        if(prevNode->succ.flag==1){
            HelpFlagged(prevNode,prevNode->succ.right);
        }
        else{
            newNode->succ=Successor(); newNode->succ.right=nextNode;newNode->succ.mark=0;newNode->succ.flag=0;
            auto result=prevNode->succ;//compareAndSwap(&(prevNode->succ),Successor(nextNode,0,0),Successor(newNode,0,0));
            if(result.right==nextNode && result.mark==0 && result.flag==0){
                return newNode;
            }
            else{
                if(result.flag==1){
                    HelpFlagged(prevNode,result.right);
                }
                while(prevNode->succ.mark==1){
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





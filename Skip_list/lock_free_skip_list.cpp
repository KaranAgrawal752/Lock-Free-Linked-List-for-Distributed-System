#include "lock_free_skip_list.hpp"

std::string flipCoin() {
    srand(time(0));
    int result = rand() % 2;
    return (result == 0) ? "Heads" : "Tails";
}

Successor compareAndSwap(Successor* address, Successor expected, Successor newValue)
{
    unsigned long long result=CAS(&address->packed_succ,expected.packed_succ,newValue.packed_succ);
    Successor temp_succ=Successor();
    temp_succ.packed_succ=result;
    return temp_succ;
}

Node* LockFreeSkipList::Search_SL(keytype k)
{
    std::pair<Node*,Node*> pair;
    Node * currNode;
    pair=SearchToLevel_SL(k,1);
    currNode=pair.first;
    if(currNode->key==k){
        return currNode;
    }else{
        return nullptr;
    }
}

std::pair<Node*,Node*> LockFreeSkipList::SearchToLevel_SL(keytype k, Level v){
    std::pair<Node*,Level> pair;
    std::pair<Node*,Node*> pairN;
    Node* currNode,*nextNode;
    Level currV;
    pair=FindStart_SL(v);
    currNode=pair.first;
    currV=pair.second;
    while(currV>v)
    {
        pairN=SearchRight(k,currNode);
        currNode=currNode->down;
        currV--;
    }
    pairN=SearchRight(k,currNode);
    return pairN;
}

std::pair<Node*,Level> LockFreeSkipList::FindStart_SL(Level v){
    Node* currNode=head;
    Level currV=1;
    while(currNode->up->succ.get_right()->key!=INT_MAX || currV<v){
        currNode=currNode->up;
        currV++;
    }
    return std::make_pair(currNode,currV);
}

std::pair<Node*, Node*> LockFreeSkipList::SearchRight(keytype k, Node* currNode) {
    std::tuple<Node*,Status,bool> tryFlagResult;
    Node* nextNode;
    bool result;
    Status status;
    nextNode=currNode->succ.get_right();
    while (nextNode->key <= k) {
        while (nextNode->tower_root->succ.get_mark() == 1) {
            tryFlagResult=TryFlagNode(currNode,nextNode);
            currNode=std::get<0>(tryFlagResult);
            status=std::get<1>(tryFlagResult);
            if (status==IN) {
                HelpFlagged(currNode, nextNode);
            }
            nextNode = currNode->succ.get_right();
        }
        if (nextNode->key <= k) {
            currNode = nextNode;
            nextNode = currNode->succ.get_right();
        }
    }
    return std::make_pair(currNode, nextNode);
}

std::tuple<Node*, Status,bool> LockFreeSkipList::TryFlagNode(Node* prevNode, Node* targetNode) {
    std::pair<Node*,Node*> searchResult;
    while (true) {
        if (prevNode->succ.get_flag() == 1 && prevNode->succ.get_right()==targetNode) {  //Predecessor is already flagged
            return std::make_tuple(prevNode,IN, false);
        }
        // auto result =prevNode->succ;
        auto result=compareAndSwap(&(prevNode->succ), Successor(targetNode, 0, 0), Successor(targetNode, 0, 1));
        if (result.get_right()==targetNode && result.get_mark()==0 && result.get_flag()==0) {   //Successful flagging
            return std::make_tuple(prevNode,IN, true);
        }
        if (result.get_right()==targetNode && result.get_mark()==0 && result.get_flag()==1) {   //Successful flagging
            return std::make_tuple(prevNode, IN, false);
        }
        while (prevNode->succ.get_mark() == 1) {
            prevNode = prevNode->backlink;
        }
        searchResult = SearchRight(targetNode->key - eps, prevNode);
        if (searchResult.second != targetNode) {
            return std::make_tuple(nullptr, DELETED,false);
        }
    }
}

Node* LockFreeSkipList::Delete_SL(keytype k) {
    std::pair<Node*,Node*> pair;
    Node* prevNode,*delNode;
    Node* result;
    pair=SearchToLevel_SL(k-eps,1);
    prevNode=pair.first;
    delNode=pair.second;
    if(delNode->key!=k){
        return nullptr;
    }
    result=DeleteNode(prevNode,delNode);
    if(result==nullptr){
        return nullptr;
    }
    SearchToLevel_SL(k,2);
    return delNode;
}

Node* LockFreeSkipList::DeleteNode(Node* prevNode,Node* delNode){
    std::tuple<Node*, Status, bool> tryFlagResult;
    Status status;
    bool result;
    tryFlagResult=TryFlagNode(prevNode,delNode);
    status=std::get<1>(tryFlagResult);
    result=std::get<0>(tryFlagResult);
    if(status==IN){
        HelpFlagged(prevNode,delNode);
    }
    if(result==false){
        return nullptr;
    }
    return delNode;
}

void LockFreeSkipList::HelpMarked(Node* prevNode, Node* delNode) {
    Node* nextNode;
    Successor sc1,sc2;
    sc1=Successor(); 
    sc2=Successor();
    nextNode=delNode->succ.get_right();
    sc1.set_right(delNode);sc1.set_mark(0);sc1.set_flag(1);
    sc2.set_right(nextNode);sc2.set_mark(0);sc2.set_flag(0);
    compareAndSwap(&(prevNode->succ),sc1,sc2);
}


void LockFreeSkipList::HelpFlagged(Node* prevNode, Node* delNode) {
    delNode->backlink=prevNode;
    if(delNode->succ.get_mark()==0){
        TryMark(delNode);
    }
    HelpMarked(prevNode,delNode);
}

void LockFreeSkipList::TryMark(Node* delNode) {
    Node* nextNode;
    Successor result;
    do{
        nextNode=delNode->succ.get_right();
        // auto result=delNode->succ;
        result=compareAndSwap(&(delNode->succ),Successor(nextNode,0,0),Successor(nextNode,1,0));
        if(result.get_flag()==1){
            HelpFlagged(delNode,result.get_right());
        }
    }while(delNode->succ.get_mark()==0);
}


Node* LockFreeSkipList::Insert_SL(keytype k, keytype e) {
    std::pair<Node*,Node*> pair,insertNodeResult;
    Node* newRNode,*prevNode,*nextNode,*newNode,*result,*lastNode;
    Level currV=1,tH=1;
    pair=SearchToLevel_SL(k,1);
    prevNode=pair.first;
    nextNode=pair.second;
    if(prevNode->key==k){
        return nullptr;
    }
    newRNode= new Node(k,e);
    newRNode->tower_root=newRNode;
    newNode=newRNode;
    while((flipCoin()=="Head") && (tH<=maxLevel-1)){
        tH++;
    }
    while(true)
    {
        insertNodeResult=InsertNode(newNode,prevNode,nextNode);
        prevNode=insertNodeResult.first;
        result=insertNodeResult.second;
        if(result==nullptr &&currV==1){
            free(newNode);
            return nullptr;
        }
        if(newRNode->succ.get_mark()==1){ //if the toer become superfluous
            if(result==newNode && newNode!=newRNode){
                DeleteNode(prevNode,newNode);
            }
            return newRNode;
        }
        currV++;
        if(currV=tH+1){
            return newRNode;
        }
        lastNode=newNode;
        newNode=new Node(k,e);
        newNode->tower_root=newRNode;
        newNode->down=lastNode;
        pair=SearchToLevel_SL(k,currV);
        prevNode=pair.first;
        nextNode=pair.second;
    }
}

std::pair<Node*,Node*> LockFreeSkipList::InsertNode(Node* newNode,Node* prevNode,Node*nextNode){
    Successor prevSucc,result;
    std::pair<Node*,Node*> pair;
    if(prevNode->key==newNode->key){
        return std::make_pair(prevNode,nullptr);
    }
    while(true){
        prevSucc=prevNode->succ;
        if(prevSucc.get_flag()==1){
            HelpFlagged(prevNode,prevSucc.get_right());
        }else{
            newNode->succ=Successor(nextNode,0,0);
            result=compareAndSwap(&(prevNode->succ),Successor(nextNode,0,0),Successor(newNode,0,0));
            if(result.get_right()==nextNode && result.get_flag()==0 && result.get_mark()==0){
                return std::make_pair(prevNode,newNode);
            }else{
                if(result.get_flag()==1){
                    HelpFlagged(prevNode,result.get_right());
                }
                while(prevNode->succ.get_mark()==1){
                    prevNode=prevNode->backlink;
                }
            }
        }
        pair=SearchRight(newNode->key,prevNode);
        prevNode=pair.first;
        nextNode=pair.second;
        if(prevNode->key==newNode->key){
            return std::make_pair(prevNode,nullptr);
        }
    }
}





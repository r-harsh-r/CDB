// -----------------------------------------------------------------------------------------
    // | type | nkeys |  pointers  |  offsets   | key-values | unused |
    // |  2B  |   2B  | (nkeys + 1) × 8B | nkeys × 2B |     ...    |        |

    // | type | nkeys | pointers | offsets |            key-values           | unused |
    // |   2  |   2   | nil nil  |  8 19   | 2 2 "k1" "hi"  2 5 "k3" "hello" |        |
    // |  2B  |  2B   |   2×8B   |  2×2B   | 4B + 2B + 2B + 4B + 2B + 5B     |        |


#pragma once

#include<iostream>
#include<string>
#include<fcntl.h>
#include<unistd.h>
#include<libgen.h>
#include<cstring>
#include<cstdint>
#include<sys/stat.h>
#include<vector>
using namespace std;

#define BNODE_NODE  1
#define BNODE_LEAF 2

const int PAGE_SIZE = 4096;
const int PAGE_LIMIT = 2048;

const int MAX_NKEYS = 10;

struct SplitResult{
    uint32_t leftPage,rightPage;
    std::string promotedKey;
};

struct NodeSplitResult{
    uint32_t leftPage = 0,rightPage = 0;
    std::string promotedKey;
    uint32_t nodeCurrPageNum = 0;
};

class DiskFile{
private : 
    int fd = -1;
    const char* filename;

    // Header Content : Stored in page 0
    int32_t page_size,
            next_free_page,
            total_page_alloted,
            root_page;

    
public:
    DiskFile(const char*filename_);

    ~DiskFile();

    // update the Header page with current values
    void updateHeader();

    // Read the page number, and store it in buffer, return true if read successfully
    bool readPage(uint32_t pageNum,void*buff);
    
    // Write at pagenum, content of buff
    bool writePage(uint32_t pageNum,const void*buff){
        off_t offset = lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
        if(offset == -1){
            return false;
        }
        
        ssize_t n = write(fd,buff,PAGE_SIZE);
        
        int32_t total_page_alloted = getPageCount();
        lseek(fd,8,SEEK_SET);
        write(fd,&total_page_alloted,sizeof(total_page_alloted));


        return n == PAGE_SIZE;
    }


    // return a free page number : either from free list or newly created page
    uint32_t allocatePage(){
        int32_t nextFreePage;
        lseek(fd,4,SEEK_SET);
        read(fd,&nextFreePage,sizeof(nextFreePage));
        
        // next page available
        if(nextFreePage != 0){
            uint32_t page = nextFreePage;
            lseek(fd,nextFreePage*PAGE_SIZE,SEEK_SET);
            
            int32_t next;
            read(fd,&next,sizeof(next));

            next_free_page = next;
            updateHeader();

            return page;
        }else{
            uint32_t newPage = appendPage();    // appendPage() adds a new page in the book
            if(newPage == 0) newPage = 1;

            total_page_alloted = newPage + 1;
            updateHeader();
            return newPage;
        }
    }

    // Mark the pageNum as free and add it to free_page linked list
    void freePage(uint32_t pageNum){
        if(pageNum == 0) return;
        
        char buff[PAGE_SIZE] = {0};
        *(uint32_t*)buff = next_free_page;

        writePage(pageNum,buff);

        next_free_page =  pageNum;

        updateHeader();

    }

    // Adds a new page in the book
    uint32_t appendPage(){

        off_t size = lseek(fd,0,SEEK_END);

        if(size == -1) return 0;

        uint32_t newPage = size/PAGE_SIZE;

        char zero[PAGE_SIZE] = {0};
        if(!writePage(newPage,zero)) return 0;
        return newPage;
    }

    // returns total number of pages Attached
    uint32_t getPageCount(){
        struct stat st;
        if (fstat(fd, &st) == -1) return 0;
        return st.st_size / PAGE_SIZE;
    }


    // input : a free page number, type of node it is, number of keys in it, list of pointers, list of keys and list of values
    // output : returns true if created successfully
    int createNode(uint32_t pageNum,int16_t type,int16_t nkeys,int64_t ptrs[],const char*keys[],const char*vals[]){
        lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);

        write(fd,&type,sizeof type);
        write(fd,&nkeys,sizeof nkeys);

        for(int i = 0;i<=nkeys;i++){
            write(fd,&ptrs[i],sizeof(ptrs[i]));
        }

        // offset calculation : see serialization schema
        int16_t offset = 0;
        for(int i = 0;i<nkeys;i++){
            offset += 4;
            offset += strlen(keys[i]);
            offset += strlen(vals[i]);

            write(fd,&offset,sizeof(offset));
        }

        for(int i = 0;i<nkeys;i++){
            int16_t keySize = strlen(keys[i]);
            int16_t valSize = strlen(vals[i]);

            write(fd,&keySize,sizeof(keySize));
            write(fd,&valSize,sizeof(valSize));
            write(fd,keys[i],keySize);
            write(fd,vals[i],valSize);
        }
        return 1;
    }

    // return number of keys (nkeys) of a node stored at pageNum 
    uint16_t getnkeys(uint32_t pageNum){
        lseek(fd,pageNum*PAGE_SIZE + 2,SEEK_SET);
        uint16_t nkeys;
        read(fd,&nkeys,sizeof(nkeys));
        return nkeys;
    }
    
    // return type of node at pagenum
    uint16_t getType(uint32_t pageNum){
        lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
        uint16_t type;
        read(fd,&type,sizeof(type));
        return type;
    }

    // return root_page = page number of root of B+ tree
    uint32_t getRoot(){
        return root_page;
    }

    // returns the nth POINTER (is basycally page number)
    // zero indexed
    int64_t getnthPageNum(uint32_t pageNum,int n){
        lseek(fd,pageNum*PAGE_SIZE + 4 + n*8,SEEK_SET);
        int64_t nthPtr;
        read(fd,&nthPtr,sizeof(nthPtr));

        return nthPtr;
    }

    vector<int64_t> getPtrs(uint32_t pageNum){
        vector<int64_t> ptrVec;
        uint16_t numKeys = getnkeys(pageNum);

        lseek(fd,pageNum*PAGE_SIZE + 4,SEEK_SET);

        uint64_t temp;
        for(int i = 0;i<=numKeys;i++){
            read(fd,&temp,sizeof(temp));
            ptrVec.push_back(temp);
        }

        return ptrVec;
    }

    vector<string> getKeys(uint32_t pageNum){
        vector<string>keyVec;
        uint16_t numKeys = getnkeys(pageNum);

        char key[1000];
        char val[1000];

        for(int i = 0;i<numKeys;i++){
            getnthKV(pageNum,i,key,val);
            keyVec.push_back(string(key));
        }

        return keyVec;
        
    }

    // GET nTH KEY (0 INDEXED)
    void getnthKV(uint32_t pageNum,int n,char key[],char val[]){
        lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
        uint32_t nKeys = getnkeys(pageNum);

        uint16_t offset = 0;
        
        if(n > 0){
            lseek(fd,pageNum*PAGE_SIZE + (2 + 2 + (nKeys + 1)*8) + (n - 1)*2,SEEK_SET);
            read(fd,&offset,sizeof(offset));
        }

        lseek(fd,pageNum*PAGE_SIZE + (2 + 2 + (nKeys + 1)*8) + (nKeys)*2 + offset,SEEK_SET);

        // read sizes
        uint16_t keylen,vallen;
        read(fd,&keylen,sizeof(keylen));
        read(fd,&vallen,sizeof(vallen));

        read(fd,key,keylen);
        key[keylen] = '\0';
        read(fd,val,vallen);
        val[vallen] = '\0';
    }

    // CoW pattern : create a new node at newPageNum, dont alter the oldPageNum
    void insertKVatNode(uint32_t newPageNum, uint32_t oldPageNum,const char key[],const char val[]){
        uint16_t nkeys_old = getnkeys(oldPageNum);
        uint16_t type = getType(oldPageNum);

        vector<string> keysVec;
        vector<string> valsVec;
        char tempKey[PAGE_SIZE]; // Temporary buffer for reading
        char tempVal[PAGE_SIZE];

        for(int i = 0;i<nkeys_old;i++){
            getnthKV(oldPageNum,i,tempKey,tempVal);
            keysVec.push_back(string(tempKey));
            valsVec.push_back(string(tempVal));
        }

        bool replace = false;
        int idx = 0;
        for(idx = 0; idx < nkeys_old; idx++){
            int valc = strcmp(key,keysVec[idx].c_str());
            if(valc > 0) continue;
            if(valc == 0){
                replace = true;
                keysVec[idx] = string(key);
                valsVec[idx] = string(val);
                break;
            }
            if(valc < 0){
                break;
            }
        }
        
        if(!replace){
            // Insert at the correct position
            keysVec.insert(keysVec.begin() + idx, string(key));
            valsVec.insert(valsVec.begin() + idx, string(val));
        }

        uint16_t newnKeys = keysVec.size();

        const char** keyPtrs = new const char*[newnKeys];
        const char** valPtrs = new const char*[newnKeys];
        for(size_t i = 0; i < newnKeys; i++) {
            keyPtrs[i] = keysVec[i].c_str();
            valPtrs[i] = valsVec[i].c_str();
        }

        int64_t ptrs[1000] = {0};
        
        // new node is created here : after inserting the new key and value 
        createNode(newPageNum,type,newnKeys,ptrs,keyPtrs,valPtrs);
        
        delete[] keyPtrs;
        delete[] valPtrs;

        // old page is marked free
        freePage(oldPageNum);
    }

    // split the old node into 2 new node and split the middle key
    SplitResult nodeSplit2(uint32_t oldPageNum){
        uint16_t numKeys = getnkeys(oldPageNum);
        uint16_t type = getType(oldPageNum);
        vector<int64_t> allPtrs = getPtrs(oldPageNum);

        int splitPtr = numKeys / 2;

        vector<string> allKeys;
        vector<string> allVals;
        
        // temperory buffer to read keys and values
        char tempKey[PAGE_SIZE];
        char tempVal[PAGE_SIZE];

        for(int i = 0; i < numKeys; i++){
            getnthKV(oldPageNum, i, tempKey, tempVal);
            allKeys.push_back(string(tempKey));
            allVals.push_back(string(tempVal));
        }

        // increase the splitPtr till left size > PAGE_LIMIT
        int leftSize = 2 + 2;                               // type + nkeys
        leftSize += (splitPtr + 1) * 8;                     // pointers: 0 to splitPtr
        leftSize += splitPtr * 2;                           // offsets

        for(int i = 0; i < splitPtr; i++){
            leftSize += 4;                                  // 2+2 for sizes
            leftSize += allKeys[i].length();
            leftSize += allVals[i].length();
        } 

        while(leftSize > PAGE_LIMIT && splitPtr > 1){
            leftSize -= 4;
            leftSize -= (allKeys[splitPtr-1].length() + allVals[splitPtr-1].length()); 
            leftSize -= 8;                                  // pointer
            leftSize -= 2;                                  // offset
            splitPtr--;
        }

        string promotedKey = allKeys[splitPtr];

        vector<string> leftKeysVec, leftValsVec;
        vector<string> rightKeysVec, rightValsVec;
        vector<int64_t> leftPtrsVec, rightPtrsVec;

        // Keys/Vals: [0 .. splitPtr-1]
        for(int i = 0; i < splitPtr; i++){
            leftKeysVec.push_back(allKeys[i]);
            leftValsVec.push_back(allVals[i]);
        }
        // Pointers: [0 .. splitPtr]
        for(int i = 0; i <= splitPtr; i++){
            leftPtrsVec.push_back(allPtrs[i]);
        }

        // A key will be promoted only in LEAF node
        int startRight = (type == BNODE_NODE) ? splitPtr + 1 : splitPtr;
        
        for(int i = startRight; i < numKeys; i++){
            rightKeysVec.push_back(allKeys[i]);
            rightValsVec.push_back(allVals[i]);
        }
        
        // Pointers: [splitPtr+1 .. end]
        for(size_t i = splitPtr + 1; i < allPtrs.size(); i++){
            rightPtrsVec.push_back(allPtrs[i]);
        }

        const char** lKeysArr = new const char*[leftKeysVec.size()];
        const char** lValsArr = new const char*[leftValsVec.size()];
        for(size_t i=0; i<leftKeysVec.size(); i++) {
             lKeysArr[i] = leftKeysVec[i].c_str();
             lValsArr[i] = leftValsVec[i].c_str();
        }

        const char** rKeysArr = new const char*[rightKeysVec.size()];
        const char** rValsArr = new const char*[rightValsVec.size()];
        for(size_t i=0; i<rightKeysVec.size(); i++) {
             rKeysArr[i] = rightKeysVec[i].c_str();
             rValsArr[i] = rightValsVec[i].c_str();
        }
 
        // --- Write Pages ---
        uint32_t leftPage = allocatePage();
        createNode(leftPage, type, leftKeysVec.size(), leftPtrsVec.data(), lKeysArr, lValsArr);
        
        uint32_t rightPage = allocatePage();
        createNode(rightPage, type, rightKeysVec.size(), rightPtrsVec.data(), rKeysArr, rValsArr);

        delete[] lKeysArr; 
        delete[] lValsArr;
        delete[] rKeysArr; 
        delete[] rValsArr;

        SplitResult ans;            // ans is split result
        ans.leftPage = leftPage;
        ans.rightPage = rightPage;
        ans.promotedKey = promotedKey;

        freePage(oldPageNum);

        return ans;
    }

    void insert(string&key,string&val){
        if(getRoot() == 0){
            cout<<"Creating root for the first time"<<endl;
            root_page = allocatePage();
            cout<<"\troot_page : "<<root_page<<endl;
            int16_t type = BNODE_LEAF;
            int16_t nkeys = 1;
            int64_t ptrs[] = {0,0};
            const char*keys[] = {key.c_str()};
            const char* vals[] = {val.c_str()};

            createNode(root_page,type,nkeys,ptrs,keys,vals);
            
            updateHeader();
            return;
        }
        
        NodeSplitResult res = recursiveInsert(getRoot(),key,val);
        
        root_page = res.nodeCurrPageNum;
        
        if(!res.promotedKey.empty()){
            // new root creation
            root_page = allocatePage();
            int16_t type = BNODE_NODE;
            int16_t nkeys = 1;
            int64_t ptrs[] = {res.leftPage,res.rightPage};
            const char*keys[] = {res.promotedKey.c_str()};
            const char* vals[] = {""};
    
            createNode(root_page,type,nkeys,ptrs,keys,vals);
            
            updateHeader();
            return;
        }

        updateHeader();

    }

    NodeSplitResult recursiveInsert(uint32_t pageNum,string&key,string&val){
        uint16_t type = getType(pageNum);
        uint16_t nkeys = getnkeys(pageNum);

        NodeSplitResult res;

        if(type == BNODE_LEAF){
            uint32_t newPage = allocatePage();
            insertKVatNode(newPage,pageNum,key.c_str(),val.c_str());

            res.nodeCurrPageNum = newPage;

            uint16_t newnKeys = getnkeys(newPage);

            if(newnKeys > MAX_NKEYS){
                SplitResult resSplit = nodeSplit2(newPage);

                res.leftPage = resSplit.leftPage;
                res.rightPage = resSplit.rightPage;
                res.promotedKey = resSplit.promotedKey;
                res.nodeCurrPageNum = 0;

                return res;
            }else{
                return res;
            }
        }

        vector<string>keys = getKeys(pageNum);
        vector<int64_t>ptrs = getPtrs(pageNum);

        freePage(pageNum);

        int u = 0;

        // u = upper_bound() : implement later during optimization
        for(u;u<nkeys;u++){
            if(keys[u] <= key) continue;
            else break;
        }


        uint32_t nextPage = ptrs[u];

        NodeSplitResult res_from_child = recursiveInsert(nextPage,key,val);

        ptrs[u] = res_from_child.nodeCurrPageNum;

        if(res_from_child.promotedKey.empty()){
            // no promotion from child
            // update this node
            uint32_t newPage = allocatePage();
            res.nodeCurrPageNum = newPage;

            int64_t ptrRawArr[1000];
            const char** keysRaw = new const char*[keys.size()];
            const char** valsRaw = new const char*[keys.size()];

            for(int i = 0;i<ptrs.size();i++){
                ptrRawArr[i] = ptrs[i];
            }

            string emp = "";
            for(int i = 0;i<keys.size();i++){
                keysRaw[i] = keys[i].c_str();
                valsRaw[i] = emp.c_str();
            }

            createNode(newPage,BNODE_NODE,(int16_t)keys.size(),ptrRawArr,keysRaw,valsRaw);

            delete[] keysRaw;
            delete[] valsRaw;

            res.leftPage = 0;
            res.rightPage = 0;
            res.nodeCurrPageNum = newPage;
            res.promotedKey = "";

            return res;
        }

        // got a promoted key from child
        string promotedKey = res_from_child.promotedKey;

        vector<string>newKeys;
        vector<int64_t> newPtr;

        for(int i = 0;i<u;i++){
            newKeys.push_back(keys[i]);
        }
        newKeys.push_back(promotedKey);
        for(int i = u;i<keys.size();i++){
            newKeys.push_back(keys[i]);
        }

        for(int i = 0;i<u;i++){
            newPtr.push_back(ptrs[i]);
        }
        newPtr.push_back(res_from_child.leftPage);
        newPtr.push_back(res_from_child.rightPage);
        for(int i = u + 1;i<ptrs.size();i++){
            newPtr.push_back(ptrs[i]);
        }
        
        // create a node
        uint32_t newPage = allocatePage();
        int64_t ptrRawArr[1000];
        const char** keysRaw = new const char*[newKeys.size()];
        const char** valsRaw = new const char*[newKeys.size()];

        for(int i = 0;i<newPtr.size();i++){
            ptrRawArr[i] = newPtr[i];
        }

        string emp = "";
        for(int i = 0;i<newKeys.size();i++){
            // strcpy(keysRaw[i],newKeys[i].c_str());
            // strcpy(valsRaw[i],emp.c_str());
            keysRaw[i] = newKeys[i].c_str();
            valsRaw[i] = emp.c_str();
        }

        createNode(newPage,BNODE_NODE,(int16_t)newKeys.size(),ptrRawArr,keysRaw,valsRaw);
        
        delete[] keysRaw;
        delete[] valsRaw;

        res.leftPage = 0;
        res.rightPage = 0;
        res.nodeCurrPageNum = newPage;
        res.promotedKey = "";

        if( getnkeys(newPage) <= MAX_NKEYS ){
            return res;
        }

        // splitting

        SplitResult splitRes = nodeSplit2(newPage);
        res.leftPage = splitRes.leftPage;
        res.rightPage = splitRes.rightPage;
        res.promotedKey = splitRes.promotedKey;
        res.nodeCurrPageNum = 0;

        return res;
    }

    
    // binary search utility
    bool lbcheck(uint32_t pageNum,string&key,int idx){
        char key_idx[1000];
        char val_idx[1000];

        getnthKV(pageNum,idx,key_idx,val_idx);
        
        string key_idx_Str = string(key_idx);
        
        return key <= key_idx_Str;
    }
    
    
    bool ubcheck(uint32_t pageNum,string&key,int idx){
        char key_idx[1000];
        char val_idx[1000];

        getnthKV(pageNum,idx,key_idx,val_idx);
        
        string key_idx_Str = string(key_idx);
               
        return key < key_idx_Str;
    }

    int lowerBound(uint32_t pageNum,string&key){
        int nkeys = getnkeys(pageNum);
        int lo = 0,hi = nkeys - 1;
        int ans = hi + 1;

        while(lo <= hi){
            int mid = (lo + hi)/2;

            if(lbcheck(pageNum,key,mid)){
                ans = mid;
                hi = mid - 1;
            }else{
                lo = mid + 1;
            }
        }
        return ans;
    }

    int upperBound(uint32_t pageNum,string&key){
        int nkeys = getnkeys(pageNum);
        int lo = 0,hi = nkeys - 1;
        int ans = hi + 1;

        while(lo <= hi){
            int mid = (lo + hi)/2;

            if(ubcheck(pageNum,key,mid)){
                ans = mid;
                hi = mid - 1;
            }else{
                lo = mid + 1;
            }
        }
        return ans;
    }


    bool recursiveGet(string&key,string&val_out,uint64_t pageNum){
        uint16_t type = getType(pageNum);
        uint16_t nkeys = getnkeys(pageNum);

        if(type == BNODE_LEAF){
            int lb_idx = lowerBound(pageNum,key);

            if(lb_idx >= nkeys){
                return false;
            }

            char key_idx[1000];
            char val_idx[1000];
    
            getnthKV(pageNum,lb_idx,key_idx,val_idx);

            string ansVal = string(val_idx);
            string ansKey = string(key_idx);
            
            // cout << "[DEBUG] Compare:" << endl;
            // cout << "  Input Key: '" << key << "' (Len: " << key.length() << ")" << endl;
            // cout << "  Read Key : '" << ansKey << "' (Len: " << ansKey.length() << ")" << endl;
            // cout<<"[DEBUG]"<<lb_idx<<" : "<<ansKey<<" "<<key<<" "<<ansVal<<" status : "<<(key == ansKey)<<endl;
            

            if(ansKey == key){
                val_out = ansVal;
                return true;
            }

            return false;
        }

        int lb_idx = upperBound(pageNum,key);
        int64_t childPage = getnthPageNum(pageNum, lb_idx);
        if (childPage <= 0 || childPage >= getPageCount()) {
            return false;  // Invalid child pointer
        }
        return recursiveGet(key, val_out, childPage);
    }

    bool get(string&key,string&val_out){
        // cout<<"root : "<<getRoot()<<endl;
        if(getRoot() == 0){
            return false;
        }
        return recursiveGet(key,val_out,getRoot());
    }

    void set(string&key,string&val){
        insert(key,val);
    }

    void printTree() {
        if (getRoot() == 0) {
            cout << "Empty Tree\n";
            return;
        }
        cout << "--- B+ TREE STRUCTURE (Root: " << getRoot() << ") ---\n";
        printRecursive(getRoot(), 0);
        cout << "-------------------------------------------\n";
    }

    void printRecursive(uint32_t pageNum, int depth) {
        uint16_t type = getType(pageNum);
        uint16_t nkeys = getnkeys(pageNum);
        string indent(depth * 4, ' ');

        if (type == BNODE_LEAF) {
            cout << indent << "[LEAF Page " << pageNum << "] Keys: ";
            char k[1000], v[1000];
            for (int i = 0; i < nkeys; i++) {
                getnthKV(pageNum, i, k, v);
                cout << "\"" << k << "\" ";
            }
            cout << endl;
        } 
        else {
            // Internal Node
            cout << indent << "[NODE Page " << pageNum << "]\n";
            vector<string> keys = getKeys(pageNum);
            vector<int64_t> ptrs = getPtrs(pageNum);

            // Internal Node Format: Ptr0, Key0, Ptr1, Key1 ... PtrN
            for (int i = 0; i < nkeys; i++) {
                // Print Child i
                printRecursive(ptrs[i], depth + 1);
                
                // Print Key i (The Separator)
                cout << indent << "  KEY: \"" << keys[i] << "\"\n";
            }
            // Print Last Child
            printRecursive(ptrs[nkeys], depth + 1);
        }
    }

};

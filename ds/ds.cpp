#include "./ds.h"

DiskFile::DiskFile(const char*filename_) : filename(filename_){
    fd = open(filename,O_RDWR|O_CREAT,0644);
    if(fd < 0){
        perror("Open failed");
        exit(1);
    }

    // if these information is already on the location, then copy it from there else default value
    off_t fsize = lseek(fd, 0, SEEK_END);
    
    if (fsize == 0) {
        // New file: Initialize header
        char buff[PAGE_SIZE] = {0};
        lseek(fd,0,SEEK_SET);
        write(fd,buff,sizeof(buff));

        page_size = PAGE_SIZE;
        next_free_page = 0;
        total_page_alloted = 1;
        root_page = 0;
        updateHeader();
    } else {
        // Existing file: Read header
        lseek(fd, 0, SEEK_SET);
        read(fd, &page_size, sizeof(page_size));
        read(fd, &next_free_page, sizeof(next_free_page));
        read(fd, &total_page_alloted, sizeof(total_page_alloted));
        read(fd, &root_page, sizeof(root_page));
    }

}

DiskFile :: ~DiskFile(){
    if(fd >= 0){
        close(fd);
    }
}

void DiskFile ::updateHeader(){
    lseek(fd,0,SEEK_SET);
    write(fd,&page_size,sizeof(page_size));
    
    write(fd,&next_free_page,sizeof next_free_page);
    
    write(fd,&total_page_alloted,sizeof(total_page_alloted));
    
    write(fd,&root_page,sizeof(root_page));
}

bool DiskFile :: readPage(uint32_t pageNum,void*buff){
    off_t offset = lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
    if(offset == -1){
        return false;
    }
    ssize_t n = read(fd,buff,PAGE_SIZE);
    return n == PAGE_SIZE;
}


bool DiskFile :: writePage(uint32_t pageNum,const void*buff){
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

uint32_t DiskFile :: allocatePage(){
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

void DiskFile :: freePage(uint32_t pageNum){
    if(pageNum == 0) return;
    
    char buff[PAGE_SIZE] = {0};
    *(uint32_t*)buff = next_free_page;

    writePage(pageNum,buff);

    next_free_page =  pageNum;

    updateHeader();
}

uint32_t DiskFile :: appendPage(){

    off_t size = lseek(fd,0,SEEK_END);

    if(size == -1) return 0;

    uint32_t newPage = size/PAGE_SIZE;

    char zero[PAGE_SIZE] = {0};
    if(!writePage(newPage,zero)) return 0;
    return newPage;
}

uint32_t DiskFile::getPageCount(){
    struct stat st;
    if (fstat(fd, &st) == -1) return 0;
    return st.st_size / PAGE_SIZE;
}

int DiskFile::createNode(uint32_t pageNum,int16_t type,int16_t nkeys,int64_t ptrs[],const char*keys[],const char*vals[]){
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

uint16_t DiskFile::getnkeys(uint32_t pageNum){
    lseek(fd,pageNum*PAGE_SIZE + 2,SEEK_SET);
    uint16_t nkeys;
    read(fd,&nkeys,sizeof(nkeys));
    return nkeys;
}

uint16_t DiskFile::getType(uint32_t pageNum){
    lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
    uint16_t type;
    read(fd,&type,sizeof(type));
    return type;
}

uint32_t DiskFile::getRoot(){
    return root_page;
}

int64_t DiskFile::getnthPageNum(uint32_t pageNum,int n){
    lseek(fd,pageNum*PAGE_SIZE + 4 + n*8,SEEK_SET);
    int64_t nthPtr;
    read(fd,&nthPtr,sizeof(nthPtr));

    return nthPtr;
}

std::vector<int64_t> DiskFile::getPtrs(uint32_t pageNum){
    std::vector<int64_t> ptrVec;
    uint16_t numKeys = getnkeys(pageNum);

    lseek(fd,pageNum*PAGE_SIZE + 4,SEEK_SET);

    uint64_t temp;
    for(int i = 0;i<=numKeys;i++){
        read(fd,&temp,sizeof(temp));
        ptrVec.push_back(temp);
    }

    return ptrVec;
}

std::vector<std::string> DiskFile::getKeys(uint32_t pageNum){
    std::vector<std::string>keyVec;
    uint16_t numKeys = getnkeys(pageNum);

    char key[1000];
    char val[1000];

    for(int i = 0;i<numKeys;i++){
        getnthKV(pageNum,i,key,val);
        keyVec.push_back(std::string(key));
    }

    return keyVec;
    
}

void DiskFile::getnthKV(uint32_t pageNum,int n,char key[],char val[]){
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

void DiskFile::insertKVatNode(uint32_t newPageNum, uint32_t oldPageNum,const char key[],const char val[]){
    uint16_t nkeys_old = getnkeys(oldPageNum);
    uint16_t type = getType(oldPageNum);

    std::vector<std::string> keysVec;
    std::vector<std::string> valsVec;
    char tempKey[PAGE_SIZE]; // Temporary buffer for reading
    char tempVal[PAGE_SIZE];

    for(int i = 0;i<nkeys_old;i++){
        getnthKV(oldPageNum,i,tempKey,tempVal);
        keysVec.push_back(std::string(tempKey));
        valsVec.push_back(std::string(tempVal));
    }

    bool replace = false;
    int idx = 0;
    for(idx = 0; idx < nkeys_old; idx++){
        int valc = strcmp(key,keysVec[idx].c_str());
        if(valc > 0) continue;
        if(valc == 0){
            replace = true;
            keysVec[idx] = std::string(key);
            valsVec[idx] = std::string(val);
            break;
        }
        if(valc < 0){
            break;
        }
    }
    
    if(!replace){
        // Insert at the correct position
        keysVec.insert(keysVec.begin() + idx, std::string(key));
        valsVec.insert(valsVec.begin() + idx, std::string(val));
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

SplitResult DiskFile::nodeSplit2(uint32_t oldPageNum){
    uint16_t numKeys = getnkeys(oldPageNum);
    uint16_t type = getType(oldPageNum);
    std::vector<int64_t> allPtrs = getPtrs(oldPageNum);

    int splitPtr = numKeys / 2;

    std::vector<std::string> allKeys;
    std::vector<std::string> allVals;
    
    // temperory buffer to read keys and values
    char tempKey[PAGE_SIZE];
    char tempVal[PAGE_SIZE];

    for(int i = 0; i < numKeys; i++){
        getnthKV(oldPageNum, i, tempKey, tempVal);
        allKeys.push_back(std::string(tempKey));
        allVals.push_back(std::string(tempVal));
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

    std::string promotedKey = allKeys[splitPtr];

    std::vector<std::string> leftKeysVec, leftValsVec;
    std::vector<std::string> rightKeysVec, rightValsVec;
    std::vector<int64_t> leftPtrsVec, rightPtrsVec;

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

void DiskFile::insert(std::string&key,std::string&val){
    if(getRoot() == 0){
        std::cout<<"Creating root for the first time"<<std::endl;
        root_page = allocatePage();
        std::cout<<"\troot_page : "<<root_page<<std::endl;
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

NodeSplitResult DiskFile::recursiveInsert(uint32_t pageNum,std::string&key,std::string&val){
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

    std::vector<std::string>keys = getKeys(pageNum);
    std::vector<int64_t>ptrs = getPtrs(pageNum);

    freePage(pageNum);

    
    int u;
    // u = upper_bound() : implement later during optimization
    for(u = 0;u<nkeys;u++){
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

        for(int i = 0;i<(int)ptrs.size();i++){
            ptrRawArr[i] = ptrs[i];
        }

        std::string emp = "";
        for(int i = 0;i<(int)keys.size();i++){
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
    std::string promotedKey = res_from_child.promotedKey;

    std::vector<std::string>newKeys;
    std::vector<int64_t> newPtr;

    for(int i = 0;i<u;i++){
        newKeys.push_back(keys[i]);
    }
    newKeys.push_back(promotedKey);
    for(int i = u;i<(int)keys.size();i++){
        newKeys.push_back(keys[i]);
    }

    for(int i = 0;i<u;i++){
        newPtr.push_back(ptrs[i]);
    }
    newPtr.push_back(res_from_child.leftPage);
    newPtr.push_back(res_from_child.rightPage);
    for(int i = u + 1;i<(int)ptrs.size();i++){
        newPtr.push_back(ptrs[i]);
    }
    
    // create a node
    uint32_t newPage = allocatePage();
    int64_t ptrRawArr[1000];
    const char** keysRaw = new const char*[newKeys.size()];
    const char** valsRaw = new const char*[newKeys.size()];

    for(int i = 0;i<(int)newPtr.size();i++){
        ptrRawArr[i] = newPtr[i];
    }

    std::string emp = "";
    for(int i = 0;i<(int)newKeys.size();i++){
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

bool DiskFile::lbcheck(uint32_t pageNum,std::string&key,int idx){
    char keyIdx[1000];
    char valIdx[1000];

    getnthKV(pageNum,idx,keyIdx,valIdx);
    
    std::string key_idx_Str = std::string(keyIdx);
    
    return key <= key_idx_Str;
}

bool DiskFile::ubcheck(uint32_t pageNum,std::string&key,int idx){
    char keyIdx[1000];
    char valIdx[1000];

    getnthKV(pageNum,idx,keyIdx,valIdx);
    
    std::string key_idx_Str = std::string(keyIdx);
            
    return key < key_idx_Str;
}

int DiskFile::lowerBound(uint32_t pageNum,std::string&key){
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

int DiskFile::upperBound(uint32_t pageNum,std::string&key){
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

bool DiskFile::recursiveGet(std::string&key,std::string&val_out,uint64_t pageNum){
    uint16_t type = getType(pageNum);
    uint16_t nkeys = getnkeys(pageNum);

    if(type == BNODE_LEAF){
        int lb_idx = lowerBound(pageNum,key);

        if(lb_idx >= nkeys){
            return false;
        }

        char keyIdx[1000];
        char valIdx[1000];

        getnthKV(pageNum,lb_idx,keyIdx,valIdx);

        std::string ansVal = std::string(valIdx);
        std::string ansKey = std::string(keyIdx);
        
        
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

bool DiskFile::get(std::string&key,std::string&val_out){
    if(getRoot() == 0){
        return false;
    }
    return recursiveGet(key,val_out,getRoot());
}

void DiskFile::set(std::string&key,std::string&val){
    insert(key,val);
}

void DiskFile::printTree() {
    if (getRoot() == 0) {
        std::cout << "Empty Tree\n";
        return;
    }
    std::cout << "--- B+ TREE STRUCTURE (Root: " << getRoot() << ") ---\n";
    printRecursive(getRoot(), 0);
    std::cout << "-------------------------------------------\n";
}

void DiskFile::printRecursive(uint32_t pageNum, int depth) {
    uint16_t type = getType(pageNum);
    uint16_t nkeys = getnkeys(pageNum);
    std::string indent(depth * 4, ' ');

    if (type == BNODE_LEAF) {
        std::cout << indent << "[LEAF Page " << pageNum << "] Keys: ";
        char k[1000], v[1000];
        for (int i = 0; i < nkeys; i++) {
            getnthKV(pageNum, i, k, v);
            std::cout << "\"" << k << "\" ";
        }
        std::cout << std::endl;
    } 
    else {
        // Internal Node
        std::cout << indent << "[NODE Page " << pageNum << "]\n";
        std::vector<std::string> keys = getKeys(pageNum);
        std::vector<int64_t> ptrs = getPtrs(pageNum);

        // Internal Node Format: Ptr0, Key0, Ptr1, Key1 ... PtrN
        for (int i = 0; i < nkeys; i++) {
            // Print Child i
            printRecursive(ptrs[i], depth + 1);
            
            // Print Key i (The Separator)
            std::cout << indent << "  KEY: \"" << keys[i] << "\"\n";
        }
        // Print Last Child
        printRecursive(ptrs[nkeys], depth + 1);
    }
}


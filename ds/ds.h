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

#define BNODE_NODE  1
#define BNODE_LEAF 2

const int PAGE_SIZE = 4096;
const int PAGE_LIMIT = 2048;

const int MAX_NKEYS = 100;

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

    // fsync to disk
    void syncToDisk();

    // update the Header page with current values
    void updateHeader();

    // Read the page number, and store it in buffer, return true if read successfully
    bool readPage(uint32_t pageNum,void*buff);
    
    // Write at pagenum, content of buff
    bool writePage(uint32_t pageNum,const void*buff);


    // return a free page number : either from free list or newly created page
    uint32_t allocatePage();

    // Mark the pageNum as free and add it to free_page linked list
    void freePage(uint32_t pageNum);

    // Adds a new page in the book
    uint32_t appendPage();

    // returns total number of pages Attached
    uint32_t getPageCount();


    // input : a free page number, type of node it is, number of keys in it, list of pointers, list of keys and list of values
    // output : returns true if created successfully
    int createNode(uint32_t pageNum,int16_t type,int16_t nkeys,int64_t ptrs[],const char*keys[],const char*vals[]);

    // return number of keys (nkeys) of a node stored at pageNum 
    uint16_t getnkeys(uint32_t pageNum);
    
    // return type of node at pagenum
    uint16_t getType(uint32_t pageNum);

    // return root_page = page number of root of B+ tree
    uint32_t getRoot();

    // returns the nth POINTER (is basycally page number)
    // zero indexed
    int64_t getnthPageNum(uint32_t pageNum,int n);

    std::vector<int64_t> getPtrs(uint32_t pageNum);

    std::vector<std::string> getKeys(uint32_t pageNum);

    // GET nTH KEY (0 INDEXED)
    void getnthKV(uint32_t pageNum,int n,char key[],char val[]);

    // CoW pattern : create a new node at newPageNum, dont alter the oldPageNum
    void insertKVatNode(uint32_t newPageNum, uint32_t oldPageNum,const char key[],const char val[]);

    // split the old node into 2 new node and split the middle key
    SplitResult nodeSplit2(uint32_t oldPageNum);

    void insert(std::string&key,std::string&val);

    NodeSplitResult recursiveInsert(uint32_t pageNum,std::string&key,std::string&val);

    
    // binary search utility
    bool lbcheck(uint32_t pageNum,std::string&key,int idx);
    
    
    bool ubcheck(uint32_t pageNum,std::string&key,int idx);

    int lowerBound(uint32_t pageNum,std::string&key);

    int upperBound(uint32_t pageNum,std::string&key);


    bool recursiveGet(std::string&key,std::string&val_out,uint64_t pageNum);

    bool get(std::string&key,std::string&val_out);

    void set(std::string&key,std::string&val);

    void printTree();

    void printRecursive(uint32_t pageNum, int depth);

};

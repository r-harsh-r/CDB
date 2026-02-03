#include<iostream>
#include<string>
#include<fcntl.h>
#include<unistd.h>
#include<libgen.h>
#include<cstring>
#include<cstdint>
#include<sys/stat.h>
#include<vector>
#include "../ds/ds.h"

using namespace std;

int main(){
    string path = "../DB/test_db";

    {
        cout << " TEST 1: Basic Insertion \n";
        DiskFile DF(path.c_str()); // Creates new file
        
        string k1 = "a", v1 = "Alice";
        string k2 = "c", v2 = "Charlie";
        string k3 = "b", v3 = "Bob";

        cout << "Inserting a, c, b\n";
        DF.insert(k1, v1);
        DF.insert(k2, v2);
        DF.insert(k3, v3);

        DF.printTree();
    }

    // return 0;

    {
        cout << "\n TEST 2: Leaf Split (Overflow) \n";
        DiskFile DF(path.c_str()); 

        for (int i = 10; i < 25; i++) {
            string key;
            key.push_back('a' + i);
            string val = "data";
            DF.insert(key, val);

            cout<<"\tinserted : "<<key<<" "<<val<<endl;
            DF.printTree();
            cout<<endl;
        }

        DF.printTree();
    }
    return 0;
    {
        cout << "\n TEST 3: Stress Test (Root Split) \n";
        DiskFile DF(path.c_str());

        cout << "Inserting 100 keys\n";
        for (int i = 100; i < 200; i++) {
            string key = "key:" + to_string(i);
            string val = "val";
            DF.insert(key, val);
        }

        DF.printTree();
    }

    return 0;
}
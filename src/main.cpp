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
    string path = "./DB/test_db";

    DiskFile DF(path.c_str());

    while(true){
        string choice;
        cin>>choice;

        if(choice == "GET"){
            string key,val; 
            cin >> key;
            bool status = DF.get(key,val);
            // cout<<"[DEBUG]"<<status<<endl;
            if(!status){
                cout<<"\tKey not found!\n";
                continue;
            }else{
                cout<<"\t"<<key<<" : "<<val<<endl;
            }
            continue;
        }

        if(choice == "SET"){
            string key,val;
            cin>>key>>val;

            DF.set(key,val);
        }

        if(choice == "PRINT"){
            DF.printTree();
        }

    }
    
    return 0;
}
#include <BTreeIndex.h>
#include <RecordFile.h>
#include <test_util.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <set>

static void generateTestFileRecordFile(std::string filename,
                                       RecordFile& rf, 
                                       int         range);
                                       
static void generateEmptyTestIndexFile(std::string filename,
                                       PageFile&   pf);
                                       
static void print_index(BTreeIndex& index, RecordFile& rf,int startKey);

int main( int argc, const char* argv[] )
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int testStatus = 0;
    PageFile index_file;
    index_file.close();
    switch (test)
    {
        // Breadth Test
        // Excersise some basic functionality, test nothings
        case 0: 
        {
            BTreeIndex bt_index;
            generateEmptyTestIndexFile("index_file.txt", index_file);
            ASSERT(0 == bt_index.open("index_file.txt", 'w'));
            int range = 4096;
            std::cout << "Breadth Test" << std::endl;
            std::string value;
            IndexCursor cursor = {0,0};
            int key;
            
            RecordId rid;
            RecordFile rf;
            generateTestFileRecordFile("testRecordFile.txt", rf, range);
            
            ASSERT(0 != bt_index.locate(1,cursor));
            
            size_t i = 0;
            size_t j = 0;
            size_t count = 0;
            while (count < range)
            {
                rid.pid = i;
                for (j = 0; j < RecordFile::RECORDS_PER_PAGE; ++j)
                {
                    if (count >= range)
                    {
                        break;
                    }
                    rid.sid = j;
                    ASSERT(0 == rf.read(rid, key, value));
                    ASSERT(0 == bt_index.insert(key, rid));   
                    count++;
                }
                i++;
            }
            ASSERT(0 == bt_index.close());
        } break;
        case 1: {
            std::cout << "Random Access Test" << std::endl;
            BTreeIndex bt_index;
            generateEmptyTestIndexFile("index_file.txt", index_file);
            ASSERT(0 == bt_index.open("index_file.txt", 'w'));
            std::set<int> old_keys;
            int range = 4096;
            int key;
            RecordFile rf;
            RecordId rid;
            IndexCursor cursor = {0,0};
            std::string value;
            generateTestFileRecordFile("testRecordFile.txt", rf, range);
            
            ASSERT(0 != bt_index.locate(1,cursor));
            size_t i = 0;
            size_t j = 0;
            size_t count = 0;
            while (count < range)
            {
                rid.pid = i;
                for (j = 0; j < RecordFile::RECORDS_PER_PAGE; ++j)
                {
                    if (count >= range)
                    {
                        break;
                    }
                    rid.sid = j;
                    do
                    {
                        key = rand() % range;
                    }
                    while(old_keys.find(key) != old_keys.end());
                    old_keys.insert(key);
                    ASSERT(0 == rf.read(rid, key, value));
                    ASSERT(0 == bt_index.insert(key, rid));   
                    count++;
                }
                i++;
            }
            ASSERT(0 == bt_index.close());
        } break;
        
        default: {
            std::cerr << "WARNING: CASE `" << test << "' NOT FOUND." << std::endl;
            testStatus = -1;
      } break;
    }
    return testStatus;
}
                                     
static void generateTestFileRecordFile(std::string filename,
                                       RecordFile& rf, 
                                       int         range)
{
    RecordId rid = {0,0};
    unlink(filename.c_str());
    rf.open(filename, 'W');
    for (int i = 1; i < range + 1; ++i)
    {
        char c[3] = {'a' + i % 26, '\0'};
        ASSERT(0 == rf.append(i, std::string(c), rid));
    }
}

static void generateEmptyTestIndexFile(std::string filename,
                                       PageFile&   pf)
{
    unlink(filename.c_str());
    pf.open(filename, 'W');
}

static void print_index(BTreeIndex& index, RecordFile& rf,int startKey)
{
    IndexCursor cursor;
    index.locate(startKey, cursor);
    int currKey;
    RecordId currRecord;
    std::string currValue;
    printf("=================================\n");
    while(0 == index.readForward(cursor, currKey, currRecord))
    {
        ASSERT(0 == rf.read(currRecord, currKey, currValue));
        printf("\tRecordId: {%d, %d} = (%d, %s}\n", currRecord.pid, 
                                                  currRecord.sid, 
                                                  currKey, 
                                                  currValue.c_str());
    }
    printf("\n\n");
}

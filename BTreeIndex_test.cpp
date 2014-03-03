#include <BTreeIndex.h>
#include <RecordFile.h>
#include <test_util.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <vector>

static void generateTestFileRecordFile(std::string filename,
                                       RecordFile& rf, 
                                       int         range);
                                       
static void generateEmptyTestIndexFile(std::string filename,
                                       PageFile&   pf);

int main( int argc, const char* argv[] )
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int testStatus = 0;
    PageFile index_file;
    generateEmptyTestIndexFile("index_file.txt", index_file);
    index_file.close();
    switch (test)
    {
        // Breadth Test
        // Excersise some basic functionality, test nothings
        case 0: 
        {
            BTreeIndex bt_index;
            ASSERT(0 == bt_index.open("index_file.txt", 'w'));
            int range = 12;
            std::cout << "Breadth Test" << std::endl;
            std::ostringstream sout;
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
                    ASSERT(0 == bt_index.insert(key + 1, rid));                
                    count++;
                }
                i++;
            }
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
        char c[3] = {'a' + i % 26,'\n', '\0'};
        ASSERT(0 == rf.append(i, std::string(c), rid));
    }
}

static void generateEmptyTestIndexFile(std::string filename,
                                       PageFile&   pf)
{
    unlink(filename.c_str());
    pf.open(filename, 'W');
}


#include "BTreeNode.h"
#include "test_util.h"
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <vector>
static void generateTestFile(std::string filename, RecordFile& rf, int range);

int main( int argc, const char* argv[] )
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int testStatus = 0;
    switch (test)
    {
        // Breadth Test
        // Excersise some basic functionality, test nothings
        case 0: 
        {
            int range = 4096;
            std::cout << "Breadth Test" << std::endl;
            std::ostringstream sout;
            std::string value;
            int eid;
            int key;
            
            RecordId rid;
            RecordFile rf;
            generateTestFile("testFile.txt", rf, range);
            std::vector<BTLeafNode *> LeafNodes;
            LeafNodes.push_back(new BTLeafNode);
            BTLeafNode* bt = *(LeafNodes.end() - 1);
            
            ASSERT(0 == bt->getKeyCount());
            ASSERT(0 != bt->locate(0,eid));
            
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
                    if (     bt->getKeyCount() != 0 && 
                        0 == bt->getKeyCount() % BTLeafNode::getMaxKeyCount())
                    {
                        LeafNodes.push_back(new BTLeafNode);
                        BTLeafNode *sibling = *(LeafNodes.end() - 1);
                        ASSERT(0 == bt->insertAndSplit(count/2, rid, *sibling, key));
                        ASSERT(sibling->getKeyCount() ==  bt->getKeyCount())
                        bt = sibling;
                        ASSERT(bt->insert(key + 1, rid) == 0);   
                    }
                    else
                    {
                        ASSERT(bt->insert(key + 1, rid) == 0);
                    }                  
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

static void generateTestFile(std::string filename, RecordFile& rf, int range)
{
    RecordId rid = {0,0};
    unlink(filename.c_str());
    rf.open(filename, 'w');
    for (int i = 0; i < range; ++i)
    {
        char c[3] = {'a' + i % 26,'\n', '\0'};
        ASSERT(0 == rf.append(i + 1, std::string(c), rid));
    }
}

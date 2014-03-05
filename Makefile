SRC = main.cc SqlParser.tab.c lex.sql.c SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc 
HDR = Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.tab.h
BTreeNodeTestSRC = BTreeNode.cc BTreeNode_test.cpp RecordFile.cc PageFile.cc
BTreeIndexTestSRC = BTreeIndex.cc BTreeIndex_test.cpp RecordFile.cc PageFile.cc  BTreeNode.cc

all: BTreeIndexTest BTreeNodeTest bruinbase

bruinbase: $(SRC) $(HDR)
	g++ -ggdb -o $@ $(SRC)

lex.sql.c: SqlParser.l
	flex -Psql $<

SqlParser.tab.c: SqlParser.y
	bison -d -psql $<

BTreeNodeTest: $(BTreeNodeTestSRC) test_util.h
	g++ -I. -ggdb -o $@ $(BTreeNodeTestSRC)
    
BTreeIndexTest: $(BTreeIndexTestSRC) test_util.h
	g++ -I. -ggdb -o $@ $(BTreeIndexTestSRC)

clean:
	rm -f bruinbase bruinbase.exe BTreeNodeTest BTreeIndexTest *.o *~ lex.sql.c SqlParser.tab.c SqlParser.tab.h 

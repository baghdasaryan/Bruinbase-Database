SRC = main.cc SqlParser.tab.c lex.sql.c SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc 
HDR = Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.tab.h
BTreeTestSRC = BTreeNode.cc BTreeNode_test.cpp RecordFile.cc PageFile.cc

bruinbase: $(SRC) $(HDR)
	g++ -ggdb -o $@ $(SRC)

lex.sql.c: SqlParser.l
	flex -Psql $<

SqlParser.tab.c: SqlParser.y
	bison -d -psql $<

BTreeTest: $(BTreeTestSRC)
	g++ -I. -ggdb -o $@ $(BTreeTestSRC)

clean:
	rm -f bruinbase bruinbase.exe BTreeTest *.o *~ lex.sql.c SqlParser.tab.c SqlParser.tab.h 

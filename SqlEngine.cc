/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;       // RecordFile containing the table
  RecordId   rid;      // record cursor for table scanning
  BTreeIndex bti;      // BTree Index for iterating through the index
  IndexCursor cursor;  // cursor for scanning index contents

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;
  int    index;

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }

  count = 0;
  if (bti.open(table + ".idx", 'r')) {
    // scan the table file from the beginning
    rid.pid = rid.sid = 0;
    while (rid < rf.endRid()) {
      // read the tuple
      if ((rc = rf.read(rid, key, value)) < 0) {
        fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
        goto exit_select;
      }

      // check the conditions on the tuple
      for (unsigned i = 0; i < cond.size(); i++) {
        // compute the difference between the tuple value and the condition value
        switch (cond[i].attr) {
        case 1:
          diff = key - atoi(cond[i].value);
          break;
        case 2:
    	  diff = strcmp(value.c_str(), cond[i].value);
	      break;
        }

        // skip the tuple if any condition is not met
        switch (cond[i].comp) {
        case SelCond::EQ:
          if (diff != 0) goto next_tuple;
          break;
        case SelCond::NE:
          if (diff == 0) goto next_tuple;
          break;
        case SelCond::GT:
          if (diff <= 0) goto next_tuple;
          break;
        case SelCond::LT:
          if (diff >= 0) goto next_tuple;
          break;
        case SelCond::GE:
          if (diff < 0) goto next_tuple;
          break;
        case SelCond::LE:
          if (diff  > 0) goto next_tuple;
          break;
        }
      }

      // the condition is met for the tuple. 
      // increase matching tuple counter
      count++;

      // print the tuple 
      switch (attr) {
      case 1:  // SELECT key
        fprintf(stdout, "%d\n", key);
        break;
      case 2:  // SELECT value
        fprintf(stdout, "%s\n", value.c_str());
        break;
      case 3:  // SELECT *
        fprintf(stdout, "%d '%s'\n", key, value.c_str());
        break;
      }

      // move to the next tuple
      next_tuple:
      ++rid;
    }
  } else {
    index = -1;
    for (unsigned i = 0; i < cond.size(); i++) {
      // Skip value columns
      if (cond[i].attr != 1)
        continue;

      if (cond[i].comp == SelCond::EQ) {
        index = i;
        break;
      }

      if ((cond[i].comp == SelCond::GT || cond[i].comp == SelCond::GE) &&
          (index == -1 || atoi(cond[i].value) > atoi(cond[index].value)))
        index = i;
    }

    if (index > -1)
      bti.locate(atoi(cond[index].value), cursor);
    else
      bti.locate(0, cursor);

    while ((bti.readForward(cursor, key, rid)) == 0) {
      // read the tuple
      if ((rc = rf.read(rid, key, value)) < 0) {
        fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
        goto exit_select;
      }

      // check the conditions on the tuple
      for (unsigned i = 0; i < cond.size(); i++) {
        // compute the difference between the tuple value and the condition value
        switch (cond[i].attr) {
        case 1:
          diff = key - atoi(cond[i].value);
          break;
        case 2:
    	  diff = strcmp(value.c_str(), cond[i].value);
	      break;
        }

        // check the condition
        switch (cond[i].comp) {
        case SelCond::EQ:
          if (diff != 0) {
            if (cond[i].attr == 1) goto print_and_exit;
            else continue;
          }
          break;
        case SelCond::NE:
          if (diff == 0) continue;
          break;
        case SelCond::GT:
          if (diff <= 0) continue;
          break;
        case SelCond::LT:
          if (diff >= 0) {
            if (cond[i].attr == 1) goto print_and_exit;
            else continue;
          }
          break;
        case SelCond::GE:
          if (diff < 0) continue;
          break;
        case SelCond::LE:
          if (diff > 0) {
            if (cond[i].attr == 1) goto print_and_exit;
            else continue;
          }
          break;
        }
      }

      // the condition is met for the tuple. 
      // increase matching tuple counter
      count++;

      // print the tuple 
      switch (attr) {
      case 1:  // SELECT key
        fprintf(stdout, "%d\n", key);
        break;
      case 2:  // SELECT value
        fprintf(stdout, "%s\n", value.c_str());
        break;
      case 3:  // SELECT *
        fprintf(stdout, "%d '%s'\n", key, value.c_str());
        break;
      }
    }
  }

  print_and_exit:
  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
  exit_select:
  rf.close();
  return rc;
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning
  BTreeIndex bti;  // BTree Index for inserting indices
  ifstream ifs;    // Input file stream for the load file

  int    ret;
  int    key;     
  string value;
  string line;

  // open the table file
  if ((ret = rf.open(table + ".tbl", 'w')) < 0) {
    fprintf(stderr, "Error: Cannot access/create table %s\n", table.c_str());
    return ret;
  }

  // open an index file
  if (index) {
    if (ret = bti.open(table + ".idx", 'w')) {
      fprintf(stderr, "Error: Cannot access/create %s index file\n", loadfile.c_str());
      goto exit_load;
    }
  }

  // open the load file
  ifs.open(loadfile.c_str(), ifstream::in);
  if (ret = !ifs.is_open()) {
    fprintf(stderr, "Error: Cannot open %s file\n", loadfile.c_str());
    goto exit_load;
  }

  // read load file line by line and insert, parse and insert tuples into record file and index
  getline(ifs, line);
  for (unsigned lineNum = 1; ifs.good(); lineNum++) {
    if (parseLoadLine(line, key, value)) {
      fprintf(stderr, "Warning: Could not parse line %u from file %s\n", lineNum, loadfile.c_str());
      goto next_line;
    }

    if (rf.append(key, value, rid)) {
      fprintf(stderr, "Warning: Could not insert tuple with key %i into %s RecordFile\n", key, table.c_str());
      goto next_line;
    }

    if (index) {
      if (bti.insert(key, rid)) {
        fprintf(stderr, "Warning: Could not insert key %i into index\n", key);
        goto next_line;
      }
    }
    next_line:
    getline(ifs, line);
  }
  ret = 1;

  // close files and streams and return
  exit_load:
  rf.close();
  ifs.close();
  if (index) {
    bti.close();
  }

  return ret;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}

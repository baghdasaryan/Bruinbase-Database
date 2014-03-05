#include "BTreeNode.h"
using namespace std;

/**
 * Class constructor.
 * Clears the buffer and computes maxKeyCount.
 */
BTLeafNode::BTLeafNode()
  : maxKeyCount((PageFile::PAGE_SIZE - sizeof(PageId)) / (sizeof(NodeEntry)))
{
  bzero(buffer, PageFile::PAGE_SIZE);
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
  return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
  return pf.write(pid, buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
  NodeEntry *ne = (NodeEntry *) buffer;

  int count;
  for (count = 0; (ne->key != 0) && (count < maxKeyCount); count++, ne++) {
    ;
  }

  return count;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
  int nodeId = 0;
  NodeEntry* newEntry = NULL;
  NodeEntry* curEntry = (NodeEntry *) buffer + getKeyCount();

  if (getKeyCount() >= maxKeyCount)
    return RC_NODE_FULL;

  // Use the end of the node if can't locate an appropriate entry
  if (locate(key, nodeId)) {
    newEntry = curEntry;
  } else {
    newEntry = (NodeEntry *) buffer + nodeId;

    // Shift node entries to the right
    while (curEntry != newEntry) {
      NodeEntry* nextEntry = curEntry - 1;
      *curEntry = *nextEntry;
      curEntry = nextEntry;
    }
  }

  // Insert data
  newEntry->key = key;
  newEntry->rid = rid;

  return 0;
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
  RC rc = 0;
  int index = 0;
  int total = getKeyCount();
  int half = (total + 1) / 2;

  rc = locate(key, index);
  if (rc == RC_END_OF_TREE)
    index = total;
  else if (rc != 0)
    return rc;

  NodeEntry ne = { key, rid }; // new entry

  for (NodeEntry* cur = (NodeEntry *) buffer + index; index < half; cur++, index++) {
    NodeEntry tmp = *cur;
    *cur = ne;
    ne = tmp;
  }
 
  if (index == half)
    siblingKey = ne.key;
  else
    siblingKey = ((NodeEntry *) buffer + half)->key;

  sibling.insert(ne.key, ne.rid);

  for (NodeEntry* cur = (NodeEntry *) buffer + half; half < total; cur++, half++) {
    sibling.insert(cur->key, cur->rid);
    cur->key = 0;
  }

  return 0;
}

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equal to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
  NodeEntry* ne = (NodeEntry *) buffer;
  for (eid = 0; eid < getKeyCount() && ne->key < searchKey; eid++, ne++) {
    ;
  }

  if (eid == getKeyCount()) {
    eid = -1;
    return RC_END_OF_TREE;
  }

  return 0;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
  if (eid < 0 || eid >= getKeyCount())
    return RC_INVALID_CURSOR;

  NodeEntry* ne = (NodeEntry *) buffer + eid;
  key = ne->key;
  rid = ne->rid;
  return 0;
}

/*
 * Return the pid of the next sibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{
  PageId* pid = (PageId *) (buffer + PageFile::PAGE_SIZE) - 1;
  return *pid;
}

/*
 * Set the pid of the next sibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
  PageId* ptr = (PageId *) (buffer + PageFile::PAGE_SIZE) - 1;
  *ptr = pid;
  return 0;
}

/**
 * Class constructor.
 * Computes maxKeyCount.
 */
BTNonLeafNode::BTNonLeafNode()
  : maxKeyCount((PageFile::PAGE_SIZE - sizeof(PageId)) / (sizeof(NodeEntry)))
{
  ;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
  return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
  return pf.write(pid, buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
  NodeEntry *ne = (NodeEntry *) buffer;

  int count;
  for (count = 0; (ne->key != 0) && (count < maxKeyCount); count++, ne++) {
    ;
  }

  return count;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
  int nodeId = 0;
  NodeEntry* newEntry = NULL;
  NodeEntry* curEntry = (NodeEntry *) buffer + getKeyCount();

  if (getKeyCount() >= maxKeyCount)
    return RC_NODE_FULL;

  // Use beginning of the node if can't locate an appropriate entry
  if (locateChildPtr(key, nodeId)) {
    newEntry = (NodeEntry *) buffer;
  } else {
    newEntry = (NodeEntry *) buffer + nodeId + 1;

    // Shift node entries to the right
    while (curEntry != newEntry) {
      NodeEntry* nextEntry = curEntry - 1;
      *curEntry = *nextEntry;
      curEntry = nextEntry;
    }
  }

  // Insert data
  newEntry->key = key;
  newEntry->pid = pid;

  return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
  RC rc = 0;
  int index = 0;
  int total = getKeyCount();
  int half = (total + 1) / 2;

  rc = locateChildPtr(key, index);
  if (rc == RC_END_OF_TREE)
    index = total;
  else if (rc != 0)
    return rc;

  index++;
  NodeEntry ne = { key, pid }; // new entry

  for (NodeEntry* cur = (NodeEntry *) buffer + index; index < half; cur++, index++) {
    NodeEntry tmp = *cur;
    *cur = ne;
    ne = tmp;
  }
 
  if (index != half) {
    NodeEntry* cur = (NodeEntry *) buffer + half;
    NodeEntry tmp = *cur;
    *cur = ne;
    ne = tmp;
  }

  midKey = ne.key;

  NodeEntry* cur = (NodeEntry *)buffer + half;
  sibling.initializeRoot(ne.pid, cur->key, cur->pid);
  cur->key = 0;
  half++;

  for (NodeEntry* cur = (NodeEntry *) buffer + half; half < total; cur++, half++) {
    sibling.insert(cur->key, cur->pid);
    cur->key = 0;
  }

  return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output its index.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param eid[OUT] the index of child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, int& eid)
{
  NodeEntry* ne = (NodeEntry *) buffer ;
  for (eid = getKeyCount() - 1; eid >= 0 && ne->key > searchKey; eid--, ne--) {
    ;
  }

  if (eid == -1) {
    return RC_END_OF_TREE;
  }

  return 0;
}

/**
 * Read the pid from the eid entry.
 * @param eid[IN] the entry number to read the pid from
 * @param pid[OUT] the PageId from the slot
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::readEntry(int eid, PageId& pid)
{
  if (eid >= getKeyCount())
    return RC_INVALID_CURSOR;

  if (eid < 0) {
    PageId *ptr = (PageId *) (buffer + PageFile::PAGE_SIZE - sizeof(PageId));
    pid = *ptr;
  } else {
    NodeEntry* ne = (NodeEntry *) buffer + eid;
    pid = ne->pid;
  }
 
  return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
  bzero(buffer, PageFile::PAGE_SIZE);

  NodeEntry root = { key, pid2 };
  *((NodeEntry *) buffer) = root;

  PageId *ptr1 = (PageId *) (buffer + PageFile::PAGE_SIZE - sizeof(PageId));
  *ptr1 = pid1;

  return 0;
}

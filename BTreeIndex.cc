/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight = 0;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
    /* Another way to open an index file:
     * 
     * Use RecordId (size ~ 8 bytes) instead of data[PageFile::PAGE_SIZE]
     * (PAGE_SIZE ~ 1024 bytes) as shown in BTreeIndex::close() --> this
     * might save some memory
     */

    RC rc;
    // Open index file
    if ((rc = pf.open(indexname, mode)) != 0)
       return rc;

    // Load root Pid and the tree height
    char data[PageFile::PAGE_SIZE];
    if (pf.endPid() == 0) {   // empty file
        // Empty tree
        rootPid = -1;
        treeHeight = 0;

        // Put a placeholder for rootPid and treeHeight
        if ((rc = pf.write(0, data)) != 0) {
            return rc;
        }
    } else {
        // Try to read previously stored data
        if ((rc = pf.read(0, data)) != 0) {
            return rc;
        }

        // Assign rootPid and treeHeight
        rootPid = *((PageId *) data);
        treeHeight = *((int *) (data + sizeof(PageId)));
    }

    return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    // Prepare root Pid and the tree height
    char dataToStore[PageFile::PAGE_SIZE];
    *((PageId *) dataToStore) = rootPid;
    *((int *) (dataToStore + sizeof(PageId))) = treeHeight;

    /* Another way to do this:
     * 
    RecordId dataToStore = { rootPid, treeHeight };
     */

    // Store data
    pf.write(0, dataToStore);

    return pf.close();
}

/*
 * Insert (key, RecordId) pair at the root level.
 * @warning This function should not be called directly.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insertAtRoot(int key, const RecordId& rid)
{
    RC rc;

    // Insert data into a new root node
    BTLeafNode root;
    if ((rc = root.insert(key, rid)) != 0)
        return rc;

    // Update private vars
    rootPid = pf.endPid();
    treeHeight = 1;

    // Write node (contents)
    if ((rc = root.write(rootPid, pf)) != 0)
        return rc;

    return 0;
}

/*
 * Inserts a (key, RecordId) pair into a leaf node.
 * @warning This function should not be called directly.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @param pid[IN] pid of the node where the search should begin
 * @param newNodeKey[OUT] the first key of the newly inserted node in case of an
 * overflow, otherwise -1
 * @param newNodePid[OUT] newly inserted node's pid in case of an overflow
 * @return error code. 0 if no error
 */
RC BTreeIndex::insertAtLeafNode(int key, const RecordId& rid, PageId pid,
                                int& newNodeKey, PageId& newNodePid)
{
    RC rc;
    newNodeKey = -1;
    BTLeafNode node;

    // Read the content of the node from pid in pf
    node.read(pid, pf);

    // Insert data into the node
    rc = node.insert(key, rid);
    // Check for an overflow and other errors
    if (rc == RC_NODE_FULL) {
        // Insert data into a new node
        BTLeafNode newNode;
        if ((node.insertAndSplit(key, rid, newNode, newNodeKey)) != 0)
            return rc;

        newNodePid = pf.endPid();   // new node's future pid

        // Update node pointers
        newNode.setNextNodePtr(node.getNextNodePtr());
        node.setNextNodePtr(newNodePid);

        // Write node [contents]
        if ((rc = newNode.write(newNodePid, pf)) != 0)
            return rc;
    } else if (rc != 0) {
        return rc;
    }

    // Write node [contents]
    if ((rc = node.write(pid, pf)) != 0)
        return rc;

    return 0;
}

/*
 * Recursively insert a (key, RecordId) pair into the index.
 * @warning This function should not be called directly.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @param pid[IN] pid of the node where the search should begin
 * @param height[IN] the height of the pid node (e.g. root has height 1)
 * @param newNodeKey[OUT] the first key of the newly inserted node in case of an
 * overflow, otherwise -1
 * @param newNodePid[OUT] newly inserted node's pid in case of an overflow
 * @return error code. 0 if no error
 */
RC BTreeIndex::insertAtNonLeafNode(int key, const RecordId& rid, PageId pid, int height,
                                   int& newNodeKey, PageId& newNodePid)
{
    RC rc;
    int childIndex;
    PageId childPid;
    BTNonLeafNode node;

    // Read the content of the node from pid in pf and obtain child's pid
    node.read(pid, pf);
    node.locateChildPtr(key, childIndex);
    node.readEntry(childIndex, childPid);

    // Check if we reached the leaf node
    if (height + 1 == treeHeight) {
        if ((rc = insertAtLeafNode(key, rid, childPid, newNodeKey, newNodePid)) != 0)
            return rc;
    } else {
        if ((rc = insertAtNonLeafNode(key, rid, childPid, height + 1, newNodeKey, newNodePid)) != 0)
            return rc;
    }

    // Check for overflows down the tree
    if (newNodeKey != -1) {
        // Try to insert new node's information into the current node
        rc = node.insert(newNodeKey, newNodePid);
        if (rc == 0) {
            newNodeKey = -1;
        } else if (rc == RC_NODE_FULL) {
            // Insert data into a new node
            BTNonLeafNode newNode;
            if ((node.insertAndSplit(newNodeKey, newNodePid, newNode, newNodeKey)) != 0)
                return rc;

            newNodePid = pf.endPid();   // new node's future pid

            // Write node [contents]
            if ((rc = newNode.write(newNodePid, pf)) != 0)
                return rc;
        } else {
            return rc;
        }

        // Write node [contents]
        if ((rc = node.write(pid, pf)) != 0)
            return rc;
    }

    return 0;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
    // Add a root node if the tree is empty
    if (treeHeight == 0) {
        return insertAtRoot(key, rid);
    }

    RC rc;
    int newNodeKey;
    PageId newNodePid;
    if (treeHeight == 1) {
        if ((rc = insertAtLeafNode(key, rid, rootPid, newNodeKey, newNodePid)) != 0)
            return rc;
    } else {
        if ((rc = insertAtNonLeafNode(key, rid, rootPid, 1, newNodeKey, newNodePid)) != 0)
            return rc;
    }

    // Check for overflows down the tree
    if (newNodeKey != -1) {
        // Insert data into a new node
        BTNonLeafNode root;
        root.initializeRoot(rootPid, newNodeKey, newNodePid);

        // Update private variables
        rootPid = pf.endPid();   // new root's future pid
        treeHeight += 1;

        // Write node [contents]
        if ((rc = root.write(rootPid, pf)) != 0)
            return rc;
    }

    return 0;
}   

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    // Start searching for data from the root node
    PageId pid = rootPid;
    
    if (treeHeight <= 0)
        return RC_NO_SUCH_RECORD;

    // Traverse the tree until reaching a Non-Leaf node
    for (int i = 0, eid; i < treeHeight - 1; i++) {
        BTNonLeafNode node;

        // Read the content of the node from pid in pf
        node.read(pid, pf);

        // Obtain next node's pid
        node.locateChildPtr(searchKey, eid);
        node.readEntry(eid, pid);
    }

    BTLeafNode node;

    // Read node data
    node.read(pid, pf);
 
    // Set cursor's pid and eid
    cursor.pid = pid;
    node.locate(searchKey, cursor.eid);

    return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to a leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    BTLeafNode node;
    // Read the content of the node from pid in pf
    node.read(cursor.pid, pf);
    // Read the (key, rid) pair from eid entry
    node.readEntry(cursor.eid, key, rid);

    // Check cursor
    if (cursor.pid <= 0 || cursor.pid >= pf.endPid())
        return RC_INVALID_CURSOR;

    // Move the cursor forward
    cursor.eid++;
    if (cursor.eid >= node.getKeyCount()) // End of node
    {
      // Move to the next node
      cursor.pid = node.getNextNodePtr();
      // Reset cursor
      cursor.eid = 0;
    }

    return 0;
}

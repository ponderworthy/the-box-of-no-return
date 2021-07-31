/*
 * Copyright (c) 2015 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This file contains automated test cases against the RTAVLTree template class.

#include "RTAVLTree.h"
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifndef TEST_ASSERT
# define TEST_ASSERT assert
#endif

class IntNode : public RTAVLNode {
public:
    enum Dir_t {
        LEFT,
        RIGHT 
    };

    int value;
    using RTAVLNode::parent;
    using RTAVLNode::children;
    using RTAVLNode::reset;

    inline bool operator==(const IntNode& other) const {
        return this->value == other.value;
    }

    inline bool operator<(const IntNode& other) const {
        return this->value < other.value;
    }

    inline operator std::string() const {
        const int n = countTwins();
        std::stringstream ss;
        if (n > 1) ss << n << "x";
        ss << "[" << value << "(" << balance << ")]";
        return ss.str();
    }

    void appendLeft(IntNode& child) {
        #if !SILENT_TEST
        std::cout << "*** Insert " << child.value << " as left child of " << this->value << " ***\n";
        #endif
        IntNode& root = *this;
        IntNode& l    = child;
        root.children[LEFT] = &l;
        l.parent = &root;
        l.children[LEFT]  = NULL;
        l.children[RIGHT] = NULL;
    }

    void appendRight(IntNode& child) {
        #if !SILENT_TEST
        std::cout << "*** Insert " << child.value << " as right child of " << this->value << " ***\n";
        #endif
        IntNode& root = *this;
        IntNode& r    = child;
        root.children[RIGHT] = &r;
        r.parent = &root;
        r.children[LEFT]  = NULL;
        r.children[RIGHT] = NULL;
    }
};

typedef RTAVLTree<IntNode> MyTree;

/// This test case requires a visual check by humans, that means it will never detect errors on its own!
static void testTreeDumpWithManualTreeBuilt() {
    std::cout << "UNIT TEST: ManualTreeBuilt\n";
    std::cout << "*** Create empty tree ***\n";
    MyTree tree;
    tree.dumpTree();

    const int MAX_NODES = 10;
    IntNode nodes[MAX_NODES];
    for (int i = 0; i < MAX_NODES; ++i) {
        nodes[i].value = i;
        nodes[i].reset();
    }

    std::cout << "*** Insert " << nodes[0].value << " into tree ***\n";
    tree.insert(nodes[0]);
    tree.dumpTree();

    nodes[0].appendLeft(nodes[1]);
    tree.dumpTree();

    nodes[1].appendLeft(nodes[2]);
    tree.dumpTree();

    nodes[1].appendRight(nodes[3]);
    tree.dumpTree();
    
    nodes[3].appendRight(nodes[4]);
    tree.dumpTree();
    
    nodes[0].appendRight(nodes[5]);
    tree.dumpTree();
    
    nodes[5].appendLeft(nodes[6]);
    tree.dumpTree();
    
    nodes[6].appendLeft(nodes[7]);
    tree.dumpTree();
    
    nodes[5].appendRight(nodes[8]);
    tree.dumpTree();
    
    nodes[6].appendRight(nodes[9]);
    tree.dumpTree();
    
    std::cout << std::endl;
}

static void assertTreeLinks(MyTree& tree) {
    IntNode* from;
    IntNode* to;
    int res = tree.assertTreeLinks(from, to);
    if (res != 0 || from || to) {
        std::string sFrom = from ? ((std::string)*from) : "NULL";
        std::string sTo   = to ? ((std::string)*to) : "NULL";
        std::cout << "!!! Tree links inconsistency detected !!!\n"
                  << "!!! Invalid link from " << sFrom << " to " << sTo << " !!!\n";
        exit(-1);
    }
}

static void assertTreeSize(MyTree& tree) {
    if (tree.count() != tree.size()) {
        std::cout << "!!! Tree size inconsistency detected !!!\n"
                  << "!!! size() says " << tree.size() << " whereas count() says " << tree.count() << " !!!\n";
        exit(-1);
    }
    if (!tree.size() != tree.isEmpty()) {
        std::cout << "!!! Tree emptyness inconsistency detected !!!\n"
                  << "!!! isEmpty() says " << tree.isEmpty() << " whereas size() says " << tree.size() << " !!!\n";
        exit(-1);
    }
}

static void assertTreeBalance(MyTree& tree) {
    IntNode* unbalanced = tree.assertTreeBalance();
    if (unbalanced) {
        std::cout << "!!! Tree imbalance detected !!!\n"
                  << "!!! Node " << ((std::string)*unbalanced) << " is unbalanced !!!\n";
        exit(-1);
    }
}

static void printMaximas(MyTree& tree) {
    if (tree.isEmpty()) return;
    std::cout << "LOW=" << tree.lowest().value << " HIGH=" << tree.highest().value << std::endl;
}

static void insert(MyTree& tree, IntNode& node) {
    #if !SILENT_TEST
    std::cout << "+++ Insert " << node.value << " into tree +++\n";
    #endif
    tree.insert(node);
    #if !NO_TREE_DUMP && !SILENT_TEST
    tree.dumpTree();
    #endif
    assertTreeLinks(tree);
    assertTreeSize(tree);
    assertTreeBalance(tree);
    #if !SILENT_TEST
    printMaximas(tree);
    #endif
}

static void erase(MyTree& tree, IntNode& node) {
    #if !SILENT_TEST
    std::cout << "--- Erase " << node.value << " from tree ---\n";
    #endif
    tree.erase(node);
    #if !NO_TREE_DUMP && !SILENT_TEST
    tree.dumpTree();
    #endif
    assertTreeLinks(tree);
    assertTreeSize(tree);
    assertTreeBalance(tree);
    #if !SILENT_TEST
    printMaximas(tree);
    #endif
}

/// Automated test case which aborts this process with exit(-1) in case an error is detected.
static void testTreeInsertAndEraseWithSelectedNumbers() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: InsertAndEraseWithSelectedNumbers\n";
    std::cout << "*** Create empty tree ***\n";
    #endif
    MyTree tree;
    #if !NO_TREE_DUMP && !SILENT_TEST
    tree.dumpTree();
    #endif

    const int MAX_NODES = 20;
    IntNode nodes[MAX_NODES];
    for (int i = 0; i < MAX_NODES; ++i)
        nodes[i].value = i;

    // fill up the tree with numbers ...

    insert(tree, nodes[6]);
    insert(tree, nodes[7]);
    insert(tree, nodes[9]);
    insert(tree, nodes[3]);
    insert(tree, nodes[5]);
    insert(tree, nodes[1]);
    insert(tree, nodes[8]);
    insert(tree, nodes[15]);
    insert(tree, nodes[12]);
    insert(tree, nodes[11]);
    insert(tree, nodes[10]);
    insert(tree, nodes[13]);
    insert(tree, nodes[17]);
    insert(tree, nodes[19]);    
    insert(tree, nodes[16]);    
    insert(tree, nodes[14]);    
    insert(tree, nodes[18]);    
    insert(tree, nodes[4]);    
    insert(tree, nodes[2]);    
    insert(tree, nodes[0]);

    // now start to erase ...

    erase(tree, nodes[18]);
    erase(tree, nodes[5]);    
    erase(tree, nodes[6]);
    erase(tree, nodes[9]);    
    erase(tree, nodes[13]);    
    erase(tree, nodes[7]);
    erase(tree, nodes[3]);
    erase(tree, nodes[10]);
    erase(tree, nodes[19]);
    erase(tree, nodes[15]);
    erase(tree, nodes[11]);
    erase(tree, nodes[4]);
    erase(tree, nodes[0]);
    erase(tree, nodes[14]);
    erase(tree, nodes[1]);
    erase(tree, nodes[16]);
    erase(tree, nodes[8]);
    erase(tree, nodes[12]);
    erase(tree, nodes[17]);
    erase(tree, nodes[2]);

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

/// Automated test case which aborts this process with exit(-1) in case an error is detected.
static void testTreeInsertAndEraseWithRandomNumbers() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: InsertAndEraseWithRandomNumbers\n";
    #endif
    srand(time(NULL));

    #if !SILENT_TEST
    std::cout << "*** Create empty tree ***\n";
    #endif
    MyTree tree;
    #if !NO_TREE_DUMP && !SILENT_TEST
    tree.dumpTree();
    #endif

    const int MAX_NODES = 30;
    IntNode nodes[MAX_NODES];
    std::vector<IntNode*> freeNodes;
    std::vector<IntNode*> usedNodes;
    for (int i = 0; i < MAX_NODES; ++i) {
        nodes[i].value = i;
        freeNodes.push_back(&nodes[i]);
    }

    // insert all MAX_NODES nodes into the tree (in randomly selected sequence)
    do {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(freeNodes.size()));

        insert(tree, *freeNodes[idx]);

        usedNodes.push_back(freeNodes[idx]);
        freeNodes.erase(freeNodes.begin()+idx);
        
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    } while (!freeNodes.empty());

    // randomly erase and re-insert elements into the tree for a certain while
    for (int run = 0; run < 300; ++run) {
        bool doInsert =
            freeNodes.empty() ? false : usedNodes.empty() ? true : (rand() & 1);
        if (doInsert) {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(freeNodes.size()));

            insert(tree, *freeNodes[idx]);

            usedNodes.push_back(freeNodes[idx]);
            freeNodes.erase(freeNodes.begin()+idx);
        } else {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(usedNodes.size()));

            erase(tree, *usedNodes[idx]);

            freeNodes.push_back(usedNodes[idx]);
            usedNodes.erase(usedNodes.begin()+idx);
        }
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase from tree until tree is completely empty
    while (!usedNodes.empty()) {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(usedNodes.size()));

        erase(tree, *usedNodes[idx]);

        freeNodes.push_back(usedNodes[idx]);
        usedNodes.erase(usedNodes.begin()+idx);
        
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase and re-insert elements into the tree for a certain while
    for (int run = 0; run < 300; ++run) {
        bool doInsert =
            freeNodes.empty() ? false : usedNodes.empty() ? true : (rand() & 1);
        if (doInsert) {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(freeNodes.size()));

            insert(tree, *freeNodes[idx]);

            usedNodes.push_back(freeNodes[idx]);
            freeNodes.erase(freeNodes.begin()+idx);
        } else {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(usedNodes.size()));

            erase(tree, *usedNodes[idx]);

            freeNodes.push_back(usedNodes[idx]);
            usedNodes.erase(usedNodes.begin()+idx);
        }
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase from tree until tree is completely empty
    while (!usedNodes.empty()) {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(usedNodes.size()));

        erase(tree, *usedNodes[idx]);

        freeNodes.push_back(usedNodes[idx]);
        usedNodes.erase(usedNodes.begin()+idx);

        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

/// Automated test case which aborts this process with exit(-1) in case an error is detected.
static void testTwinsWithRandomNumbers() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: TwinsWithRandomNumbers\n";
    #endif
    srand(time(NULL));

    #if !SILENT_TEST
    std::cout << "*** Create empty tree ***\n";
    #endif
    MyTree tree;
    #if !NO_TREE_DUMP && !SILENT_TEST
    tree.dumpTree();
    #endif

    const int MAX_NODES = 30;
    IntNode nodes[MAX_NODES];
    std::vector<IntNode*> freeNodes;
    std::vector<IntNode*> usedNodes;
    for (int i = 0; i < MAX_NODES; ++i) {
        const double r = double(rand()) / double(RAND_MAX);
        const int value = int(r * double(MAX_NODES) * 0.66);

        nodes[i].value = value;
        freeNodes.push_back(&nodes[i]);
    }

    // insert all MAX_NODES nodes into the tree (in randomly selected sequence)
    do {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(freeNodes.size()));

        insert(tree, *freeNodes[idx]);

        usedNodes.push_back(freeNodes[idx]);
        freeNodes.erase(freeNodes.begin()+idx);
        
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    } while (!freeNodes.empty());
    
    // randomly erase and re-insert elements into the tree for a certain while
    for (int run = 0; run < 300; ++run) {
        bool doInsert =
            freeNodes.empty() ? false : usedNodes.empty() ? true : (rand() & 1);
        if (doInsert) {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(freeNodes.size()));

            insert(tree, *freeNodes[idx]);

            usedNodes.push_back(freeNodes[idx]);
            freeNodes.erase(freeNodes.begin()+idx);
        } else {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(usedNodes.size()));

            erase(tree, *usedNodes[idx]);

            freeNodes.push_back(usedNodes[idx]);
            usedNodes.erase(usedNodes.begin()+idx);
        }
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase from tree until tree is completely empty
    while (!usedNodes.empty()) {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(usedNodes.size()));

        erase(tree, *usedNodes[idx]);

        freeNodes.push_back(usedNodes[idx]);
        usedNodes.erase(usedNodes.begin()+idx);
        
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase and re-insert elements into the tree for a certain while
    for (int run = 0; run < 300; ++run) {
        bool doInsert =
            freeNodes.empty() ? false : usedNodes.empty() ? true : (rand() & 1);
        if (doInsert) {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(freeNodes.size()));

            insert(tree, *freeNodes[idx]);

            usedNodes.push_back(freeNodes[idx]);
            freeNodes.erase(freeNodes.begin()+idx);
        } else {
            const double r = double(rand()) / double(RAND_MAX);
            const int idx = int(r * double(usedNodes.size()));

            erase(tree, *usedNodes[idx]);

            freeNodes.push_back(usedNodes[idx]);
            usedNodes.erase(usedNodes.begin()+idx);
        }
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    // randomly erase from tree until tree is completely empty
    while (!usedNodes.empty()) {
        const double r = double(rand()) / double(RAND_MAX);
        const int idx = int(r * double(usedNodes.size()));

        erase(tree, *usedNodes[idx]);

        freeNodes.push_back(usedNodes[idx]);
        usedNodes.erase(usedNodes.begin()+idx);
        
        TEST_ASSERT(usedNodes.size() + freeNodes.size() == MAX_NODES);
        TEST_ASSERT(tree.size() == usedNodes.size());
    }

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

#if !NO_MAIN

int main() {
    testTreeDumpWithManualTreeBuilt();
    testTreeInsertAndEraseWithSelectedNumbers();
    testTreeInsertAndEraseWithRandomNumbers();
    testTwinsWithRandomNumbers();
    std::cout << "\nAll tests passed successfully. :-)\n";
    return 0;
}

#endif // !NO_MAIN

/*
 * Copyright (c) 2015-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef RTAVLTREE_H
#define RTAVLTREE_H

#include <vector>
#include <string>

#include "RTMath.h"

class RTAVLTreeBase;

/**
 * @brief Base class of RTAVLTree elements.
 *
 * For being able to manage elements with an RTAVLTree, this class must be
 * derived and the missing methods must be implemented. At least the following
 * two methods must be implemented in the deriving node class:
 * @code
 * bool operator==(const YourNodeType& other) const;
 * bool operator<(const YourNodeType& other) const;
 * @endcode
 * The latter two methods are mandatory and used by the RTAVLTree class to
 * compare elements with each other for being able to sort them. In case you are
 * also using one of the testing/debugging methods of the RTAVLTree class, then
 * the deriving node class also needs to implement the following method:
 * @code
 * operator std::string() const;
 * @endcode
 * The latter is responsible to convert the value of your node into a string
 * representation which is i.e. used by RTAVLTree::dumpTree() to print the
 * current structure of the AVL tree as human readable presentation to the
 * terminal.
 */
class RTAVLNode {
public:
    /**
     * Returns the current RTAVLTree this node is a member of.
     *
     * @b CAUTION: This method must only be used if RTAVLTree's template
     * parameter was set to T_SAFE = true. Using the result of this method call
     * with T_SAFE = false may result in undefined behavior!
     */
    RTAVLTreeBase* rtavlTree() const { return tree; }

protected:
    /**
     * Initialize the members of this node. It is not necessearily required to
     * be called by the deriving class (i.e. from the constructor of the
     * deriving node class), that's because this method is automatically called
     * by the RTAVLTree class whenever an element is inserted or removed from
     * the tree. You may however call this method explicitly to initialize nodes
     * in case you encounter undeterministic behaviors (i.e. crashes) with your
     * own code.
     */
    inline void reset() {
        parent = children[0] = children[1] = NULL;
        prevTwin = nextTwin = this;
        balance = 0;
        twinHead = true;
        tree = NULL;
    }

    /**
     * Counts and returns the total amount of twins of this node (including
     * this node). A twin is a node with the exact same key value. If this node
     * does not have any other twins (that is if this node is unique), then this
     * method returns 1.
     *
     * @b CAUTION: This method is inefficient and should only be used for unit
     * tests and debugging! This method should @b NOT be used by production code!
     */
    int countTwins() const {
        int n = 1;
        for (RTAVLNode* twin = nextTwin; twin != this; ++n, twin = twin->nextTwin);
        return n;
    }

protected:
    RTAVLNode* parent;
    RTAVLNode* children[2];
    RTAVLNode* prevTwin;
    RTAVLNode* nextTwin;
    int balance;
    bool twinHead;
    RTAVLTreeBase* tree; ///< This is only used by RTAVLTree if T_SAFE = true
    template<class T_node, bool T_SAFE> friend class RTAVLTree;
};

/**
 * Abstract base class for deriving tempalte class RTAVLTree. This is just
 * needed for the "tree" pointer member variable in RTAVLNode.
 */
class RTAVLTreeBase {
public:
};

/**
 * @brief Real-time safe implementation of an AVL tree (with multi-map design).
 *
 * An AVL tree (Georgy Maximovich Adelson-Velsky & Yevgeny Mikhaylowich Landis
 * tree) is a self-balancing binary search tree, thus it is a sorted container
 * for elements. This particular RTAVLTree class uses a multi-map design. That
 * means multiple elements with the exact same key may be inserted into the
 * tree. So it does not require the individual elements to be unique and
 * inserting such duplicate elements do not replace such existing elements with
 * equal key.
 *
 * This implementation of an AVL tree provides real-time safe operations, that
 * is tree operations do not allocate or deallocate memory, they are
 * non-recursive algorithms and thus are not growing the memory stack
 * (substantially). Additionally, and in contrast to many other AVL tree
 * implementations, this implementation of an AVL tree provides
 * constant time average complexity for erase operations.
 *
 * @c T_SAFE: this boolean template class parameter defines whether the tree's
 * algorithm should be more safe but a bit slower or rather fast and less safe.
 * If T_SAFE = true then additional checks and measures will be performed when
 * calling insert() or erase(). The latter methods will then check whether the
 * passed node is already part of the tree and act accordingly to avoid
 * undefined behavior which could happen if T_SAFE = false. So if you decide to
 * set T_SAFE = false then it is your responsibility to only insert() elements
 * to this tree which are not yet members of the tree and to only erase()
 * nodes which are really members of the tree. The only real performance
 * penalty that comes with T_SAFE = true is the efficiency of the clear() method
 * which will be much slower than with T_SAFE = false. See the description of the
 * clear() method for the precise performance differences regarding T_SAFE.
 *
 * @b IMPORTANT: some of the methods of this class are only intended for unit
 * tests and debugging purposes and are not real-time safe! Those methods are
 * explicitly marked as such and must not be used in a real-time context!
 */
template<class T_node, bool T_SAFE = true>
class RTAVLTree : public RTAVLTreeBase {
public:
    enum Dir_t {
        LEFT,
        RIGHT 
    };

    /**
     * Constructs an empty RTAVLTree object.
     */
    RTAVLTree() : root(NULL), nodesCount(0) {}

    /**
     * Returns true if there are no elements in this tree (that is if size()
     * is zero).
     *
     * This method is real-time safe.
     *
     * Complexity: Theta(1).
     */
    inline bool isEmpty() const {
        return !root;
    }

    /**
     * Inserts the new element @a item into the tree. Since this class uses a
     * multi-map design, it is allowed to insert multiple elements with equal
     * key. Inserting an element does never replace an existing element. Also,
     * inserting such duplicate elements into the tree does not grow the
     * tree structure complexity at all. So no matter how many duplicates are
     * inserted into the tree, those duplicates have no negative impact on the
     * efficiency of any tree operation. In other words: a tree with @c n
     * elements having @c n different (and thus entirely unique) keys is slower
     * than a tree with @c n elements having less than @c n keys (thus having
     * some non-unique keys).
     *
     * Trying to insert an item that is already part of the tree will be
     * detected and ignored. Note however, that if T_SAFE = false then this
     * detection is limited to unique elements! So if T_SAFE = false then better
     * take care to only insert a new element that is not yet member of the
     * tree.
     *
     * This method is real-time safe.
     *
     * Complexity: O(log n) on worst case.
     *
     * @param item - new element to be inserted into the tree
     */
    void insert(T_node& item) {
        if (T_SAFE && item.tree == this) return;

        if (!root) {
            item.reset();
            if (T_SAFE) item.tree = this;
            root = &item;
            ++nodesCount;
            return;
        }

        RTAVLNode* node = root;

        // walk tree from top down & attach the new item to the tree
        while (true) {
            const Relation_t relation = compare(node, &item);
            if (relation == EQUAL) {
                // there is already a node with that exact same key (if it is
                // the same key AND same item, then do nothing)
                if (node != static_cast<RTAVLNode*>(&item)) {
                    // it is the same key, but different item, so insert the
                    // passed item to this twin ring
                    item.reset();
                    if (T_SAFE) item.tree = this;
                    node->prevTwin->nextTwin = &item;
                    item.prevTwin = node->prevTwin;
                    item.nextTwin = node;
                    item.twinHead = false;
                    node->prevTwin = &item;
                    ++nodesCount;
                }
                return;
            } else {
                Dir_t dir = (relation == LESS) ? LEFT : RIGHT;
                if (node->children[dir]) {
                    node = node->children[dir];
                } else {
                    item.reset();
                    if (T_SAFE) item.tree = this;
                    node->children[dir] = &item;
                    item.parent = node;
                    node = &item;
                    ++nodesCount;
                    break;
                }
            }
        }

        int increase = 1;

        // walk tree from new item's position upwards & re-balance tree
        while (node->parent && increase) {
            increase *= relationToParent(node);
            node = node->parent;
            node->balance += increase;
            increase =
                (node->balance)
                    ? (1 - rebalance(node)) : 0;
        }
    }

    /**
     * Removes the existing element @a item from the tree.
     *
     * If T_SAFE = true then calling erase() with a node that is not part of
     * this tree will simply be ignored.
     *
     * If T_SAFE = false then it is assumed that the passed @a item is really
     * a member of this tree when this method is called. There are some checks
     * even if T_SAFE = false which abort the erase operation if the
     * passed element is detected not being part of the tree, however these
     * checks do not cover all possible cases and they also require that
     * RTAVLNode::reset() was called after the element was allocated. So better
     * don't rely on those checks if T_SAFE = flase and only call erase() in this
     * case for elements which are really a member of this tree at that point.
     *
     * This method is real-time safe.
     *
     * Complexity: O(log n) on worst case, Theta(1) on average.
     *
     * @param item - element of the tree to be removed from the tree
     */
    void erase(T_node& item) {
        if (T_SAFE && item.tree != this) return;

        if (!root) {
            item.reset();
            return;
        }

        // if the item is part of a twin ring and not head of the ring, then
        // just link it out from the twin ring
        if (!item.twinHead) {
            item.prevTwin->nextTwin = item.nextTwin;
            item.nextTwin->prevTwin = item.prevTwin;
            item.reset();
            --nodesCount;
            return;
        } else if (item.nextTwin != static_cast<RTAVLNode*>(&item)) {
            // item is head of a twin ring (and ring has at least 2 twins), so
            // remove this item from the ring and make next one in the twin ring
            // head of the twin ring
            item.nextTwin->parent      = item.parent;
            item.nextTwin->children[0] = item.children[0];
            item.nextTwin->children[1] = item.children[1];
            item.nextTwin->balance     = item.balance;
            item.nextTwin->twinHead = true;
            item.prevTwin->nextTwin = item.nextTwin;
            item.nextTwin->prevTwin = item.prevTwin;
            if (item.children[0])
                item.children[0]->parent = item.nextTwin;
            if (item.children[1])
                item.children[1]->parent = item.nextTwin;
            *downLinkTo(&item) = item.nextTwin;
            item.reset();
            --nodesCount;
            return;
        }

        RTAVLNode* node = &item;

        int decrease = 0;

        if (node->children[LEFT] && node->children[RIGHT]) { // node to be removed has exactly two children ...
            //TODO: this code branch is currently ALWAYS using the "in-order successor", one might however also pick the "in-order predecessor" in cases where the balancing situation would create a performance benefit (that decision and then using the "in-order predecessor" is not implemented here yet).

            // walk down to the minimum node of the right subtree ("in-order
            // successor"), hang it out and replace it with the actual node to
            // be erased, that is:
            //     node -> right -> left -> left -> ... -> left -> NULL
            for (node = node->children[RIGHT]; node->children[LEFT]; node = node->children[LEFT]);
            RTAVLNode* dangling = node;
            const bool danglingIs1stChild = item.children[RIGHT] == dangling;

            *downLinkTo(dangling) = dangling->children[RIGHT];
            if (dangling->children[RIGHT])
                dangling->children[RIGHT]->parent = dangling->parent;
            
            // define start node where rebalancing shall be started later on
            node = (!danglingIs1stChild) ? dangling->parent : dangling;

            // replace item to be erased by the dangling node
            if (item.children[LEFT])
                item.children[LEFT]->parent = dangling;
            if (!danglingIs1stChild)
                dangling->children[RIGHT] = item.children[RIGHT];
            if (item.children[RIGHT])
                item.children[RIGHT]->parent = dangling; // NOTE: this could assign dangling's parent to itself, thus we assign it next (no branch required) ...
            dangling->parent = item.parent;
            dangling->balance = item.balance;
            dangling->children[LEFT] = item.children[LEFT];
            *downLinkTo(&item) = dangling;

            decrease = 1;
            for (; true; node = node->parent) {
                const bool isDangling = node == dangling;
                decrease *= ((isDangling) ? GREATER : LESS);
                node->balance -= decrease;
                if (decrease)
                    decrease = (node->balance) ? rebalance(node) : 1; 
                if (isDangling) break;
            }
        } else if (!node->children[LEFT] && !node->children[RIGHT]) { // node to be removed is a leaf ...
            if (root == &item) {
                root = NULL;
                nodesCount = 0;
                item.reset();
                return;
            }
            const Dir_t dir = (node->parent->children[LEFT] == node) ? LEFT : RIGHT;
            node->parent->children[dir] = NULL;
            decrease = 1;
            // since we hooked out the node already, we must do one iteration
            // of the while loop below explicitly, becase relationToParent()
            // would not work
            decrease *= (dir == LEFT) ? LESS : GREATER;
            node = node->parent;
            node->balance -= decrease;
            decrease = (node->balance) ? rebalance(node) : 1;
        } else { // node to be removed is a branch with exactly one child ...
            RTAVLNode* child = node->children[(node->children[RIGHT]) ? RIGHT : LEFT];
            if (node == root) {
                root = child;
            } else {
                if (node->parent->children[LEFT] == node)
                    node->parent->children[LEFT] = child;
                else
                    node->parent->children[RIGHT] = child;
            }
            child->parent = node->parent;
            node = child;
            decrease = 1;
        }

        // rebalance tree from erased item's position upwards until a tree level
        // is reached where no rebalancing is required
        while (node->parent && decrease) {
            decrease *= relationToParent(node);
            node = node->parent;
            node->balance -= decrease;
            decrease = (node->balance) ? rebalance(node) : 1;
        }

        --nodesCount;
        item.reset();
    }

    /**
     * Returns the smallest element of this tree (element with the lowest key).
     * It assumes that this tree is not empty. If you call this method on an
     * empty tree, then a call to this method will crash.
     *
     * Since this class uses a multi-map design, there may be more than one
     * element with the same lowest key. In this case and with the current
     * implementation, the first element of those duplicates that was inserted
     * into the tree is returned. However you should not rely on this and expect
     * that any one of the duplicates may be returned here.
     *
     * This method is real-time safe.
     *
     * Complexity: Theta(log n).
     */
    T_node& lowest() const {
        if (!root) return *(T_node*)NULL; // crash it baby
        RTAVLNode* node = root;
        for (; node->children[LEFT]; node = node->children[LEFT]);
        return *static_cast<T_node*>(node);
    }

    /**
     * Returns the largest element of this tree (element with the highest key).
     * It assumes that this tree is not empty. If you call this method on an
     * empty tree, then a call to this method will crash.
     *
     * Since this class uses a multi-map design, there may be more than one
     * element with the same highest key. In this case and with the current
     * implementation, the first element of those duplicates that was inserted
     * into the tree is returned. However you should not rely on this and expect
     * that any one of the duplicates may be returned here.
     *
     * This method is real-time safe.
     *
     * Complexity: Theta(log n).
     */
    T_node& highest() const {
        if (!root) return *(T_node*)NULL; // crash it baby
        RTAVLNode* node = root;
        for (; node->children[RIGHT]; node = node->children[RIGHT]);
        return *static_cast<T_node*>(node);
    }
    
    /**
     * Returns successor of @a item in its tree, that is the element with the
     * next higher key value compared to @a item 's key value.
     *
     * Since this class uses a multi-map design, there may be more than one
     * element with the same key as @a item. In this case this method will
     * return the next duplicate element of @a item, or if @a item is already
     * the last duplicate of its key, then the element with the next higher key
     * value compared to @a item 's key value is returned.
     *
     * In other words: you may use this method to forward-iterate over all
     * elements of the entire tree (including duplicate elements).
     *
     * This method is real-time safe.
     *
     * Complexity: O(log n) on worst case.
     *
     * @param item - element of this tree whose successor is to be searched
     * @returns successor or NULL if @a item is the largest element of its tree
     *          (and has no further duplicates)
     */
    static T_node* after(const T_node& item) {
        RTAVLNode* node = (RTAVLNode*)(&item);

        if (!node->nextTwin->twinHead)
            return node->nextTwin;

        if (node->children[RIGHT]) {
            for (node = node->children[RIGHT]; node->children[LEFT]; node = node->children[LEFT]);
            return static_cast<T_node*>(node);
        } else {
            while (true) {
                if (!node->parent) return NULL;
                if (node->parent->children[LEFT] == node)
                    return static_cast<T_node*>(node->parent);
                node = node->parent;
            }
        }
    }

    /**
     * Returns predecessor of @a item in its tree, that is the element with the
     * next smaller key value compared to @a item 's key value.
     *
     * Since this class uses a multi-map design, there may be more than one
     * element with the same key as @a item. In this case this method will
     * return the previous duplicate element of @a item, or if @a item is
     * already the last duplicate of its key, then the element with the next
     * smaller key value compared to @a item 's key value is returned.
     *
     * In other words: you may use this method to backward-iterate over all
     * elements of the entire tree (including duplicate elements).
     *
     * This method is real-time safe.
     *
     * Complexity: O(log n) on worst case.
     *
     * @param item - element of this tree whose predecessor is to be searched
     * @returns predecessor or NULL if @a item is the smallest element of its
     *          tree (and has no further duplicates)
     */
    static T_node* before(const T_node& item) {
        RTAVLNode* node = (RTAVLNode*)(&item);

        if (!node->twinHead)
            return node->nextTwin;

        if (node->children[LEFT]) {
            for (node = node->children[LEFT]; node->children[RIGHT]; node = node->children[RIGHT]);
            return static_cast<T_node*>(node);
        } else {
            while (true) {
                if (!node->parent) return NULL;
                if (node->parent->children[RIGHT] == node)
                    return static_cast<T_node*>(node->parent);
                node = node->parent;
            }
        }
    }

    /**
     * Returns the amount of elements in this tree.
     *
     * This method is real-time safe.
     *
     * Complexity: Theta(1).
     */
    inline int size() const {
        return nodesCount;
    }

    /**
     * Removes all elements from this tree. That is size() will return @c 0
     * after calling this method.
     *
     * @b IMPORTANT: For the precise behavior and efficiency of this method, as
     * well as saftety of subsequent other method calls, it is important which
     * value you assigned for template class parameter @c T_SAFE :
     *
     * If @c T_SAFE @c = @c false then this method does not reset the
     * invidual element nodes (for performance reasons). Due to this, after
     * calling clear(), you @b must @b not pass any of those elements to the
     * erase() method of this class, nor to any static method of this class.
     * Doing so would lead to undefined behavior. Re-inserting the elements to
     * this or to any other tree with insert() is safe though. The advantage on
     * the other hand is that clear() is extremely fast if T_SAFE = false
     * (see algorithm complexity below).
     *
     * If @c T_SAFE @c = @c true then this method will reset() @b each node of
     * this tree before removing the nodes and thus clearing the tree. The
     * advantage is that with T_SAFE = true subsequent method calls on
     * this tree are way more safe, because guaranteed checks can be performed
     * whether the respective node is already member of the tree. This safety
     * comes with the price that clear() calls will be much slower (see
     * algorithm complexity below).
     *
     * This method is real-time safe.
     *
     * Complexity: Theta(1) if T_SAFE = false, or n log n if T_SAFE = true.
     */
    inline void clear() {
        if (T_SAFE) {
            while (root) erase(*(T_node*)root);
        }
        root = NULL;
        nodesCount = 0;
    }

    ///////////////////////////////////////////////////////////////////////
    // Methods intended for unit tests & debugging purposes ...

    /**
     * Returns the amount of elements in this tree. You do @b NOT want to call
     * this method! You want to call size() instead! Both methods count() and
     * size() return the same value, however count() actually traverses the tree
     * each time this method is called, to really "count" the actual amount of
     * elements currently contained in the tree, size() returns a buffered
     * scalar instead.
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     */
    int count() const {
        int n = 0;
        count(root, n);
        return n;
    }

    /**
     * Returns the height of this tree, that is the largest distance between the
     * root of this tree to any of its leafs (plus one). Or in other words:
     * the largest amount of elements of this tree from top down when seen from
     * vertical perspective.
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     */
    int height() const {
        return height(root);
    }

    /**
     * Returns the width of this tree, that is the largest amount of nodes of
     * this tree from left to right when seen from horizontal perspective.
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     */
    int width() const {
        return width(root);
    }

    /**
     * Prints a human-readable graphical representation of the current tree to
     * the terminal. In case you are using this method, then your RTAVLNode
     * deriving class must also implement the following method:
     * @code
     * operator std::string() const;
     * @endcode
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     */
    void dumpTree() const {
        if (!root) {
            printf("(Empty Tree)\n");
            return;
        }
        int unused;
        std::vector<std::string> buffer;
        dumpTree(root, unused, buffer);
        for (int l = 0; l < buffer.size(); ++l)
            printf("%s\n", buffer[l].c_str());
    }

    /**
     * Checks the integrity of the current tree by checking whether all
     * individual down- and uplinks between all elements of this tree are equal.
     * If all bi-directional links between all nodes of this tree are correct,
     * then this method returns zero. If an inconsistency is found regarding a
     * bidirectional link between two nodes, then this method returns -1 and the
     * two arguments @a from and @a to are assigned to the two elements of the
     * tree which were found to be incorrectly linked.
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     *
     * @param from - on error, this will be assigned to the source node having
     *               an invalid link
     * @param from - on error, this will be assigned to the destination node
     *               having an invalid link
     * @returns 0 if integrity of tree is correct, -1 on errors
     */
    int assertTreeLinks(T_node*& from, T_node*& to) const {
        from = to = NULL;
        return assertTreeLinks(root, from, to);
    }

    /**
     * Checks the integrity of the current tree by recalculating and checking
     * the AVL balance factor of each individual element of this tree. Each
     * element of an AVL tree must always have a balance factor between -1 and
     * +1. The insert() and erase() operations of this AVL tree implementation
     * are automatically rebalancing the tree in order that the individual
     * balance factors are always in that range. So if one of the elements is
     * found by this method to have a balance factor outside that valid value
     * range, then something is wrong with the currrent tree state and that
     * would mean there is a bug.
     *
     * @b CAUTION: This method is @b NOT real-time safe! This method is just
     * intended for unit tests & debugging purposes, do not call this method in
     * a real-time context!
     *
     * @returns NULL if integrity of tree is correct, or on errors pointer to
     *          node which was found to have an invalid balance factor
     */
    T_node* assertTreeBalance() const {
        return assertTreeBalance(root);
    }

protected:
    enum Relation_t {
       LESS    = -1,
       EQUAL   =  0,
       GREATER =  1
    };

    enum Balance_t {
        LEFT_HEAVY  = -1,
        BALANCED    =  0,
        RIGHT_HEAVY =  1
    };

    inline static int LEFT_IMBALANCE(int balance) {
        return (balance < LEFT_HEAVY);
    }

    inline static int RIGHT_IMBALANCE(int balance) {
        return (balance > RIGHT_HEAVY);
    }

    inline static Dir_t opposite(Dir_t dir) {
        return Dir_t(1 - int(dir));
    }

    inline static Relation_t compare(const RTAVLNode* a, const RTAVLNode* b) {
        const T_node& A = *(T_node*)a;
        const T_node& B = *(T_node*)b;
        if (B == A) return EQUAL;
        else if (B < A) return LESS;
        else return GREATER;
    }

private:
    int rebalance(RTAVLNode*& node) {
        int heightChange = 0;
        if (LEFT_IMBALANCE(node->balance)) {
            if (node->children[LEFT]->balance == RIGHT_HEAVY)
                heightChange = rotateTwice(node, RIGHT);
            else
                heightChange = rotateOnce(node, RIGHT);
        } else if (RIGHT_IMBALANCE(node->balance)) {
            if (node->children[RIGHT]->balance == LEFT_HEAVY)
                heightChange = rotateTwice(node, LEFT);
            else
                heightChange = rotateOnce(node, LEFT);
        }
        return heightChange;
    }

    int rotateOnce(RTAVLNode*& node, Dir_t dir) {
        const Dir_t otherDir = opposite(dir);
        RTAVLNode* old = node;

        const int heightChange = (node->children[otherDir]->balance == 0) ? 0 : 1;

        node = old->children[otherDir];
        *downLinkTo(old) = node;
        node->parent = old->parent;

        old->children[otherDir] = node->children[dir];
        if (old->children[otherDir])
            old->children[otherDir]->parent = old;
        old->parent = node;
        node->children[dir] = old;

        old->balance = -((dir == LEFT) ? --(node->balance) : ++(node->balance));

        return heightChange;
    }

    int rotateTwice(RTAVLNode*& node, Dir_t dir) {
        const Dir_t otherDir = opposite(dir);
        RTAVLNode* x = node;
        RTAVLNode* z = node->children[otherDir];
        RTAVLNode* y = z->children[dir];

        node = y;
        *downLinkTo(x) = y;
        y->parent = x->parent;

        x->children[otherDir] = y->children[dir];
        if (x->children[otherDir])
            x->children[otherDir]->parent = x;
        y->children[dir] = x;
        x->parent = y;

        z->children[dir] = y->children[otherDir];
        if (z->children[dir])
            z->children[dir]->parent = z;
        y->children[otherDir] = z;
        z->parent = y;

        // update balances
        node->children[LEFT]->balance  = -RTMath::Max(node->balance, 0);
        node->children[RIGHT]->balance = -RTMath::Min(node->balance, 0);
        node->balance = 0;

        // a double rotation always shortens the overall height of the tree
        return 1;
    }

    inline RTAVLNode** downLinkTo(const RTAVLNode* node) const {
        if (!node) return NULL;
        if (!node->parent) return (RTAVLNode**) &root;
        return (node->parent->children[LEFT] == node)
                    ? &node->parent->children[LEFT]
                    : &node->parent->children[RIGHT];
    }

    static inline Relation_t relationToParent(const RTAVLNode* node) {
        if (!node || !node->parent) return EQUAL;
        const Dir_t dir = uplinkDirFrom(node);
        return (dir == LEFT) ? LESS : GREATER;
    }

    static inline Dir_t uplinkDirFrom(const RTAVLNode* node) {
        return (node->parent->children[LEFT] == node) ? LEFT : RIGHT;
    }

    static int height(const RTAVLNode* node) {
        if (!node) return 0;
        return RTMath::Max(
            height(node->children[LEFT]),
            height(node->children[RIGHT])
        ) + 1;
    }

    int width(Dir_t dir) const {
        return width(root, dir);
    }

    static int width(const RTAVLNode* node) {
        if (!node) return 0;
        return width(node->children[LEFT]) +
               width(node->children[RIGHT]) + 1;
    }

    static int width(const RTAVLNode* node, Dir_t dir) {
        if (!node) return 0;
        return width(node->children[dir]);
    }

    static int width(const std::vector<std::string>& buffer) {
        int w = 0;
        for (int i = 0; i < buffer.size(); ++i)
            w = RTMath::Max(w, buffer[i].length());
        return w;
    }

    static int height(const std::vector<std::string>& buffer) {
        return buffer.size();
    }

    static void resize(std::vector<std::string>& buffer, int width, int height) {
        buffer.resize(height);
        for (int i = 0; i < height; ++i)
            buffer[i].resize(width, ' ');
    }

    static void count(const RTAVLNode* node, int& n) {
        if (!node) return;
        n += node->countTwins();
        count(node->children[LEFT], n);
        count(node->children[RIGHT], n);
    }

    static void dumpTree(const RTAVLNode* node, int& rootX, std::vector<std::string>& buffer) {
        if (!node) {
            rootX = 0;
            return;
        }

        T_node& nodeImpl = *(T_node*)node;
        std::string nodeValue = (std::string)nodeImpl;

        int childX[2] = {};
        std::vector<std::string> bufferChild[2];
        if (node->children[LEFT])
            dumpTree(node->children[LEFT], childX[LEFT], bufferChild[LEFT]);
        if (node->children[RIGHT])
            dumpTree(node->children[RIGHT], childX[RIGHT], bufferChild[RIGHT]);

        const int branchSpacing = nodeValue.length();
        const int totalW = width(bufferChild[LEFT]) + branchSpacing + width(bufferChild[RIGHT]);
        const int totalH = RTMath::Max(bufferChild[LEFT].size(), bufferChild[RIGHT].size()) + 1;
        resize(buffer, totalW, totalH);

        // merge bufferChild[2] -> buffer
        {
            for (int y = 0; y < bufferChild[LEFT].size(); ++y) {
                for (int x = 0; x < bufferChild[LEFT][y].length(); ++x) {
                    buffer[y+1][x] = bufferChild[LEFT][y][x];
                }
            }
        }
        {
            const int xOffset = width(bufferChild[LEFT]) + branchSpacing;
            for (int y = 0; y < bufferChild[RIGHT].size(); ++y) {
                for (int x = 0; x < bufferChild[RIGHT][y].length(); ++x) {
                    buffer[y+1][x+xOffset] = bufferChild[RIGHT][y][x];
                }
            }
        }
        // print child link lines
        {
            const int from = childX[LEFT];
            const int to   =
                (node->children[RIGHT]) ?
                    width(bufferChild[LEFT]) + branchSpacing + childX[RIGHT] :
                    width(bufferChild[LEFT]);
            for (int x = from; x <= to; ++x)
                buffer[0][x] = (x == from || x == to) ? '+' : '-';
        }
        // print node value
        {
            const int xOffset = width(bufferChild[LEFT]);
            for (int i = 0; i < nodeValue.length(); ++i)
                buffer[0][xOffset+i] = nodeValue[i];
        }

        rootX = width(bufferChild[LEFT]) + nodeValue.length() / 2;
    }

    int assertTreeLinks(const RTAVLNode* node, T_node*& from, T_node*& to) const {
        if (!node) return 0;
        if (*downLinkTo(node) != node) {
            from = (T_node*)(node->parent);
            to   = (T_node*)(node);
            return -1;
        }
        if (node->children[LEFT]) {
            int res = assertTreeLinks(node->children[LEFT], from, to);
            if (res) return res;
        }
        if (node->children[RIGHT]) {
            int res = assertTreeLinks(node->children[RIGHT], from, to);
            if (res) return res;
        }
        return 0;
    }

    static T_node* assertTreeBalance(const RTAVLNode* node) {
        if (!node) return NULL;
        int left  = height(node->children[LEFT]);
        int right = height(node->children[RIGHT]);
        int balance = right - left;
        if (balance < -1 || balance > 1)
            return (T_node*)node;
        T_node* res = assertTreeBalance(node->children[LEFT]);
        if (res) return res;
        return assertTreeBalance(node->children[RIGHT]);
    }

private:
    RTAVLNode* root;
    int nodesCount;
};

#endif // RTAVLTREE_H

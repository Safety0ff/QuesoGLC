#ifndef __glc_btree_h
#define __glc_btree_h

#define AVL_BALANCED 0
#define AVL_TO_LEFT  1
#define AVL_TO_RIGHT -1

typedef void(*destroyFunc)(void *);
typedef int(*compareFunc)(void *, void *);

class BTree {
 protected:
  void* data;
  BTree* left;
  BTree* right;
  destroyFunc destroy;

 public:
  BTree(void *data, destroyFunc destroy);
  ~BTree();
  int insertLeft(void *data);
  int insertRight(void *data);
  inline void removeLeft(void) { delete left; }
  inline void removeRight(void) { delete right; }

  inline BTree& getLeft(void) { return *left; }
  inline BTree& getRight(void) { return *right; }
  inline void* getData(void) { return data; }
};

class BSTree : public BTree {
  void *key;
  int factor;
  int hide;
  compareFunc compare;
  destroyFunc destroyKey;

  friend int insertNode(BSTree **, void *, void *, int&);
  friend void rotateLeft(BSTree **);
  friend void rotateRight(BSTree **);
  friend BSTree* walkThrough(BSTree *, int, int&);

 public:
  BSTree(void *key, void *data, destroyFunc destroyKey, destroyFunc destroyData, compareFunc compare);
  ~BSTree();
  BSTree* insert(void *key, void *data);
  void remove(void *key);
  void* lookup(void *key);
  void* element(int& elm);
};

#endif /* __glc_btree_h */

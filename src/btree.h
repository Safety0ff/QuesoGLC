#define AVL_BALANCED 0
#define AVL_TO_LEFT  1
#define AVL_TO_RIGHT -1

typedef void(*destroyFunc)(void *);
typedef int(*compareFunc)(void *, void *);

class BTree {
  BTree(BTree &tree) {}
  BTree& operator=(const BTree &tree) {}

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
  static BSTree* garbage;
  static int occurence;
  compareFunc compare;
  destroyFunc destroyKey;

  friend int insertNode(BSTree **, void *, void *, int&);
  friend void rotateLeft(BSTree **);
  friend void rotateRight(BSTree **);
  friend BSTree* mergeTree(BSTree *, BSTree *);

  BSTree* allocNode(void *, void *);
  void freeNode(BSTree *);

 public:
  BSTree(void *key, void *data, destroyFunc destroyKey, destroyFunc destroyData, compareFunc compare);
  ~BSTree();
  BSTree* insert(void *key, void *data);
  BSTree* remove(void *key);
  BSTree* merge(BSTree *tree);
  void* lookup(void *key);
};

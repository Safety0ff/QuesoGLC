#include <stdlib.h>
#include "btree.h"

BTree::BTree(void *inData, destroyFunc inDestroyFunc)
{
  data = inData;
  left = NULL;
  right = NULL;
  destroy = inDestroyFunc;
}

BTree::~BTree()
{
  delete left;
  delete right;
  if (destroy)
    destroy(data);
}

int BTree::insertLeft(void *inData)
{
  if (left)
    return -1;

  left = new BTree(inData, destroy);
  if (!left)
    return -1;

  return 0;
}

int BTree::insertRight(void *inData)
{
  if (right)
    return -1;

  right = new BTree(inData, destroy);
  if (!right)
    return -1;

  return 0;
}

BSTree::BSTree(void *inKey, void *inData, destroyFunc inDestroyKey, destroyFunc inDestroyData, compareFunc inCompareFunc)
  :BTree(inData, inDestroyData)
{
  key = inKey;
  factor = AVL_BALANCED;
  hide = 0;
  compare = inCompareFunc;
  destroyKey = inDestroyKey;
}

BSTree::~BSTree()
{
  if (destroyKey)
    destroyKey(key);
}

void rotateLeft(BSTree **root)
{
  BSTree *left = NULL;
  BSTree *grandSon = NULL;

  left = (BSTree *)(*root)->left;

  if (left->factor == AVL_TO_LEFT) {

    (*root)->left = left->right;
    left->right = *root;
    (*root)->factor = AVL_BALANCED;
    left->factor = AVL_BALANCED;
    *root = left;

  }
  else {

    grandSon = (BSTree *)left->right;
    left->right = grandSon->left;
    grandSon->left = left;
    (*root)->left = grandSon->right;
    grandSon->right = *root;

    switch(grandSon->factor) {

    case AVL_TO_LEFT:
      (*root)->factor = AVL_TO_RIGHT;
      left->factor = AVL_BALANCED;
      break;

    case AVL_BALANCED:
      (*root)->factor = AVL_BALANCED;
      left->factor = AVL_BALANCED;
      break;

    case AVL_TO_RIGHT:
      (*root)->factor = AVL_BALANCED;
      left->factor = AVL_TO_LEFT;
      break;

    }

    grandSon->factor = AVL_BALANCED;
    *root = grandSon;
  }

  return;
}

void rotateRight(BSTree **root)
{
  BSTree *right = NULL;
  BSTree *grandSon = NULL;

  right = (BSTree *)(*root)->right;

  if (right->factor == AVL_TO_RIGHT) {

    (*root)->right = right->left;
    right->left = *root;
    (*root)->factor = AVL_BALANCED;
    right->factor = AVL_BALANCED;
    *root = right;

  }
  else {

    grandSon = (BSTree *)right->left;
    right->left = grandSon->right;
    grandSon->right = right;
    (*root)->right = grandSon->left;
    grandSon->left = *root;

    switch(grandSon->factor) {

    case AVL_TO_LEFT:
      (*root)->factor = AVL_BALANCED;
      right->factor = AVL_TO_RIGHT;
      break;

    case AVL_BALANCED:
      (*root)->factor = AVL_BALANCED;
      right->factor = AVL_BALANCED;
      break;

    case AVL_TO_RIGHT:
      (*root)->factor = AVL_TO_LEFT;
      right->factor = AVL_BALANCED;
      break;

    }

    grandSon->factor = AVL_BALANCED;
    *root = grandSon;
  }

  return;
}

int insertNode(BSTree **root, void *inKey, void *inData, int& balance)
{
  int retVal = 0;
  int valCmp = 0;
  BSTree *tree = *root;

  valCmp = tree->compare(inKey, tree->key);

  if (valCmp < 0) {

    if (!tree->left) {

      tree->left = new BSTree(inKey, inData, tree->destroyKey, tree->destroy, tree->compare);
      if (!tree->left)
	return -1;

      ((BSTree *)(tree->left))->factor = AVL_BALANCED;
      balance = 0;

    }
    else {

      retVal = insertNode((BSTree **)&tree->left, inKey, inData, balance);
      if (retVal)
	return retVal;

    }

    if (!balance) {

      switch(tree->factor) {

      case AVL_TO_LEFT:
	rotateLeft(root);
	balance = 1;
	break;

      case AVL_BALANCED:
	tree->factor = AVL_TO_LEFT;
	break;

      case AVL_TO_RIGHT:
	tree->factor = AVL_BALANCED;
	balance = 1;

      }
    }
  }
  else if (valCmp > 0) {

    if (!tree->right) {

      tree->right = new BSTree(inKey, inData, tree->destroyKey, tree->destroy, tree->compare);
      if (!tree->right)
	return -1;

      ((BSTree *)(tree->right))->factor = AVL_BALANCED;
      balance = 0;

    }
    else {

      retVal = insertNode((BSTree **)&tree->right, inKey, inData, balance);
      if (retVal)
	return retVal;

    }

    if (!balance) {

      switch(tree->factor) {

      case AVL_TO_LEFT:
	tree->factor = AVL_BALANCED;
	balance = 1;
	break;

      case AVL_BALANCED:
	tree->factor = AVL_TO_RIGHT;
	break;

      case AVL_TO_RIGHT:
	rotateRight(root);
	balance = 1;

      }
    }
  }
  else {
    if (!tree->hide)
      return 1;
    else {
      if (tree->destroy)
	tree->destroy(tree->data);

      tree->data = inData;
      tree->key = inKey;
      tree->hide = 0;

      balance = 1;
    }
  }

  return 0;
}

BSTree* BSTree::insert(void *inKey, void *inData)
{
  BSTree *root = this;
  int balance = 0;

  if (insertNode(&root, inKey, inData, balance))
    return this;
  else
    return root;
}

void* BSTree::lookup(void *inKey)
{
  int valCmp = 0;
  void *retVal = NULL;

  valCmp = compare(inKey, key);

  if (valCmp < 0) {
    if (left)
      retVal = ((BSTree *)left)->lookup(inKey);
    else
      retVal = NULL;
  }
  else if (valCmp > 0) {
    if (right)
      retVal = ((BSTree *)right)->lookup(inKey);
    else
      retVal = NULL;
  }
  else
    retVal = data;

  return retVal;
}

void BSTree::remove(void *inKey)
{
  int valCmp = 0;
  BSTree *node = this;

  valCmp = compare(inKey, node->key);

  // Look for the node to remove and its parent
  while (valCmp && node) {

    if (valCmp < 0)
      node = (BSTree *)node->left;
    else if (valCmp > 0)
      node = (BSTree *)node->right;

    valCmp = compare(inKey, node->key);

  }

  // The node we are looking for does not exist
  if (!node)
    return;

  node->hide = 1;

  return;
}

BSTree* walkThrough(BSTree *node, int inElement, int& inCount)
{
  BSTree* found = NULL;

  if (node->right)
    found = walkThrough((BSTree *)node->right, inElement, inCount);

  if (!found) {
    if (inCount++ == inElement)
      found = node;
    else if (node->left)
      found = walkThrough((BSTree *)node->left, inElement, inCount);
  }

  return found;
}

void* BSTree::element(int& inElement)
{
  int count = 0;
  BSTree *node = NULL;

  node =  walkThrough(this, inElement, count);
  inElement -= count + 1;
  if (node)
    return node->data;
  else
    return NULL;
}

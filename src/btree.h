/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */
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

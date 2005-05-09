/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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

/* This file defines the functions needed for cleanup stack exception handling
 * (CSEH) which concept is described in details at
 * http://www.freetype.org/david/reliable-c.html
 */

/* Unhandled exceptions (when area->exceptContextStack.tail == NULL)
 * still need to be implemented. Such situations can occur if an exception
 * is thrown out of a try/catch block.
 */

#include "internal.h"
#include "except.h"
#include "ocontext.h"

#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LIST_H

typedef struct {
  void (*destructor) (void *);
  void *data;
} __glcCleanupStackNode;

typedef struct {
  __glcException exception;
  FT_List cleanupStack;
  jmp_buf env;
} __glcExceptContext;

jmp_buf* __glcExceptionCreateContext(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode node = NULL;
 
  area = __glcGetThreadArea();
  assert(area);
  xContext = (__glcExceptContext*)__glcMalloc(sizeof(__glcExceptContext));
  node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
  node->data = xContext;
  xContext->exception = GLC_NO_EXC;
  xContext->cleanupStack = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  xContext->cleanupStack->head = NULL;
  xContext->cleanupStack->tail = NULL;
  FT_List_Add(&area->exceptContextStack, node);

  return &xContext->env;
}

void __glcExceptionReleaseContext(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode node = NULL;

  area = __glcGetThreadArea();
  assert(area);

  node = area->exceptContextStack.tail;
  assert(node);

  xContext = (__glcExceptContext*)node->data;
  assert(xContext);
  assert(xContext->cleanupStack);
  assert(!xContext->cleanupStack->tail);
  assert(!xContext->cleanupStack->head);

  __glcFree(xContext->cleanupStack);
  FT_List_Remove(&area->exceptContextStack, node);
  __glcFree(node);
  __glcFree(xContext);
}

void __glcExceptionPush(void (*destructor)(void*), void *data)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode node = NULL;
  __glcCleanupStackNode *stackNode = NULL;
  char* ptr = NULL;

  area = __glcGetThreadArea();
  assert(area);
  assert(area->exceptContextStack.tail);

  xContext = (__glcExceptContext*)area->exceptContextStack.tail->data;
  assert(xContext);
  assert(xContext->cleanupStack);

  ptr = malloc(sizeof(FT_ListNodeRec) + sizeof(__glcCleanupStackNode));
  if (!ptr) {
    destructor(data);
    THROW(GLC_MEMORY_EXC);
  }

  node = (FT_ListNode)ptr;
  stackNode = (__glcCleanupStackNode*)(ptr + sizeof(FT_ListNodeRec));

  stackNode->destructor = destructor;
  stackNode->data = data;
  node->data = stackNode;
  FT_List_Add(xContext->cleanupStack, node);
}

void __glcExceptionPop(int destroy)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode node = NULL;
  __glcCleanupStackNode *stackNode = NULL;

  area = __glcGetThreadArea();
  assert(area);
  assert(area->exceptContextStack.tail);

  xContext = (__glcExceptContext*)area->exceptContextStack.tail->data;
  assert(xContext);
  assert(xContext->cleanupStack);

  node = xContext->cleanupStack->tail;
  assert(node);

  stackNode = (__glcCleanupStackNode*)node->data;
  if (destroy)
    stackNode->destructor(stackNode->data);
  FT_List_Remove(xContext->cleanupStack, node);
  free(node);
}

void __glcExceptionUnwind(int destroy)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;
  __glcCleanupStackNode *stackNode = NULL;

  area = __glcGetThreadArea();
  assert(area);
  assert(area->exceptContextStack.tail);

  xContext = (__glcExceptContext*)area->exceptContextStack.tail->data;
  assert(xContext);
  assert(xContext->cleanupStack);

  node = xContext->cleanupStack->head;

  while (node) {
    next = node->next;
    stackNode = (__glcCleanupStackNode*)node->data;
    if (destroy)
      stackNode->destructor(stackNode->data);
    free(node);
    node = next;
  }

  xContext->cleanupStack->head = NULL;
  xContext->cleanupStack->tail = NULL;
}

jmp_buf* __glcExceptionThrow(__glcException exception)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;

  area = __glcGetThreadArea();
  assert(area);
  assert(area->exceptContextStack.tail);

  xContext = (__glcExceptContext*)area->exceptContextStack.tail->data;
  assert(xContext);

  xContext->exception = exception;
  return &xContext->env;
}

__glcException __glcExceptionCatch(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;

  area = __glcGetThreadArea();
  assert(area);
  assert(area->exceptContextStack.tail);

  xContext = (__glcExceptContext*)area->exceptContextStack.tail->data;
  assert(xContext);

  return xContext->exception;
}

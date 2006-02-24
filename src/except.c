/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2006, Bertrand Coconnier
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

/* Unhandled exceptions (when area->exceptionStack.tail == NULL)
 * still need to be implemented. Such situations can occur if an exception
 * is thrown out of a try/catch block.
 */

#include "internal.h"
#include "except.h"

#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LIST_H

typedef struct {
  FT_ListNodeRec node;

  void (*destructor) (void *);
  void *data;
} __glcCleanupStackNode;

typedef struct {
  FT_ListNodeRec node;

  __glcException exception;
  FT_ListRec cleanupStack;
  jmp_buf env;
} __glcExceptContext;

/* Create a context for exception handling */
jmp_buf* __glcExceptionCreateContext(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
 
  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)malloc(sizeof(__glcExceptContext));
  if (!xContext) {
    area->failedTry = GLC_MEMORY_EXC;
    return NULL;
  }
  xContext->exception = GLC_NO_EXC;
  xContext->cleanupStack.head = NULL;
  xContext->cleanupStack.tail = NULL;
  FT_List_Add(&area->exceptionStack, (FT_ListNode)xContext);

  return &xContext->env;
}

/* Destroy the last context for exception handling */
void __glcExceptionReleaseContext(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;

  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);
  /* The cleanup stack must be empty */
  assert(!xContext->cleanupStack.head);
  assert(!xContext->cleanupStack.tail);

  FT_List_Remove(&area->exceptionStack, (FT_ListNode)xContext);
  free(xContext);
}

/* Keep track of data to be destroyed in case of */
void __glcExceptionPush(void (*destructor)(void*), void *data)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  __glcCleanupStackNode *stackNode = NULL;

  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);

  stackNode = (__glcCleanupStackNode*) malloc(sizeof(__glcCleanupStackNode));
  if (!stackNode) {
    destructor(data);
    THROW(GLC_MEMORY_EXC);
  }

  stackNode->destructor = destructor;
  stackNode->data = data;
  FT_List_Add(&xContext->cleanupStack, (FT_ListNode)stackNode);
}

/* Remove the last entry of the cleanup stack, eventually destroying the
 * corresponding data if destroy != 0
 */
void __glcExceptionPop(int destroy)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  __glcCleanupStackNode *stackNode = NULL;

  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);

  stackNode = (__glcCleanupStackNode*)xContext->cleanupStack.tail;
  assert(stackNode);

  if (destroy) {
    assert(stackNode->destructor);
    assert(stackNode->data);
    stackNode->destructor(stackNode->data);
  }
  FT_List_Remove(&xContext->cleanupStack, (FT_ListNode)stackNode);
  free(stackNode);
}

/* Empty the cleanup stack and destroy the corresponding data if
 * destroy != 0
 */
void __glcExceptionUnwind(int destroy)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;
  FT_ListNode next = NULL;
  __glcCleanupStackNode *stackNode = NULL;

  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);

  stackNode = (__glcCleanupStackNode*)xContext->cleanupStack.head;

  while (stackNode) {
    next = stackNode->node.next;
    if (destroy) {
      assert(stackNode->destructor);
      assert(stackNode->data);
      stackNode->destructor(stackNode->data);
    }
    free(stackNode);
    stackNode = (__glcCleanupStackNode*)next;
  }

  xContext->cleanupStack.head = NULL;
  xContext->cleanupStack.tail = NULL;
}

/* Throw an exception */
jmp_buf* __glcExceptionThrow(__glcException exception)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;

  area = __glcGetThreadArea();
  assert(area);

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);

  xContext->exception = exception;
  return &xContext->env;
}

/* Rethrow an exception that has already been catched */
__glcException __glcExceptionCatch(void)
{
  threadArea *area = NULL;
  __glcExceptContext *xContext = NULL;

  area = __glcGetThreadArea();
  assert(area);

  if (area->failedTry) {
    __glcException exc = area->failedTry;

    area->failedTry = GLC_NO_EXC;
    return exc;
  }

  xContext = (__glcExceptContext*)area->exceptionStack.tail;
  assert(xContext);

  return xContext->exception;
}

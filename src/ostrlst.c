/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

/* Defines the methods of an object that is used to manage string lists
 */

#include "internal.h"
#include "ostrlst.h"



/* Create a string list (i.e. a double linked list of Unicode strings) */
FT_List __glcStrLstCreate(__glcUniChar* inString)
{
  GLCchar *room = NULL;
  FT_List This = NULL;
  __glcUniChar *ustring = NULL;
  FT_ListNode node = NULL;

  This = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  This->head = NULL;
  This->tail = NULL;

  if (inString) {
    /* Make a local copy of the incoming string */
    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    __glcUniDup(inString, room, __glcUniLenBytes(inString));

    ustring = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    ustring->ptr = room;
    ustring->type = inString->type;
    node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
    node->data = ustring;
    FT_List_Add(This, node);
  }

  return This;
}



/* The string list destructor called by FT_List_Finalize() */
void __glcStrLstDestructor(FT_Memory memory, void *data, void *user)
{
  __glcUniChar *ustring = (__glcUniChar *)data;

  if (ustring)
    __glcUniDestroy(ustring);
}

/* Destroy a string list */
void __glcStrLstDestroy(FT_List This)
{
  FT_List_Finalize(This, __glcStrLstDestructor, __glcCommonArea->memoryManager,
		   NULL);
  __glcFree(This);
}



/* Convert a string list to the type 'inType'
 * We assume that every string in a string list has the same type. Hence the
 * need to convert the list to another if we plan to add a string which type
 * is different from the list type.
 * NOTE : we have made the assumption that the list is only converted to an
 * 'upper' type, that is a type which has a bigger memory footprint.
 */
static void __glcStrLstConvert(FT_List This, int inType)
{
  GLCchar *room = NULL;
  int size = 0;
  FT_ListNode node = NULL;
  __glcUniChar *ustring = NULL;

  if (!This->head)
    return;

  node = This->head;
  ustring = (__glcUniChar *)node->data;

  /* No conversion needed */
  if (inType == ustring->type)
    return;

  /* Convert each node of the list to the new type */
  while (node) {
    ustring = (__glcUniChar *)node->data;
    size = __glcUniEstimate(ustring, inType);
    room = (GLCchar *)__glcMalloc(size);
    __glcUniConvert(ustring, room, inType, size);
    __glcFree(ustring->ptr);
    ustring->ptr = room;
    ustring->type = inType;
    node = node->next;
  }
}



/* Create a node to be appended or prepended to the string list 'This'. */
static FT_ListNode __glcStrLstCreateNode(FT_List This, __glcUniChar* inString)
{
  GLCchar *room = NULL;
  FT_ListNode node = NULL;
  __glcUniChar *s = NULL;

  if (!inString)
    return NULL;

  s = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));

  if (This->head) {
    __glcUniChar *ustring = (__glcUniChar *)This->head->data;

    /* If the string type differs from the list type then the list is
     * converted to the string type. However conversion is only done if the
     * the string type is bigger (in terms of memory footprint) than the
     * list type.
     */
    if (inString->type > ustring->type)
      __glcStrLstConvert(This, inString->type);

    room = (GLCchar *)__glcMalloc(__glcUniEstimate(inString, ustring->type));

    /* If the conversion of the list to the string type has not been made
     * then it means that the string type is smaller (in terms of memory
     * footprint) than the list type. In that case this is the string that
     * is converted to the list type.
     */
    __glcUniConvert(inString, room, ustring->type,
		    __glcUniEstimate(inString, ustring->type));
    s->type = ustring->type;
  }
  else {
    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    __glcUniDup(inString, room, __glcUniLenBytes(inString));
    s->type = inString->type;
  }

  s->ptr = room;

  node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
  node->data = s;
  return node;
}



/* Append the Unicode string 'inString' to the string list */
GLint __glcStrLstAppend(FT_List This, __glcUniChar* inString)
{
  FT_ListNode node = __glcStrLstCreateNode(This, inString);

  if (node) {
    FT_List_Add(This, node);
    return 0;
  }
  else
    return -1;
}



/* Prepend the Unicode string 'inString' to the string list */
GLint __glcStrLstPrepend(FT_List This, __glcUniChar* inString)
{
  FT_ListNode node = __glcStrLstCreateNode(This, inString);

  if (node) {
    FT_List_Insert(This, node);
    return 0;
  }
  else
    return -1;
}



/* Find a node which Unicode string equals inString.
 * The function also returns the index of the node (which is needed because
 * GLC often refers to items by their index).
 */
static FT_ListNode __glcStrLstFindNode(FT_List This, __glcUniChar* inString,
				       GLint *inIndex)
{
  FT_ListNode node = NULL;

  *inIndex = 0;

  if (!This->head) {
    *inIndex = -1;
    return NULL;
  }

  node = This->head;

  while (node) {
    if (!__glcUniCompare((__glcUniChar *)node->data, inString)) break;
    node = node->next;
    (*inIndex)++;
  }

  if (!node)
    *inIndex = -1;

  return node;
}



/* Remove the node which contains inString from the string list */
GLint __glcStrLstRemove(FT_List This, __glcUniChar* inString)
{
  __glcUniChar *ustring = NULL;
  GLint index = 0;
  FT_ListNode node = __glcStrLstFindNode(This, inString, &index);

  if (!node)
    return -1;

  ustring = (__glcUniChar *)node->data;

  FT_List_Remove(This, node);
  __glcUniDestroy(ustring);
  __glcFree(node);
  return 0;
}



/* Find a node in the string list which index equals inIndex */
static FT_ListNode __glcStrLstFindNodeIndex(FT_List This, GLuint inIndex)
{
  FT_ListNode node = NULL;
  GLuint i = 0;

  if (!This->head)
    return NULL;

  node = This->head;

  for (i = 1; (i <= inIndex) && node; i++)
    node = node->next;

  return node;
}



/* Remove a node from the string list which index equals inIndex */
GLint __glcStrLstRemoveIndex(FT_List This, GLuint inIndex)
{
  __glcUniChar *ustring = NULL;
  FT_ListNode node = __glcStrLstFindNodeIndex(This, inIndex);

  if (!node)
    return -1;

  ustring = (__glcUniChar *)node->data;

  FT_List_Remove(This, node);
  __glcUniDestroy(ustring);
  __glcFree(node);
  return 0;
}



/* Returns the string which index in the string list equals inIndex */
__glcUniChar* __glcStrLstFindIndex(FT_List This, GLuint inIndex)
{
  FT_ListNode node = __glcStrLstFindNodeIndex(This, inIndex);

  if (!node)
    return NULL;

  return (__glcUniChar *)node->data;
}



/* Get the index of a Unicode string in a string list
 * Returns -1 in case of failure.
 */
GLint __glcStrLstGetIndex(FT_List This, __glcUniChar* inString)
{
  GLint index = 0;

  __glcStrLstFindNode(This, inString, &index);
  return index;
}



/* Calculates the number of elements of a list */
GLuint __glcStrLstLen(FT_List This)
{
  FT_ListNode node = This->head;
  GLuint length = 0;

  if (!node)
    return 0;

  while (node) {
    length++;
    node = node->next;
  }

  return length;
}

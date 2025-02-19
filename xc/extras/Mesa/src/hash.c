
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "glthread.h"
#include "hash.h"
#include "mem.h"
#endif


/*
 * Generic hash table.
 *
 * This is used to implement display list and texture object lookup.
 * NOTE: key=0 is illegal.
 */


#define TABLE_SIZE 1024

struct HashEntry {
   GLuint Key;
   void *Data;
   struct HashEntry *Next;
};

struct _mesa_HashTable {
   struct HashEntry *Table[TABLE_SIZE];
   GLuint MaxKey;
   _glthread_Mutex Mutex;
};



/*
 * Return pointer to a new, empty hash table.
 */
struct _mesa_HashTable *_mesa_NewHashTable(void)
{
   return CALLOC_STRUCT(_mesa_HashTable);
}



/*
 * Delete a hash table.
 */
void _mesa_DeleteHashTable(struct _mesa_HashTable *table)
{
   GLuint i;
   assert(table);
   for (i=0;i<TABLE_SIZE;i++) {
      struct HashEntry *entry = table->Table[i];
      while (entry) {
	 struct HashEntry *next = entry->Next;
	 FREE(entry);
	 entry = next;
      }
   }
   FREE(table);
}



/*
 * Lookup an entry in the hash table.
 * Input:  table - the hash table
 *         key - the key
 * Return:  user data pointer or NULL if key not in table
 */
void *_mesa_HashLookup(const struct _mesa_HashTable *table, GLuint key)
{
   GLuint pos;
   const struct HashEntry *entry;

   assert(table);

   pos = key & (TABLE_SIZE-1);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
	 return entry->Data;
      }
      entry = entry->Next;
   }
   return NULL;
}



/*
 * Insert into the hash table.  If an entry with this key already exists
 * we'll replace the existing entry.
 * Input:  table - the hash table
 *         key - the key (not zero)
 *         data - pointer to user data
 */
void _mesa_HashInsert(struct _mesa_HashTable *table, GLuint key, void *data)
{
   /* search for existing entry with this key */
   GLuint pos;
   struct HashEntry *entry;

   assert(table);

   _glthread_LOCK_MUTEX(table->Mutex);

   if (key > table->MaxKey)
      table->MaxKey = key;

   pos = key & (TABLE_SIZE-1);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         /* replace entry's data */
	 entry->Data = data;
         _glthread_UNLOCK_MUTEX(table->Mutex);
	 return;
      }
      entry = entry->Next;
   }

   /* alloc and insert new table entry */
   entry = MALLOC_STRUCT(HashEntry);
   entry->Key = key;
   entry->Data = data;
   entry->Next = table->Table[pos];
   table->Table[pos] = entry;

   _glthread_UNLOCK_MUTEX(table->Mutex);
}



/*
 * Remove an entry from the hash table.
 * Input:  table - the hash table
 *         key - key of entry to remove
 */
void _mesa_HashRemove(struct _mesa_HashTable *table, GLuint key)
{
   GLuint pos;
   struct HashEntry *entry, *prev;

   assert(table);

   _glthread_LOCK_MUTEX(table->Mutex);

   pos = key & (TABLE_SIZE-1);
   prev = NULL;
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         /* found it! */
         if (prev) {
            prev->Next = entry->Next;
         }
         else {
            table->Table[pos] = entry->Next;
         }
         FREE(entry);
         _glthread_UNLOCK_MUTEX(table->Mutex);
	 return;
      }
      prev = entry;
      entry = entry->Next;
   }

   _glthread_UNLOCK_MUTEX(table->Mutex);
}



/*
 * Return the key of the "first" entry in the hash table.
 * This is used in the course of deleting all display lists when
 * a context is destroyed.
 */
GLuint _mesa_HashFirstEntry(struct _mesa_HashTable *table)
{
   GLuint pos;
   assert(table);
   _glthread_LOCK_MUTEX(table->Mutex);
   for (pos=0; pos < TABLE_SIZE; pos++) {
      if (table->Table[pos]) {
         _glthread_UNLOCK_MUTEX(table->Mutex);
         return table->Table[pos]->Key;
      }
   }
   _glthread_UNLOCK_MUTEX(table->Mutex);
   return 0;
}



/*
 * Dump contents of hash table for debugging.
 */
void _mesa_HashPrint(const struct _mesa_HashTable *table)
{
   GLuint i;
   assert(table);
   for (i=0;i<TABLE_SIZE;i++) {
      const struct HashEntry *entry = table->Table[i];
      while (entry) {
	 printf("%u %p\n", entry->Key, entry->Data);
	 entry = entry->Next;
      }
   }
}



/*
 * Find a block of 'numKeys' adjacent unused hash keys.
 * Input:  table - the hash table
 *         numKeys - number of keys needed
 * Return:  starting key of free block or 0 if failure
 */
GLuint _mesa_HashFindFreeKeyBlock(struct _mesa_HashTable *table, GLuint numKeys)
{
   GLuint maxKey = ~((GLuint) 0);
   _glthread_LOCK_MUTEX(table->Mutex);
   if (maxKey - numKeys > table->MaxKey) {
      /* the quick solution */
      _glthread_UNLOCK_MUTEX(table->Mutex);
      return table->MaxKey + 1;
   }
   else {
      /* the slow solution */
      GLuint freeCount = 0;
      GLuint freeStart = 1;
      GLuint key;
      for (key=1; key!=maxKey; key++) {
	 if (_mesa_HashLookup(table, key)) {
	    /* darn, this key is already in use */
	    freeCount = 0;
	    freeStart = key+1;
	 }
	 else {
	    /* this key not in use, check if we've found enough */
	    freeCount++;
	    if (freeCount == numKeys) {
               _glthread_UNLOCK_MUTEX(table->Mutex);
	       return freeStart;
	    }
	 }
      }
      /* cannot allocate a block of numKeys consecutive keys */
      _glthread_UNLOCK_MUTEX(table->Mutex);
      return 0;
   }
}



#ifdef HASH_TEST_HARNESS
int main(int argc, char *argv[])
{
   int a, b, c;
   struct HashTable *t;

   printf("&a = %p\n", &a);
   printf("&b = %p\n", &b);

   t = _mesa_NewHashTable();
   _mesa_HashInsert(t, 501, &a);
   _mesa_HashInsert(t, 10, &c);
   _mesa_HashInsert(t, 0xfffffff8, &b);
   _mesa_HashPrint(t);
   printf("Find 501: %p\n", _mesa_HashLookup(t,501));
   printf("Find 1313: %p\n", _mesa_HashLookup(t,1313));
   printf("Find block of 100: %d\n", _mesa_HashFindFreeKeyBlock(t, 100));
   _mesa_DeleteHashTable(t);

   return 0;
}
#endif

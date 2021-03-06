/* Obstack wrapper for GDB.

   Copyright (C) 2002-2018 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#if !defined (GDB_OBSTACK_H)
#define GDB_OBSTACK_H 1

#include "obstack.h"

/* Utility macros - wrap obstack alloc into something more robust.  */

#define OBSTACK_ZALLOC(OBSTACK,TYPE) \
  ((TYPE *) memset (obstack_alloc ((OBSTACK), sizeof (TYPE)), 0, sizeof (TYPE)))

#define OBSTACK_CALLOC(OBSTACK,NUMBER,TYPE) \
  ((TYPE *) memset (obstack_alloc ((OBSTACK), (NUMBER) * sizeof (TYPE)), \
		    0, (NUMBER) * sizeof (TYPE)))

/* Unless explicitly specified, GDB obstacks always use xmalloc() and
   xfree().  */
/* Note: ezannoni 2004-02-09: One could also specify the allocation
   functions using a special init function for each obstack,
   obstack_specify_allocation.  However we just use obstack_init and
   let these defines here do the job.  While one could argue the
   superiority of one approach over the other, we just chose one
   throughout.  */

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free xfree

#define obstack_grow_str(OBSTACK,STRING) \
  obstack_grow (OBSTACK, STRING, strlen (STRING))
#define obstack_grow_str0(OBSTACK,STRING) \
  obstack_grow0 (OBSTACK, STRING, strlen (STRING))

#define obstack_grow_wstr(OBSTACK, WSTRING) \
  obstack_grow (OBSTACK, WSTRING, sizeof (gdb_wchar_t) * gdb_wcslen (WSTRING))

/* Concatenate NULL terminated variable argument list of `const char
   *' strings; return the new string.  Space is found in the OBSTACKP.
   Argument list must be terminated by a sentinel expression `(char *)
   NULL'.  */

extern char *obconcat (struct obstack *obstackp, ...) ATTRIBUTE_SENTINEL;

/* Duplicate STRING, returning an equivalent string that's allocated on the
   obstack OBSTACKP.  */

extern char *obstack_strdup (struct obstack *obstackp, const char *string);

/* An obstack that frees itself on scope exit.  */
struct auto_obstack : obstack
{
  auto_obstack ()
  { obstack_init (this); }

  ~auto_obstack ()
  { obstack_free (this, NULL); }

  /* Free all memory in the obstack but leave it valid for further
     allocation.  */
  void clear ()
  { obstack_free (this, obstack_base (this)); }
};

/* Objects are allocated on obstack instead of heap.  */

struct allocate_on_obstack
{
  allocate_on_obstack () = default;

  void* operator new (size_t size, struct obstack *obstack)
  {
    return obstack_alloc (obstack, size);
  }

  void* operator new[] (size_t size, struct obstack *obstack)
  {
    return obstack_alloc (obstack, size);
  }

  void operator delete (void *memory) {}
  void operator delete[] (void *memory) {}
};

#endif

#ifndef	_LIST_H_
#define _LIST_H_

#include <stdio.h>

typedef struct list_head 
{
	struct list_head *next, *prev;
} LISTHEAD, LISTENTRY, *PLISTHEAD, *PLISTENTRY;

typedef struct slist_head 
{
	struct slist_head *next;
} SLISTHEAD, SLISTENTRY, *PSLISTHEAD, *PSLISTENTRY;

#define LIST_ENTRY(ptr, type, member)			(  (type) ( (char *)(ptr)-(unsigned long)(&((type)0)->member) )   )
#define SLIST_ENTRY(ptr, type, member)			(  (type) ( (char *)(ptr)-(unsigned long)(&((type)0)->member) )   )


static __inline void INIT_LIST_HEAD(PLISTHEAD pListHead)
{
	pListHead->next = pListHead;
	pListHead->prev = pListHead;
}

static __inline void INIT_LIST_ENTRY(PLISTENTRY pListEntry)
{
	pListEntry->next = pListEntry;
	pListEntry->prev = pListEntry;
}

static __inline void INIT_SLIST_HEAD(PSLISTHEAD pListHead)
{
	pListHead->next = NULL;
}

static __inline void INIT_SLIST_ENTRY(PSLISTENTRY pListEntry)
{
	pListEntry->next = NULL;
}

static __inline void _LIST_ADD(	PLISTENTRY	entry,
								PLISTENTRY	prev,
								PLISTENTRY	next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

static __inline void _SLIST_ADD(PSLISTENTRY	entry,
								PSLISTENTRY	prev,
								PSLISTENTRY	next)
{
	entry->next = next;
	prev->next = entry;
}

static __inline void LIST_ADD(PLISTENTRY entry, PLISTHEAD head)
{
	_LIST_ADD(entry, head, head->next);
}

static __inline void SLIST_ADD(PSLISTENTRY entry, PSLISTENTRY head)
{
	_SLIST_ADD(entry, head, head->next);
}

static __inline void LIST_ADD_TAIL(PLISTENTRY entry, PLISTHEAD head)
{
	_LIST_ADD(entry, head->prev, head);
}

static __inline void _LIST_DEL(PLISTENTRY prev, PLISTENTRY next)
{
	next->prev = prev;
	prev->next = next;
}

static __inline void LIST_DEL(PLISTENTRY entry)
{
	_LIST_DEL(entry->prev, entry->next);
	INIT_LIST_ENTRY(entry);
}

static __inline void _SLIST_DEL(PSLISTENTRY prev, PSLISTENTRY next)
{
	prev->next = next;
}

static __inline void SLIST_DEL(PSLISTENTRY entry, PSLISTENTRY prev)
{
	_SLIST_DEL(prev, entry->next);
	INIT_SLIST_ENTRY(entry);
}

static __inline void LIST_MOVE(PLISTENTRY entry, PLISTHEAD head)
{
	_LIST_DEL(entry->prev, entry->next);
	LIST_ADD(entry, head);
}

static __inline PLISTENTRY LIST_CHOP_LAST_ENTRY(PLISTHEAD head)
{
	PLISTENTRY	entry = head->prev;

	if (entry != head)
	{
		LIST_DEL(entry);
		return entry;
	}
	return NULL;
}

static __inline PLISTENTRY LIST_LAST_ENTRY(PLISTHEAD head)
{
	PLISTENTRY	entry = head->prev;

	if (entry != head)
	{
		return entry;
	}
	return NULL;
}

static __inline PSLISTENTRY SLIST_CHOP_FIRST_ENTRY(PSLISTHEAD head)
{
	PSLISTENTRY	entry = head->next;

	if (entry != NULL)
	{
		SLIST_DEL(entry, head);
		return entry;
	}
	return NULL;
}

#define LIST_FOR_EACH(entry, head)		for (entry = (head)->next; entry != (head); entry = entry->next)
#define LIST_FOR_EACH_TAIL(entry, head)		for (entry = (head)->prev; entry != (head); entry = entry->prev)
#define SLIST_FOR_EACH(entry, head)		for (entry = (head)->next; entry != NULL; entry = entry->next)

#endif



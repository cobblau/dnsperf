#ifndef _LIST_H
#define _LIST_H

#define LIST(type) struct { type *head, *tail; }
#define LIST_INIT(list)                    \
	do { (list).head = NULL; (list).tail = NULL; } while (0)

#define LINK(type) struct { type *prev, *next; }
#define LINK_INIT_TYPE(elt, link, type)    \
	do {                                   \
		(elt)->link.prev = (type *)(-1);   \
		(elt)->link.next = (type *)(-1);   \
	} while (0)
#define LINK_INIT(elt, link)               \
	LINK_INIT_TYPE(elt, link, void)
#define LINK_LINKED(elt, link) ((void *)((elt)->link.prev) != (void *)(-1))

#define LIST_HEAD(list) ((list).head)
#define LIST_TAIL(list) ((list).tail)
#define LIST_EMPTY(list) ((list).head == NULL)

#define __LIST_PREPENDUNSAFE(list, elt, link)    \
	do {                                         \
		if ((list).head != NULL)                 \
			(list).head->link.prev = (elt);      \
		else                                     \
			(list).tail = (elt);                 \
		(elt)->link.prev = NULL;                 \
		(elt)->link.next = (list).head;          \
		(list).head = (elt);                     \
	} while (0)

#define LIST_PREPEND(list, elt, link)            \
	do {                                         \
		__LIST_PREPENDUNSAFE(list, elt, link);   \
	} while (0)

#define LIST_INITANDPREPEND(list, elt, link)     \
		__LIST_PREPENDUNSAFE(list, elt, link)

#define __LIST_APPENDUNSAFE(list, elt, link)      \
	do {                                          \
		if ((list).tail != NULL)                  \
			(list).tail->link.next = (elt);       \
		else                                      \
			(list).head = (elt);                  \
		(elt)->link.prev = (list).tail;           \
		(elt)->link.next = NULL;                  \
		(list).tail = (elt);                      \
	} while (0)

#define LIST_APPEND(list, elt, link)              \
	do {                                          \
		__LIST_APPENDUNSAFE(list, elt, link);     \
	} while (0)

#define LIST_INITANDAPPEND(list, elt, link)    \
		__LIST_APPENDUNSAFE(list, elt, link)

#define __LIST_UNLINKUNSAFE_TYPE(list, elt, link, type)     \
	do {                                                    \
		if ((elt)->link.next != NULL)                       \
			(elt)->link.next->link.prev = (elt)->link.prev; \
		else {                                              \
			(list).tail = (elt)->link.prev;                 \
		}                                                   \
		if ((elt)->link.prev != NULL)                       \
			(elt)->link.prev->link.next = (elt)->link.next; \
		else {                                              \
			(list).head = (elt)->link.next;                 \
		}                                                   \
		(elt)->link.prev = (type *)(-1);                    \
		(elt)->link.next = (type *)(-1);                    \
	} while (0)

#define __LIST_UNLINKUNSAFE(list, elt, link)             \
	__LIST_UNLINKUNSAFE_TYPE(list, elt, link, void)

#define LIST_UNLINK_TYPE(list, elt, link, type)             \
	do {                                                    \
		__LIST_UNLINKUNSAFE_TYPE(list, elt, link, type);    \
	} while (0)
#define LIST_UNLINK(list, elt, link)                        \
	LIST_UNLINK_TYPE(list, elt, link, void)

#define LIST_PREV(elt, link) ((elt)->link.prev)
#define LIST_NEXT(elt, link) ((elt)->link.next)

#define __LIST_INSERTBEFOREUNSAFE(list, before, elt, link)    \
	do {                                                      \
		if ((before)->link.prev == NULL)                      \
			LIST_PREPEND(list, elt, link);                    \
		else {                                                \
			(elt)->link.prev = (before)->link.prev;           \
			(before)->link.prev = (elt);                      \
			(elt)->link.prev->link.next = (elt);              \
			(elt)->link.next = (before);                      \
		}                                                     \
	} while (0)

#define LIST_INSERTBEFORE(list, before, elt, link)            \
	do {                                                      \
		__LIST_INSERTBEFOREUNSAFE(list, before, elt, link);   \
	} while (0)

#define __LIST_INSERTAFTERUNSAFE(list, after, elt, link)    \
	do {                                                    \
		if ((after)->link.next == NULL)                     \
			LIST_APPEND(list, elt, link);                   \
		else {                                              \
			(elt)->link.next = (after)->link.next;          \
			(after)->link.next = (elt);                     \
			(elt)->link.next->link.prev = (elt);            \
			(elt)->link.prev = (after);                     \
		}                                                   \
	} while (0)

#define LIST_INSERTAFTER(list, after, elt, link)            \
	do {                                                    \
		__LIST_INSERTAFTERUNSAFE(list, after, elt, link);   \
	} while (0)

#define LIST_APPENDLIST(list1, list2, link)         \
	do {                                            \
		if (LIST_EMPTY(list1))                      \
			(list1) = (list2);                      \
		else if (!LIST_EMPTY(list2)) {              \
			(list1).tail->link.next = (list2).head; \
			(list2).head->link.prev = (list1).tail; \
			(list1).tail = (list2).tail;            \
		}                                           \
		(list2).head = NULL;                        \
		(list2).tail = NULL;                        \
	} while (0)

#define LIST_ENQUEUE(list, elt, link) LIST_APPEND(list, elt, link)
#define __LIST_ENQUEUEUNSAFE(list, elt, link)    \
	__LIST_APPENDUNSAFE(list, elt, link)
#define LIST_DEQUEUE(list, elt, link)            \
	 LIST_UNLINK_TYPE(list, elt, link, void)
#define LIST_DEQUEUE_TYPE(list, elt, link, type) \
	 LIST_UNLINK_TYPE(list, elt, link, type)
#define __LIST_DEQUEUEUNSAFE(list, elt, link)    \
	__LIST_UNLINKUNSAFE_TYPE(list, elt, link, void)
#define __LIST_DEQUEUEUNSAFE_TYPE(list, elt, link, type) \
	__LIST_UNLINKUNSAFE_TYPE(list, elt, link, type)

#endif

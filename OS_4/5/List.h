#ifndef LIST_H
#define LIST_H

typedef struct Node
{
    void *payload;
    struct Node *next;
} Node;

Node *NodeCreate(void *payload);
void NodeFree(Node *node, void (*PayloadDelete)(void*));

typedef struct List
{
    Node *head;
    Node *tail;
} List;

List *ListCreate();
void ListFree(List *list, void (*PayloadDelete)(void*));

int ListEmpty(List *list);
int ListSize(List *list);
int ListContains(List *list, void *payload, int (*PayloadCompare)(const void*, const void*));

void ListPushFront(List *list, void *payload);
void ListPushBack(List *list, void *payload);

void *ListPopFront(List *list);
void *ListPopBack(List *list);

void ListInsert(List *list, void *payload, int (*PayloadCompare)(const void*, const void*));

void *ListRemove(List *list, void *payload, int (*PayloadCompare)(const void*, const void*));

#endif

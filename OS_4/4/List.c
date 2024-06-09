#include "List.h"
#include <stdlib.h>
#include <assert.h>

Node *NodeCreate(void *payload)
{
    Node *node = malloc(sizeof(*node));
    assert(node);
    node->payload = payload;
    node->next = NULL;
    return node;
}

void NodeFree(Node *node, void (*PayloadDelete)(void *))
{
    assert(node);
    PayloadDelete(node->payload);
    free(node);
}

List *ListCreate()
{
    List *list = malloc(sizeof(*list));
    assert(list);
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void ListFree(List *list, void (*PayloadDelete)(void *))
{
    assert(list);
    while (list->head)
    {
        Node *node = list->head;
        list->head = list->head->next;
        NodeFree(node, PayloadDelete);
    }
    free(list);
}

int ListEmpty(List *list)
{
    return list->head == NULL;
}

int ListSize(List *list)
{
    int size = 0;
    Node *iter = list->head;
    while (iter)
    {
        size += 1;
        iter = iter->next;
    }
    return size;
}

int ListContains(List *list, void *payload, int (*PayloadCompare)(const void *, const void *))
{
    Node *iter = list->head;
    while (iter)
    {
        if (PayloadCompare(iter->payload, payload) == 0)
        {
            return 1;
        }
        iter = iter->next;
    }
    return 0;
}

void ListPushFront(List *list, void *payload)
{
    assert(list);
    Node *node = NodeCreate(payload);
    node->next = list->head;
    list->head = node;
    if (list->tail == NULL)
    {
        list->tail = node;
    }
}

void ListPushBack(List *list, void *payload)
{
    assert(list);
    Node *node = NodeCreate(payload);
    if (list->head == NULL)
    {
        list->head = node;
    }
    else
    {
        list->tail->next = node;
    }
    list->tail = node;
}

void *ListPopFront(List *list)
{
    assert(list);
    assert(list->head);
    Node *node = list->head;
    list->head = list->head->next;
    if (list->head == NULL)
    {
        list->tail = NULL;
    }
    void *payload = node->payload;
    free(node);
    return payload;
}

void *ListPopBack(List *list)
{
    assert(list);
    assert(list->tail);

    if (list->head == list->tail)
    {
        return ListPopFront(list);
    }

    Node *iter = list->head;
    while (iter->next != list->tail)
    {
        iter = iter->next;
    }

    Node *node = list->tail;
    void *payload = node->payload;

    iter->next = NULL;
    list->tail = iter;

    free(node);

    return payload;
}

// Inserts an item in the list in the ordered way using PayloadCompare function for comparison
void ListInsert(List *list, void *payload, int (*PayloadCompare)(const void *, const void *))
{
    assert(list);
    if (list->head == NULL || PayloadCompare(payload, list->head->payload) < 0)
    {
        ListPushFront(list, payload);
    }
    else if (PayloadCompare(payload, list->tail->payload) >= 0)
    {
        ListPushBack(list, payload);
    }
    else
    {
        Node *iter = list->head;
        while (PayloadCompare(iter->next->payload, payload) < 0)
        {
            iter = iter->next;
        }

        Node *node = NodeCreate(payload);
        node->next = iter->next;
        iter->next = node;
    }
}

// Remove an item from the list
void *ListRemove(List *list, void *payload, int (*PayloadCompare)(const void *, const void *))
{
    assert(list);
    if (list->head == NULL)
    {
        return NULL;
    }

    if (PayloadCompare(payload, list->head->payload) == 0)
    {
        return ListPopFront(list);
    }

    if (PayloadCompare(payload, list->tail->payload) == 0)
    {
        return ListPopBack(list);
    }

    Node *iter = list->head;
    while (iter->next && PayloadCompare(iter->next->payload, payload) != 0)
    {
        iter = iter->next;
    }

    if (iter->next)
    {
        Node *node = iter->next;
        void *payload = node->payload;
        iter->next = iter->next->next;
        free(node);
        return payload;
    }

    return NULL;
}

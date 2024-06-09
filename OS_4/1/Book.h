#ifndef BOOK_H
#define BOOK_H

#include "Position.h"

typedef struct Book
{
    int id; // unique name
    Position pos;
} Book;

Book * BookCreate(int id, int m, int n, int k);
int BookCompare(const void *, const void *);

#endif

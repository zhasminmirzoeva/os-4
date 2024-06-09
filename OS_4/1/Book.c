#include "Book.h"
#include <stdlib.h>

Book * BookCreate(int id, int m, int n, int k)
{
    Book *book = malloc(sizeof(*book));
    book->id = id;    
    book->pos.m = m;
    book->pos.n = n;
    book->pos.k = k;
    return book;
}

int BookCompare(const void *a, const void *b)
{
    const Book *b1 = (const Book *)a;
    const Book *b2 = (const Book *)b;
    if (b1->id < b2->id)
        return -1;
    if (b1->id > b2->id)
        return 1;
    return 0;
}

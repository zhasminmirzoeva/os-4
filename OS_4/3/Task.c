#include "Task.h"
#include <stdio.h>
#include <stdlib.h>

Task *TaskCreate(int m, int n, int k)
{
    Task *task = malloc(sizeof(*task));
    task->m = m;
    task->n = n;    
    task->k = k;
    return task;
}

int TaskParse(const char *str, Task *task)
{
    if (sscanf(str, "%d:%d:%d", &task->m, &task->n, &task->k) != 3)
    {
        return 0;
    }
    return 1;
}

void TaskCreateMessage(char *str, Task *task)
{
    sprintf(str, "%d:%d:%d", task->m, task->n, task->k);
}

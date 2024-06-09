#ifndef TASK_H
#define TASK_H

#include "Position.h"

// Structure representing the task for a client:
// need to find the name of book at the given coordinates
typedef struct Position Task;

Task *TaskCreate();

int TaskParse(const char *str, Task *task);

void TaskCreateMessage(char *str, Task *task);

#endif

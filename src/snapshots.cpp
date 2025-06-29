#include "snapshots.h"

/* TODO */
/*
void 
undo_stack_push(Snapshots* self, Anm2* anm2)
{
    if (self->top >= UNDO_STACK_MAX)
    {
        memmove(&self->snapshots[0], &self->snapshots[1], sizeof(Anm2) * (UNDO_STACK_MAX - 1));
        self->top = UNDO_STACK_MAX - 1;
    }

    self->snapshots[self->top++] = *anm2;
}

bool
undo_stack_pop(Snapshots* self, Anm2* anm2)
{
    if (self->top == 0)
        return false;

    *anm2 = self->snapshots[--self->top];

    return true;
}
*/
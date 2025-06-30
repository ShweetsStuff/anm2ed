#include "snapshots.h"

void 
snapshots_undo_stack_push(Snapshots* self, Snapshot* snapshot)
{
    if (self->undoStack.top >= SNAPSHOT_STACK_MAX)
    {
        memmove(&self->undoStack.snapshots[0], &self->undoStack.snapshots[1], sizeof(Snapshot) * (SNAPSHOT_STACK_MAX - 1));
        self->undoStack.top = SNAPSHOT_STACK_MAX - 1;
    }

    self->undoStack.snapshots[self->undoStack.top++] = *snapshot;
    self->redoStack.top = 0; 
}

bool
snapshots_undo_stack_pop(Snapshots* self, Snapshot* snapshot)
{
    if (self->undoStack.top == 0)
        return false;

    *snapshot = self->undoStack.snapshots[--self->undoStack.top];

    return true;
}

void
snapshots_redo_stack_push(Snapshots* self, Snapshot* snapshot)
{
    if (self->redoStack.top >= SNAPSHOT_STACK_MAX)
    {
        memmove(&self->redoStack.snapshots[0], &self->redoStack.snapshots[1], sizeof(Snapshot) * (SNAPSHOT_STACK_MAX - 1));
        self->redoStack.top = SNAPSHOT_STACK_MAX - 1;
    }

    self->redoStack.snapshots[self->redoStack.top++] = *snapshot;
}

bool
snapshots_redo_stack_pop(Snapshots* self, Snapshot* snapshot)
{
    if (self->redoStack.top == 0)
        return false;

    *snapshot = self->redoStack.snapshots[--self->redoStack.top];
    return true;
}

void 
snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, f32* time, Input* input)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->time = time;
    self->input = input;
}

void
snapshots_tick(Snapshots* self)
{
    /* Undo */
    if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_UNDO]))
    {
        Snapshot snapshot;
        if (snapshots_undo_stack_pop(self, &snapshot))
        {
            Snapshot current = {*self->anm2, *self->reference, *self->time};
            snapshots_redo_stack_push(self, &current);

            *self->anm2 = snapshot.anm2;
            *self->reference = snapshot.reference;
            *self->time = snapshot.time;
        }
    }

    /* Redo */
    if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_REDO]))
    {
        Snapshot snapshot;
        if (snapshots_redo_stack_pop(self, &snapshot))
        {
            Snapshot current = {*self->anm2, *self->reference, *self->time};
            snapshots_undo_stack_push(self, &current);

            *self->anm2 = snapshot.anm2;
            *self->reference = snapshot.reference;
            *self->time = snapshot.time;
        }
    }
}
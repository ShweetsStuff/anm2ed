#include "snapshots.h"

// Pushes the undo stack
void 
snapshots_undo_stack_push(Snapshots* self, Snapshot* snapshot)
{
    // If stack over the limit, shift it
    if (self->undoStack.top >= SNAPSHOT_STACK_MAX)
    {
        for (s32 i = 0; i < SNAPSHOT_STACK_MAX - 1; i++)
            self->undoStack.snapshots[i] = self->undoStack.snapshots[i + 1];

        self->undoStack.top = SNAPSHOT_STACK_MAX - 1;
    }

    self->undoStack.snapshots[self->undoStack.top++] = *snapshot;
    self->redoStack.top = 0; 
}

// Pops the undo stack
bool
snapshots_undo_stack_pop(Snapshots* self, Snapshot* snapshot)
{
    if (self->undoStack.top == 0)
        return false;

    *snapshot = self->undoStack.snapshots[--self->undoStack.top];

    return true;
}

// Pushes the redo stack
void
snapshots_redo_stack_push(Snapshots* self, Snapshot* snapshot)
{
    if (self->redoStack.top >= SNAPSHOT_STACK_MAX)
    {
        for (s32 i = 0; i < SNAPSHOT_STACK_MAX - 1; i++)
            self->redoStack.snapshots[i] = self->redoStack.snapshots[i + 1];
        self->redoStack.top = SNAPSHOT_STACK_MAX - 1;
    }

    self->redoStack.snapshots[self->redoStack.top++] = *snapshot;
}

// Pops the redo stack
bool
snapshots_redo_stack_pop(Snapshots* self, Snapshot* snapshot)
{
    if (self->redoStack.top == 0)
        return false;

    *snapshot = self->redoStack.snapshots[--self->redoStack.top];
    return true;
}

// Initializes snapshots
void 
snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, f32* time, Input* input)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->time = time;
    self->input = input;
}

// Ticks snapshots
void
snapshots_tick(Snapshots* self)
{
    /* Undo */
    if (input_press(self->input, INPUT_UNDO))
        self->isUndo = true;
    
    // isUndo disconnected, if another part of the program wants to set it 
    if (self->isUndo)
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

        self->isUndo = false;
    }

    /* Redo */
    if (input_press(self->input, INPUT_REDO))
        self->isRedo = true;

    // isRedo disconnected, if another part of the program wants to set it 
    if (self->isRedo)
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

        self->isRedo = false;
    }
}
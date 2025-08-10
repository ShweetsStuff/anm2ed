#include "snapshots.h"

static void _snapshot_stack_push(SnapshotStack* stack, const Snapshot* snapshot)
{
    if (stack->top >= SNAPSHOT_STACK_MAX)
    {
        for (s32 i = 0; i < SNAPSHOT_STACK_MAX - 1; i++)
            stack->snapshots[i] = stack->snapshots[i + 1];
        stack->top = SNAPSHOT_STACK_MAX - 1;
    }
    stack->snapshots[stack->top++] = *snapshot;
}

static bool _snapshot_stack_pop(SnapshotStack* stack, Snapshot* snapshot)
{
    if (stack->top == 0) return false;

    *snapshot = stack->snapshots[--stack->top];
    return true;
}

static void _snapshot_set(Snapshots* self, const Snapshot& snapshot)
{
    *self->anm2 = snapshot.anm2;
    *self->reference = snapshot.reference;
    self->preview->time = snapshot.time;
    self->action = snapshot.action;
}

void snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, Preview* preview)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->preview = preview;
}

void snapshots_reset(Snapshots* self)
{
    self->undoStack = SnapshotStack{};
    self->redoStack = SnapshotStack{};
    self->action.clear();
}

void snapshots_undo_stack_push(Snapshots* self, const Snapshot* snapshot)
{
    _snapshot_stack_push(&self->undoStack, snapshot);
    self->redoStack.top = 0;
}

void snapshots_undo(Snapshots* self)
{
    Snapshot snapshot;
    if (_snapshot_stack_pop(&self->undoStack, &snapshot))
    {
        Snapshot current = {*self->anm2, *self->reference, self->preview->time, self->action};
        _snapshot_stack_push(&self->redoStack, &current);
        _snapshot_set(self, snapshot);
    }
}

void snapshots_redo(Snapshots* self)
{
    Snapshot snapshot;
    if (_snapshot_stack_pop(&self->redoStack, &snapshot))
    {
        Snapshot current = {*self->anm2, *self->reference, self->preview->time, self->action};
        _snapshot_stack_push(&self->undoStack, &current);
        _snapshot_set(self, snapshot);
    }
}
#pragma once

#include "anm2.h"

#define SNAPSHOT_STACK_MAX 100

struct SnapshotStack
{
    Anm2 snapshots[SNAPSHOT_STACK_MAX];
    s32 top = 0;
};

struct Snapshots
{
    SnapshotStack undoStack;
    SnapshotStack redoStack;
}



void undo_stack_push(UndoStack* self, Anm2* anm2);
bool undo_stack_pop(UndoStack* self, Anm2* anm2);
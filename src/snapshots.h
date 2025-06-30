#pragma once

#include "anm2.h"
#include "input.h"

#define SNAPSHOT_STACK_MAX 100

struct Snapshot
{
    Anm2 anm2;
    Anm2Reference reference;
    f32 time;
};

struct SnapshotStack
{
    Snapshot snapshots[SNAPSHOT_STACK_MAX];
    s32 top = 0;
};

struct Snapshots
{
    Anm2* anm2 = NULL;
    Anm2Reference* reference = NULL;
    f32* time = NULL;
    Input* input = NULL;
    SnapshotStack undoStack;
    SnapshotStack redoStack;
};

void snapshots_undo_stack_push(Snapshots* self, Snapshot* snapshot);
bool snapshots_undo_stack_pop(Snapshots* self, Snapshot* snapshot);
void snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, f32* time, Input* input);
void snapshots_tick(Snapshots* self);
#pragma once

#include "anm2.h"
#include "preview.h"
#include "texture.h"

#define SNAPSHOT_STACK_MAX 1000
#define SNAPSHOT_ACTION "Action"

struct Snapshot
{
    Anm2 anm2;
    Anm2Reference reference;
    f32 time = 0.0f;
    std::string action = SNAPSHOT_ACTION;
};

struct SnapshotStack
{
    Snapshot snapshots[SNAPSHOT_STACK_MAX];
    s32 top = 0;

    bool is_empty() const { return top == 0; }
};

struct Snapshots
{
    Anm2* anm2 = nullptr;
    Preview* preview = nullptr;
    Anm2Reference* reference = nullptr;
    std::string action = SNAPSHOT_ACTION;
    SnapshotStack undoStack;
    SnapshotStack redoStack;
};

void snapshots_undo_push(Snapshots* self, const Snapshot* snapshot);
void snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, Preview* preview);
void snapshots_undo(Snapshots* self);
void snapshots_redo(Snapshots* self);
void snapshots_reset(Snapshots* self);
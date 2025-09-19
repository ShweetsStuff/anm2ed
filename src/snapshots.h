#pragma once

#include "anm2.h"
#include "preview.h"

#define SNAPSHOT_STACK_MAX 100
#define SNAPSHOT_ACTION "Action"

struct Snapshot {
  Anm2 anm2;
  Anm2Reference reference;
  float time = 0.0f;
  std::string action = SNAPSHOT_ACTION;
};

struct SnapshotStack {
  Snapshot snapshots[SNAPSHOT_STACK_MAX];
  int top = 0;

  bool is_empty() const { return top == 0; }
};

struct Snapshots {
  Anm2* anm2 = nullptr;
  Preview* preview = nullptr;
  Anm2Reference* reference = nullptr;
  std::string action = SNAPSHOT_ACTION;
  SnapshotStack undoStack;
  SnapshotStack redoStack;
};

void snapshots_undo_push(Snapshots* self, Snapshot* snapshot);
void snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, Preview* preview);
void snapshots_undo(Snapshots* self);
void snapshots_redo(Snapshots* self);
void snapshots_reset(Snapshots* self);
Snapshot snapshot_get(Snapshots* self);
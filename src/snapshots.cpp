#include "snapshots.h"

static void _snapshot_stack_push(SnapshotStack* stack, Snapshot* snapshot) {
  if (stack->top >= SNAPSHOT_STACK_MAX) {
    for (int i = 0; i < SNAPSHOT_STACK_MAX - 1; i++)
      stack->snapshots[i] = stack->snapshots[i + 1];
    stack->top = SNAPSHOT_STACK_MAX - 1;
  }
  stack->snapshots[stack->top++] = *snapshot;
}

static Snapshot* _snapshot_stack_pop(SnapshotStack* stack) {
  if (stack->top == 0)
    return nullptr;
  return &stack->snapshots[--stack->top];
}

static void _snapshot_set(Snapshots* self, Snapshot* snapshot) {
  if (!snapshot)
    return;

  *self->anm2 = snapshot->anm2;
  *self->reference = snapshot->reference;
  self->preview->time = snapshot->time;
  self->action = snapshot->action;

  anm2_spritesheet_texture_pixels_upload(self->anm2);
}

Snapshot snapshot_get(Snapshots* self) {
  Snapshot snapshot = {*self->anm2, *self->reference, self->preview->time, self->action};
  anm2_spritesheet_texture_pixels_download(&snapshot.anm2);
  return snapshot;
}

void snapshots_init(Snapshots* self, Anm2* anm2, Anm2Reference* reference, Preview* preview) {
  self->anm2 = anm2;
  self->reference = reference;
  self->preview = preview;
}

void snapshots_reset(Snapshots* self) {
  self->undoStack = SnapshotStack{};
  self->redoStack = SnapshotStack{};
  self->action.clear();
}

void snapshots_undo_push(Snapshots* self, Snapshot* snapshot) {
  self->redoStack.top = 0;
  _snapshot_stack_push(&self->undoStack, snapshot);
}

void snapshots_undo(Snapshots* self) {
  if (Snapshot* snapshot = _snapshot_stack_pop(&self->undoStack)) {
    Snapshot current = snapshot_get(self);
    _snapshot_stack_push(&self->redoStack, &current);
    _snapshot_set(self, snapshot);
  }
}

void snapshots_redo(Snapshots* self) {
  if (Snapshot* snapshot = _snapshot_stack_pop(&self->redoStack)) {
    Snapshot current = snapshot_get(self);
    _snapshot_stack_push(&self->undoStack, &current);
    _snapshot_set(self, snapshot);
  }
}
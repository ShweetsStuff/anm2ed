# Manual UI checklist

Run before merging any stage. The automated suite covers file I/O and document operations; this covers what only the UI exercises.
For each step, also check that **undo** restores the prior state and **redo** reapplies it.

## Files
- [ ] New, open, save, save-as, close with unsaved changes (prompt appears).
- [ ] Open an older file containing `<SourceDocument>`; save; the game-facing part is unchanged apart from intended edits.
- [ ] Merge file (each preset); append frames / merge animations does not duplicate groups.
- [ ] Reopen recent files; autosave restore.

## Animations
- [ ] Add, duplicate, remove, rename, reorder (drag), set default.
- [ ] Copy/paste animations within and across documents.
- [ ] Merge animations (prepend/append/replace/ignore).
- [ ] Animation groups: create, rename, collapse, drag animations in and out.

## Items (layers / nulls / events / sounds)
- [ ] Add, remove, rename, reorder; remove unused.
- [ ] Copy/paste; layer spritesheet reassignment; null ShowRect toggle.

## Timeline
- [ ] Select frames (click, shift, ctrl, box); select across tracks.
- [ ] Insert, delete, duplicate, move (drag), resize duration (drag edge).
- [ ] Copy/paste frames; paste onto another track.
- [ ] Bake, split, interpolation set (none/linear/ease in/out/in-out).
- [ ] Triggers: add, move, assign event and sounds.
- [ ] Layer/null groups: create, nest tracks, group root frames, hide/show, expand/collapse.
- [ ] Playback, loop, scrub; track visibility toggles.

## Frame properties
- [ ] Edit each field on one frame and on a multi-selection; flip X/Y.
- [ ] Change all frame properties wizard (adjust/add/subtract/multiply/divide).

## Spritesheets / regions
- [ ] Add, reload, replace, remove; remove unused.
- [ ] Pack spritesheets; merge spritesheets (regions remap correctly).
- [ ] Regions: add, edit crop/pivot, each origin, generate from frames, scan, remove unused.
- [ ] Spritesheet editor: crop/pivot drag tools, zoom/pan, grid.

## Shaders
- [ ] Add, remove, edit uniforms/components; assign to frames; preview renders.

## Preview / canvas
- [ ] Zoom, pan, center, onion skin, overlay, background, grid.
- [ ] Move/rotate/scale tools on frames; root transform.

## Wizards
- [ ] Generate animation from grid; render animation (image sequence / spritesheet / video).
- [ ] Configure: settings persist across restart.

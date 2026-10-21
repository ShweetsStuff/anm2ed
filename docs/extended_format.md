# Anm2Ed extended format (ExtendedVersion 2)

An extended `.anm2` is the editor's own document, saved alone. It is the Rebirth `.anm2` XML with extra elements and attributes. This page lists what it adds and how the editor plays it, so that a converter or a game's loader can match the editor exactly.

A Rebirth document is different: it saves the game's copy, followed by the extended document embedded as `<SourceDocument>`. A loader for extended files can ignore it.

## Recognizing one
```xml
<AnimatedActor>
  <Info CreatedBy="…" CreatedOn="…" Fps="30" Version="…" ExtendedVersion="2"/>
  …
```
- **`ExtendedVersion`** is present only on extended documents.
- **Version 1** files can only use the features marked *(v1)*.
- **Unknown elements and attributes** are kept by the editor and written back unchanged.

## Content
```xml
<Content>
  <Spritesheets>
    <Spritesheet Id="0" Path="parts.png"/>
    <Spritesheet Id="1" Path="body.svg">
      <Region Id="0" Name="head" XCrop="48" YCrop="0" Width="32" Height="32" Origin="Center"/>
    </Spritesheet>
  </Spritesheets>
  <Layers>
    <Layer Id="0" Name="torso" SpritesheetId="1"/>
    <Layer Id="1" Name="hx" SpritesheetId="0" Animation="Head"/>
    <Layer Id="2" Name="shadow" SpritesheetId="0" Library="0" Animation="SmallShadow"/>
  </Layers>
  <Nulls>…</Nulls>
  <Events>
    <Event Id="0" Name="shoot"/>
    <Event Id="1" Name="done"/>
  </Events>
  <Sounds>…</Sounds>
  <Libraries>
    <Library Id="0" Path="shared/shadows.anm2"/>
  </Libraries>
</Content>
```

**SVG spritesheets**
- A spritesheet whose path ends in `.svg` is an SVG.
- One SVG unit is one pixel. Crops, sizes and pivots are in SVG units.
- The editor draws an SVG at whatever resolution it is shown at.

**Regions** *(v1)*
- `Origin` is `TopLeft`, `Center`, or absent (custom, with `XPivot`/`YPivot`).
- A layer frame with `RegionId` takes the region's crop, size and pivot.

**Libraries**
- A library is another `.anm2` whose animations layers can play.
- `Path` is relative to the file that names it, so a library's own libraries are relative to that library.

**Layers**
- A layer either shows its spritesheet, or plays an animation when `Animation` is set.
- **Same file or library:** with no `Library`, the animation comes from this file. With `Library`, it comes from that library.
- **`SpritesheetId`** is still written on animation layers (the game expects it) but is not used.
- **Instance name:** the layer's `Name` is the name code should use for the instance.
- **No self-nesting:** an animation never plays inside itself, even through other animations. That draw is skipped.

## Animations
```xml
<Animations DefaultAnimation="Walk">
  <Animation Name="Walk" FrameNum="24" Loop="true" StartMarker="4" EndMarker="19">
    <RootAnimation>…frames…</RootAnimation>
    <LayerAnimations>
      <LayerAnimation LayerId="0" Visible="true">…frames…</LayerAnimation>
      <Group Name="arms" IsExpanded="true" Visible="true">
        <RootAnimation>…frames…</RootAnimation>
        <LayerAnimation LayerId="3" Visible="true">…</LayerAnimation>
      </Group>
    </LayerAnimations>
    <NullAnimations>…</NullAnimations>
    <Triggers>
      <Trigger EventId="0" AtFrame="12"><Sound Id="0"/></Trigger>
      <Trigger EventId="0" AtFrame="23"><Event Id="1"/></Trigger>
    </Triggers>
  </Animation>
</Animations>
```

**Markers** *(v1)*
- `StartMarker`/`EndMarker` keep editor playback between those frames, inclusive. They are editor-only.

**Groups** *(v1)*
- A group nests tracks and has its own `RootAnimation`, a transform applied to its tracks after the animation's root.

**Layer order**
- Layers draw in `LayerAnimations` order: the first one is drawn first, so it ends up at the bottom.
- `Depth` on a layer frame moves it among the layers for that frame. The sort key is (position in the list + depth, depth).

**Triggers**
- A trigger's events are its `EventId`, followed by its `<Event Id/>` children in order. All of them fire on `AtFrame`.
- `<Sound Id/>` children are the trigger's sounds. The editor picks one at random.
- An older single `SoundId` attribute is still read.

## Frames
Attributes on `<Frame>` (layer, null and root tracks):

| Attribute | Meaning |
|---|---|
| `XPosition`, `YPosition` | Position in pixels; y points down. |
| `XPivot`, `YPivot` | Pivot in the frame's pixels (layer frames). |
| `XCrop`, `YCrop`, `Width`, `Height` | The spritesheet rectangle (layer frames). |
| `XScale`, `YScale` | Scale in percent. A negative value flips. |
| `Rotation` | Degrees, clockwise on screen. |
| `SkewX`, `SkewY` *(v1)* | Skew in percent: `x' = x + SkewX/100·y`, `y' = y + SkewY/100·x`. Older files say `ShearX`/`ShearY`. |
| `RedTint` … `AlphaTint` | 0–255, multiplied. |
| `RedOffset` … `BlueOffset` | 0–255, added after the tint. |
| `AlphaOffset` | 0–255 (may be negative), added to alpha after the tint; the result is clamped to 0–1. |
| `Blur` | Flash's low-quality BlurFilter: a box this many canvas pixels wide, averaged over the frame's crop. |
| `Depth` *(v1)* | Layer frames only; see Layer order. |
| `ShaderId` *(v1)* | A custom shader from `<Shaders>`. |
| `Delay` | Duration in frames. |
| `Visible` | Hidden frames draw nothing. |
| `Interpolated` | `false`, `true` (linear), `EaseIn` (t²), `EaseOut` (1−(1−t)²) or `EaseInOut`. It blends position, scale, skew, rotation, tint, color offset and blur toward the next frame. |

**A layer frame's transform.** A point `p` of the spritesheet crop, in pixels, lands at:
```
world = Root · GroupRoot · ( position + Flip · Rotate · Skew · |scale| · (p − pivot) )
```
- `Flip` holds the signs of the scale. When exactly one axis is flipped, the rotation turns the other way.
- A root's transform is `position + Flip · Rotate · Skew · |scale| · p`. A root has no pivot.

### Animation layer frames
On a layer that plays an animation, a frame places and plays the child:

| Attribute | Default | Meaning |
|---|---|---|
| `ChildAnimation` | the layer's | The animation this keyframe plays, from the same file or library. |
| `ChildMode` | `Loop` | `Loop`, `Once` (stops on its last frame) or `Hold` (stays on the start frame). |
| `ChildStart` | `0` | The child's frame when its clock starts, or `Random`. |
| `Restart` | `false` | Restarts the child's clock at this keyframe. |

**Placement.** The child's whole drawing goes through the frame's transform, with the frame's pivot as the child's origin:
```
world = Root · GroupRoot · ( position + Flip · Rotate · Skew · |scale| · (q − pivot) )
```
- `q` is a point in the child's space, after the child's own root and groups.
- **Colour:** the frame's tint multiplies the child's colour, and its colour offset adds to it.
- **Unused fields:** crop, size, region and shader are not used.

**Timing.** The child's time when the parent is at time `t`:
1. **Clock start.** The child's clock starts at the first keyframe. It restarts at any keyframe with `Restart="true"`, or whose `ChildAnimation` differs from the previous keyframe's. Each start begins at that keyframe's `ChildStart`.
2. **Child time.** Take the start keyframe that most recently began (call its start frame `S` and its `ChildStart` `C`). The child time is `C + (t − S)`.
3. **Mode.** The keyframe playing at `t` decides the mode:
   - `Loop` wraps the time over the child's `FrameNum`.
   - `Once` stops at `FrameNum − 1`.
   - `Hold` uses that keyframe's own `ChildStart`.
4. **Random start.** A `Random` start is `(layerId·73856093 xor keyframeIndex·19349663) mod FrameNum`, using unsigned 32-bit arithmetic.

**Child triggers.** A child fires its trigger on each frame it reaches. That is the frames where its whole-number time, or its animation, differs from the previous parent frame's, and always on parent frame 0. A held child fires once.

## Rebirth copies
A Rebirth document's game copy keeps only what the game reads:
- **Animation layers** are flattened into plain layers, one per layer the child draws, named `layer/childLayer`. Their frames are baked one per frame and skew is dropped. A library's spritesheets are added with paths relative to the document.
- **Children's trigger events** join the animation's triggers, matched by name.
- **Triggers** keep one event each: the first.
- **Dropped attributes:** markers, depth, skew, alpha offset, blur, shaders, extra events, trigger sounds and libraries.
- **SVG spritesheets** are offered for conversion to PNGs before saving.

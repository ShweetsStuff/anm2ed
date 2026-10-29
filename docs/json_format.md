# Anm2Ed JSON format (version 1)

The JSON format is the editor's own document, saved alone when **Save as JSON** is on (Configure → Serialization). It holds everything the editor knows about a document; the game can't read it. This page lists its shape and how the editor plays it, so that a converter or a game's loader can match the editor exactly.

A regular save is different: it writes the game's `.anm2` XML, with the editor's copy embedded as `<SourceDocument>`. See [XML saves](#xml-saves).

## Recognizing one
```json
{
  "anm2ed": 1,
  "info": {"createdBy": "…", "createdOn": "2026-01-01T12:00:00", "fps": 30, "version": 2},
  …
}
```
- **`anm2ed`** is the format version. A JSON file without it is not opened.
- **Defaults are left out.** Any key missing from an object has its default (listed below).
- **Ids are indices.** An item is referred to by its position in its array, starting at 0.
- **Unknown keys, wrong types, unknown values and out-of-range indices** are reported by where they are (e.g. `animations.Idle.layers[0].frames[2].pos: expected an array of 2 numbers`); the document still opens, with the default in their place.
- **`createdOn`** is ISO 8601 local time.
- **`info.isaacIgnored`** is `true` when the document was last saved with Ignore and Save; see [XML saves](#xml-saves).

## Content
```json
"spritesheets": [
  {"path": "parts.png"},
  {
    "path": "body.svg",
    "regions": [
      {"name": "head", "crop": [48, 0], "size": [32, 32], "pivot": [16, 16], "origin": "center"}
    ]
  }
],
"shaders": [
  {"name": "glow", "vertex": "glow.vs", "fragment": "glow.fs", "uniforms": [{"name": "strength", "value": "0.5"}]}
],
"libraries": [
  "shared/shadows.anm2"
],
"layers": [
  {"name": "torso", "sheet": 1},
  {"name": "hx", "animation": "Head"},
  {"name": "shadow", "library": 0, "animation": "SmallShadow"}
],
"nulls": [
  {"name": "hitbox", "showRect": true}
],
"events": [
  "shoot",
  "done"
],
"sounds": [
  "sounds/step.ogg"
]
```

| Item | Key | Default | Meaning |
|---|---|---|---|
| Spritesheet | `path` | | The image, relative to the document. |
| Region | `name` | `""` | |
| | `crop`, `size`, `pivot` | `[0, 0]` | The rectangle in the spritesheet, and its pivot. |
| | `origin` | custom | `topLeft` or `center` sets the pivot from the size. |
| Shader | `name`, `vertex`, `fragment` | `""` | A custom shader and its source files. |
| | `uniforms` | none | `{ name, binding, value, components: [ { index, binding, value } ] }` |
| Layer | `name` | `""` | The name code should use for the layer (or its instance). |
| | `sheet` | `0` | Its spritesheet; `-1` for one that is missing. |
| | `animation` | none | Makes it an animation layer, playing this animation. |
| | `library` | none | Where `animation` comes from; none means this file. |
| Null | `name` | `""` | |
| | `showRect` | `false` | |
| Event, sound, library | | | A name, or a path relative to the document. |

**SVG spritesheets**
- A spritesheet whose path ends in `.svg` is an SVG.
- One SVG unit is one pixel. Crops, sizes and pivots are in SVG units.
- The editor draws an SVG at whatever resolution it is shown at.

**Libraries**
- A library is another `.anm2` (XML or JSON) whose animations layers can play.
- A path is relative to the file that names it, so a library's own libraries are relative to that library.

**Layers**
- A layer either shows its spritesheet, or plays an animation when `animation` is set.
- **No self-nesting:** an animation never plays inside itself, even through other animations. That draw is skipped.

## Animations
Animations are an object keyed by their name; names are unique (a repeated one is saved as `"Name 2"`, `"Name 3"`…). A key whose object has an `animations` object is a group of animations.
```json
"defaultAnimation": "Walk",
"animations": {
  "Walk": {
    "length": 24, "startMarker": 4, "endMarker": 19,
    "root": […frames…],
    "layers": [
      {"layer": 0, "frames": […frames…]},
      {"group": "arms", "root": […frames…], "layers": [{"layer": 3, "frames": […]}]}
    ],
    "nulls": [{"null": 0, "frames": […]}],
    "triggers": [
      {"at": 12, "events": [0], "sounds": [0]},
      {"at": 23, "events": [0, 1]}
    ]
  },
  "Combat": {"expanded": false, "animations": {"Attack": {…}}}
}
```

| Key | Default | Meaning |
|---|---|---|
| `length` | `1` | Frames in the animation. |
| `loop` | `true` | |
| `startMarker`, `endMarker` | none | Editor playback stays between these frames, inclusive. Editor-only. |
| `root`, `triggers` | none | Frame arrays. |
| `layers`, `nulls` | none | Tracks and groups, in order. |
| Track `layer` / `null` | | The item the track animates. |
| Track `visible` | `true` | |
| Group `group` | | The group's name; always present. |
| Group `expanded`, `visible` | `true` | Timeline state. |
| Group `root` | one default frame | A transform applied to its tracks after the animation's root. |
| Trigger `at` | | The frame it fires on. |
| Trigger `events` | none | Every event fires on `at`. |
| Trigger `sounds` | none | The editor picks one at random. |

**Layer order**
- Layers draw in `layers` order: the first one is drawn first, so it ends up at the bottom.
- `depth` on a layer frame moves it among the layers for that frame. The sort key is (position in the list + depth, depth).

## Frames
Keys on a frame of a layer, null or root track:

| Key | Default | Meaning |
|---|---|---|
| `pos` | `[0, 0]` | Position in pixels; y points down. |
| `region` | none | Layer frames: a region of the layer's spritesheet. The frame takes its crop, size and pivot, and has none of its own. |
| `crop`, `size` | `[0, 0]` | Layer frames without a region: the spritesheet rectangle. |
| `pivot` | `[0, 0]` | Layer frames without a region: the pivot in the frame's pixels. |
| `scale` | `[100, 100]` | Percent. A negative value flips. |
| `rotation` | `0` | Degrees, clockwise on screen. |
| `skew` | `[0, 0]` | Percent: `x' = x + skew.x/100·y`, `y' = y + skew.y/100·x`. |
| `tint` | `[255, 255, 255, 255]` | RGBA, multiplied. |
| `offset` | `[0, 0, 0, 0]` | RGBA, added after the tint. Alpha may be negative; the result is clamped to 0–1. |
| `blur` | `0` | Layer frames: Flash's low-quality BlurFilter, a box this many canvas pixels wide, averaged over the frame's crop. |
| `depth` | `0` | Layer frames; see Layer order. |
| `shader` | none | A custom shader from `shaders`. |
| `delay` | `1` | Duration in frames. |
| `visible` | `true` | Hidden frames draw nothing. |
| `interpolation` | none | `linear`, `easeIn` (t²), `easeOut` (1−(1−t)²) or `easeInOut`. It blends position, scale, skew, rotation, tint, color offset and blur toward the next frame. |

**A layer frame's transform.** A point `p` of the spritesheet crop, in pixels, lands at:
```
world = Root · GroupRoot · ( pos + Flip · Rotate · Skew · |scale| · (p − pivot) )
```
- `Flip` holds the signs of the scale. When exactly one axis is flipped, the rotation turns the other way.
- A root's transform is `pos + Flip · Rotate · Skew · |scale| · p`. A root has no pivot.

### Animation layer frames
On a layer that plays an animation, a frame places and plays the child:

| Key | Default | Meaning |
|---|---|---|
| `child` | the layer's | The animation this keyframe plays, from the same file or library. |
| `childMode` | `loop` | `loop`, `once` (stops on its last frame) or `hold` (stays on the start frame). |
| `childStart` | `0` | The child's frame when its clock starts. |
| `childStartRandom` | `false` | Start on a random frame instead. |
| `restart` | `false` | Restarts the child's clock at this keyframe. |

**Placement.** The child's whole drawing goes through the frame's transform, with the frame's pivot as the child's origin:
```
world = Root · GroupRoot · ( pos + Flip · Rotate · Skew · |scale| · (q − pivot) )
```
- `q` is a point in the child's space, after the child's own root and groups.
- **Colour:** the frame's tint multiplies the child's colour, and its offset adds to it.
- **Unused keys:** crop, size, region and shader are not used.

**Timing.** The child's time when the parent is at time `t`:
1. **Clock start.** The child's clock starts at the first keyframe. It restarts at any keyframe with `restart`, or whose `child` differs from the previous keyframe's. Each start begins at that keyframe's `childStart`.
2. **Child time.** Take the start keyframe that most recently began (call its start frame `S` and its `childStart` `C`). The child time is `C + (t − S)`.
3. **Mode.** The keyframe playing at `t` decides the mode:
   - `loop` wraps the time over the child's `length`.
   - `once` stops at `length − 1`.
   - `hold` uses that keyframe's own `childStart`.
4. **Random start.** A random start is `(layerIndex·73856093 xor keyframeIndex·19349663) mod length`, using unsigned 32-bit arithmetic.

**Child triggers.** A child fires its trigger on each frame it reaches. That is the frames where its whole-number time, or its animation, differs from the previous parent frame's, and always on parent frame 0. A held child fires once.

## XML saves
A regular save writes the game's `.anm2` XML, keeping only what the game reads, followed by the editor's full copy as `<SourceDocument>` (the same data as the JSON, in XML):
- **Default colors** (tints of 255, color offsets of 0) are left out of both copies, since readers assume them.
- **Animation layers** are flattened into plain layers, one per layer the child draws, named `layer/childLayer`. Their frames are baked one per frame and skew is dropped. A library's spritesheets are added with paths relative to the document.
- **Children's trigger events** join the animation's triggers, matched by name.
- **Triggers** keep one event each: the first.
- **Depth** is stood in for by copies of the layer, named for the depth (`body (depth +1)`), placed where the editor draws it, and each showing only the frames at its depth. Groups in such an animation have their roots baked into their tracks.
- **Alpha offsets** join the alpha tint (clamped), which matches on opaque pixels.
- **Skew, blur and shaders** are baked: each layer frame using one is drawn alone at 1x into `<name>.baked.png` beside the save (identical images once), and stands on a copy of its layer (`body (baked)`, on that spritesheet) right after it, one frame per step, while the layer hides those frames. A shaded frame's tint and color offset go into its image; others keep theirs. A skewed root (or group root) goes into its layers' images, and becomes plain; nulls take its transform without the skew. Children of animation layers are flattened first, without their skew. Baking needs OpenGL (`convert` makes a context); without it, these are dropped. Autosaves don't bake.
- These are what **Convert and Save** does; **Ignore and Save**, in the save's Isaac compatibility warning, leaves them out. The document remembers which (`info.isaacIgnored`) for saves that don't ask (the warning turned off, or `convert`).
- **Dropped attributes:** markers, extra events, trigger sounds and libraries.
- **SVG spritesheets** are offered for conversion to PNGs before saving.

Older editor files saved without a game copy (the former extended format) still open, and are written as above when saved again.

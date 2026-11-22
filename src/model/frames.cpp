#include "model/frames.h"

#include <algorithm>
#include <limits>

#include "model/draw.h"
#include "math_util.h"

void frame_mix(Frame& frame, const Frame& next, float amount)
{
    frame.rotation = vec_mix(frame.rotation, next.rotation, amount);
    frame.position = vec_mix(frame.position, next.position, amount);
    frame.scale = vec_mix(frame.scale, next.scale, amount);
    frame.skew = vec_mix(frame.skew, next.skew, amount);
    frame.colorOffset = vec_mix(frame.colorOffset, next.colorOffset, amount);
    frame.tint = vec_mix(frame.tint, next.tint, amount);
    frame.blur = vec_mix(frame.blur, next.blur, amount);
}

// Splits an interpolated frame into uninterpolated frames of at most `interval`. The first keeps the frame's uid.
std::vector<Frame> frame_bake_split(const Frame& original, const Frame& next, int interval, BakeOptions options)
{
    auto total = std::max(original.duration, FRAME_DURATION_MIN);
    interval = std::max(interval, FRAME_DURATION_MIN);
    std::vector<Frame> frames;
    for (int duration = 0; duration < total; duration += interval)
    {
        auto& baked = frames.emplace_back(duration == 0 ? original : frame_clone(original));
        frame_mix(baked, next, interpolation_factor(original.interpolation, (float)duration / total));
        baked.duration = std::min(interval, total - duration);
        baked.interpolation = Interpolation::NONE;
        if (options.isRoundScale)
            baked.scale = vec_round(baked.scale);
        if (options.isRoundRotation)
            baked.rotation = std::round(baked.rotation);
    }
    return frames;
}

Mat4 frame_parent_model_get(const Frame& frame)
{
    return quad_model_parent_get(frame.position, {}, percent_to_unit(frame.scale), frame.rotation,
                                 percent_to_unit(frame.skew));
}

void frame_root_transform_apply(Frame& frame, const Frame& rootFrame, BakeOptions options)
{
    auto rootScale = percent_to_unit(rootFrame.scale);
    auto pivot = options.isUseRootPivot ? rootFrame.position : Vec2();
    auto offset = (frame.position - pivot) * rootScale;
    auto angle = radians(rootFrame.rotation);
    auto cos = std::cos(angle);
    auto sin = std::sin(angle);

    frame.position = rootFrame.position + Vec2(offset.x * cos - offset.y * sin, offset.x * sin + offset.y * cos);
    frame.scale *= rootScale;
    frame.rotation += rootFrame.rotation;
    frame.tint *= rootFrame.tint;
    frame.colorOffset += rootFrame.colorOffset;

    if (options.isRoundScale)
        frame.scale = vec_round(frame.scale);
    if (options.isRoundRotation)
        frame.rotation = std::round(frame.rotation);
}

// For the game, a group root's transform is baked into its tracks one frame at a time.
void track_group_root_bake(Track& track, const Track& root, int frameNum)
{
    auto trackLength = track_length_get(track);
    if (trackLength <= 0)
        return;
    auto length = std::max({trackLength, track_length_get(root), frameNum, FRAME_DURATION_MIN});
    std::vector<Frame> frames;
    for (int time = 0; time < length; ++time)
    {
        auto& frame = frames.emplace_back(frame_generate(track, (float)time));
        frame_root_transform_apply(frame, frame_generate(root, (float)time));
        frame.duration = FRAME_DURATION_MIN;
        frame.interpolation = Interpolation::NONE;
    }
    track.frames = std::move(frames);
}

// The track's state at a time: the frame playing then, mixed toward the next one when interpolated. A trigger is
// returned only on its exact frame. Otherwise the result is invisible.
Frame frame_generate(const Track& track, float time)
{
    Frame frame = {};
    frame.isVisible = false;
    if (track.frames.empty())
        return frame;
    time = std::max(time, 0.0f);

    if (track.type == ItemType::TRIGGER)
    {
        for (auto& trigger : track.frames)
            if ((int)time == trigger.atFrame)
                return trigger;
        return frame;
    }

    const Frame* next = nullptr;
    int durationCurrent = 0;
    int durationNext = 0;
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
        frame = track.frames[i];
        durationNext += frame.duration;
        if (time >= durationCurrent && time < durationNext)
        {
            if (i + 1 < (int)track.frames.size())
                next = &track.frames[i + 1];
            break;
        }
        durationCurrent += frame.duration;
    }

    if (frame.interpolation != Interpolation::NONE && next && frame.duration > 1)
        frame_mix(
            frame, *next,
            interpolation_factor(frame.interpolation, (time - durationCurrent) / (durationNext - durationCurrent)));
    return frame;
}

int frame_index_from_at_frame_get(const Track& track, int atFrame)
{
    for (int i = 0; i < (int)track.frames.size(); ++i)
        if (track.frames[i].atFrame == atFrame)
            return i;
    return -1;
}

int frame_index_from_time_get(const Track& track, float time)
{
    if (track.type == ItemType::TRIGGER)
        return frame_index_from_at_frame_get(track, (int)time);
    if (track.frames.empty())
        return -1;
    if (time <= 0.0f)
        return 0;

    float duration = 0;
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
        duration += track.frames[i].duration;
        if (time < duration)
            return i;
    }
    return (int)track.frames.size() - 1;
}

float frame_time_from_index_get(const Track& track, int index)
{
    if (index < 0 || index >= (int)track.frames.size())
        return 0.0f;
    if (track.type == ItemType::TRIGGER)
        return (float)track.frames[index].atFrame;
    float time = 0;
    for (int i = 0; i < index; ++i)
        time += track.frames[i].duration;
    return time;
}

void frame_bake(Track& track, int index, int interval, BakeOptions options)
{
    if (index < 0 || index >= (int)track.frames.size())
        return;
    auto original = track.frames[index];
    if (original.duration <= FRAME_DURATION_MIN)
    {
        track.frames[index].interpolation = Interpolation::NONE;
        return;
    }

    auto next = index + 1 < (int)track.frames.size() ? track.frames[index + 1] : original;
    auto baked = frame_bake_split(original, next, interval, options);
    track.frames.erase(track.frames.begin() + index);
    track.frames.insert(track.frames.begin() + index, baked.begin(), baked.end());
}

void frames_generate_from_grid(Track& track, IVec2 startPosition, IVec2 size, Vec2 pivot, int columns, int count,
                               int duration)
{
    for (int i = 0; i < count; ++i)
    {
        auto& frame = track.frames.emplace_back();
        frame.duration = duration;
        frame.pivot = pivot;
        frame.size = size;
        frame.crop = startPosition + IVec2(size.x * (i % columns), size.y * (i / columns));
    }
}

void frames_sort_by_at_frame(Track& track)
{
    std::stable_sort(track.frames.begin(), track.frames.end(),
                     [](const Frame& a, const Frame& b) { return a.atFrame < b.atFrame; });
}

void frames_change(Track& track, FrameChange change, ItemType itemType, ChangeType changeType,
                   const std::set<int>& selection, bool isAnimationLayer)
{
    auto apply = [&](auto& target, const auto& optionalValue) {
        if (!optionalValue)
            return;
        auto value = *optionalValue;
        if (changeType == ChangeType::ADJUST)
            target = value;
        if (changeType == ChangeType::ADD)
            target += value;
        if (changeType == ChangeType::SUBTRACT)
            target -= value;
        if (changeType == ChangeType::MULTIPLY)
            target *= value;
        if (changeType == ChangeType::DIVIDE && value != decltype(value){})
            target /= value;
    };

    for (auto index : selection)
    {
        if (index < 0 || index >= (int)track.frames.size())
            continue;
        auto& frame = track.frames[index];
        if (change.isVisible)
            frame.isVisible = *change.isVisible;
        if (change.interpolation)
            frame.interpolation = *change.interpolation;
        if (change.isFlipX)
            frame.scale.x = -frame.scale.x;
        if (change.isFlipY)
            frame.scale.y = -frame.scale.y;

        apply(frame.duration, change.duration);
        if (change.duration)
            frame.duration = std::max(FRAME_DURATION_MIN, frame.duration);
        if (itemType == ItemType::LAYER && change.regionId)
            frame.regionId = *change.regionId;
        if (itemType == ItemType::LAYER && change.shaderId)
            frame.shaderId = *change.shaderId;
        if (itemType == ItemType::LAYER && change.depth)
        {
            apply(frame.depth, change.depth);
            frame.depth = std::clamp(frame.depth, -FRAME_DEPTH_MAX, FRAME_DEPTH_MAX);
        }
#define X(name, member, isLayerOnly, isColor, isEnabled, value)                                                        \
    if (!isLayerOnly || itemType == ItemType::LAYER)                                                                   \
        apply(frame.member, change.name);
        FRAME_CHANGE_SCALARS
#undef X
        frame.blur = std::clamp(frame.blur, 0.0f, FRAME_BLUR_MAX);
        if (!isAnimationLayer)
            continue;
        if (change.childAnimation)
            frame.childAnimation = *change.childAnimation;
        if (change.childMode)
            frame.childMode = *change.childMode;
        if (change.isChildStartRandom)
            frame.isChildStartRandom = *change.isChildStartRandom;
        if (change.isChildRestart)
            frame.isChildRestart = *change.isChildRestart;
        apply(frame.childStart, change.childStart);
        frame.childStart = std::clamp(frame.childStart, 0, FRAME_NUM_MAX);
    }
}

// Bounds of all visible layer frames over the animation's length, or -1 when nothing is drawn.
Vec4 animation_rect(const Model& model, const Animation& animation, bool isRootTransform, const LibraryGet& library_get)
{
    constexpr IVec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    Vec2 min{std::numeric_limits<float>::infinity()};
    Vec2 max{-std::numeric_limits<float>::infinity()};
    bool isAny = false;

    for (float time = 0.0f; time < (float)animation.frameNum; time += 1.0f)
        for (auto& draw : animation_draws_get(
                 model, animation, {.time = time, .isRootTransform = isRootTransform, .library_get = library_get}))
        {
            if (draw.type != DrawType::LAYER || draw.frame.size == Vec2())
                continue;
            auto transform = draw.parent * draw_quad_model_get(draw);
            for (auto& corner : CORNERS)
            {
                auto world = Vec2(transform * Vec4(corner, 0.0f, 1.0f));
                min = vec_min(min, world);
                max = vec_max(max, world);
                isAny = true;
            }
        }

    if (!isAny)
        return Vec4(-1.0f);
    return {min.x, min.y, max.x - min.x, max.y - min.y};
}

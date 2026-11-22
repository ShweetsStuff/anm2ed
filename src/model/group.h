#pragma once

#include <algorithm>
#include <set>
#include <type_traits>
#include <variant>
#include <vector>

// Lists of items and groups of items, as an animation keeps its layer and null tracks and a document its animations.
// An entry is an item or a group, and a group has an `id` and its `items`.

template <class Entries> using EntryItem = std::variant_alternative_t<0, typename Entries::value_type>;
template <class Entries> using EntryGroup = std::variant_alternative_t<1, typename Entries::value_type>;
template <class Entries>
using EntryGroupOf = std::conditional_t<std::is_const_v<Entries>, const EntryGroup<std::remove_const_t<Entries>>,
                                        EntryGroup<std::remove_const_t<Entries>>>;
template <class Entries>
using EntryItemOf = std::conditional_t<std::is_const_v<Entries>, const EntryItem<std::remove_const_t<Entries>>,
                                       EntryItem<std::remove_const_t<Entries>>>;

// Calls on_group(group) for each group, before on_item(item, group) for its items. Ungrouped items get nullptr.
template <class Entries, class OnItem, class OnGroup>
void entries_walk(Entries& entries, OnItem&& on_item, OnGroup&& on_group)
{
    for (auto& entry : entries)
        if (auto item = std::get_if<0>(&entry))
            on_item(*item, (EntryGroupOf<Entries>*)nullptr);
        else
        {
            auto& group = std::get<1>(entry);
            on_group(group);
            for (auto& groupItem : group.items)
                on_item(groupItem, &group);
        }
}

template <class Entries, class OnItem> void items_each(Entries& entries, OnItem&& on_item)
{
    entries_walk(entries, on_item, [](auto&) {});
}

template <class Entries, class OnGroup> void groups_each(Entries& entries, OnGroup&& on_group)
{
    entries_walk(entries, [](auto&, auto*) {}, on_group);
}

template <class Entries> EntryGroupOf<Entries>* group_find(Entries& entries, int id)
{
    for (auto& entry : entries)
        if (auto group = std::get_if<1>(&entry); group && group->id == id)
            return group;
    return nullptr;
}

// Where a group sits among the entries, or -1.
template <class Entries> int group_entry_get(const Entries& entries, int id)
{
    for (int i = 0; i < (int)entries.size(); ++i)
        if (auto group = std::get_if<1>(&entries[i]); group && group->id == id)
            return i;
    return -1;
}

template <class Entries> int group_next_id(const Entries& entries)
{
    int id = 0;
    groups_each(entries, [&](auto& group) { id = std::max(id, group.id + 1); });
    return id;
}

template <class Entries> int items_count(const Entries& entries)
{
    int count = 0;
    items_each(entries, [&](auto&, auto*) { ++count; });
    return count;
}

// The item at a flat index (grouped items count in order), and the group holding it.
template <class Entries> struct ItemPlace
{
    EntryItemOf<Entries>* item = nullptr;
    EntryGroupOf<Entries>* group = nullptr;
};

template <class Entries> ItemPlace<Entries> item_place_get(Entries& entries, int index)
{
    ItemPlace<Entries> place;
    if (index < 0)
        return place;
    items_each(entries, [&](auto& item, auto* group) {
        if (index-- == 0)
            place = {&item, group};
    });
    return place;
}

// Removes the items isRemoved picks, grouped ones too.
template <class Entries, class IsRemoved> void entries_remove(Entries& entries, IsRemoved&& isRemoved)
{
    std::erase_if(entries, [&](auto& entry) {
        if (auto item = std::get_if<0>(&entry))
            return (bool)isRemoved(*item);
        std::erase_if(std::get<1>(entry).items, isRemoved);
        return false;
    });
}

// Puts each item's `expand(item, group)` in its place (any number of items), keeping groups.
template <class Entries, class Expand> Entries entries_expand(Entries entries, Expand&& expand)
{
    Entries result;
    for (auto& entry : entries)
        if (auto item = std::get_if<0>(&entry))
            for (auto& expanded : expand(*item, (EntryGroup<Entries>*)nullptr))
                result.emplace_back(std::move(expanded));
        else
        {
            auto group = std::move(std::get<1>(entry));
            decltype(group.items) items;
            for (auto& groupItem : group.items)
                for (auto& expanded : expand(groupItem, &group))
                    items.push_back(std::move(expanded));
            group.items = std::move(items);
            result.emplace_back(std::move(group));
        }
    return result;
}

// Items and whole groups taken out of the entries, in the order they had.
template <class Entries> struct EntriesTaken
{
    std::vector<EntryItem<Entries>> items;
    std::vector<EntryGroup<Entries>> groups;
};

// Takes out the items isTaken picks (grouped ones too) and the groups in groupIds.
template <class Entries, class IsTaken>
EntriesTaken<Entries> entries_take(Entries& entries, IsTaken&& isTaken, const std::set<int>& groupIds)
{
    EntriesTaken<Entries> taken;
    std::erase_if(entries, [&](auto& entry) {
        if (auto item = std::get_if<0>(&entry))
        {
            if (!isTaken(*item))
                return false;
            taken.items.push_back(std::move(*item));
            return true;
        }
        auto& group = std::get<1>(entry);
        if (groupIds.contains(group.id))
        {
            taken.groups.push_back(std::move(group));
            return true;
        }
        std::erase_if(group.items, [&](auto& groupItem) {
            if (!isTaken(groupItem))
                return false;
            taken.items.push_back(std::move(groupItem));
            return true;
        });
        return false;
    });
    return taken;
}

// Inserts items and groups as entries at `index`.
template <class Entries> void entries_insert(Entries& entries, int index, EntriesTaken<Entries> taken)
{
    Entries inserted;
    for (auto& item : taken.items)
        inserted.emplace_back(std::move(item));
    for (auto& group : taken.groups)
        inserted.emplace_back(std::move(group));
    entries.insert(entries.begin() + std::clamp(index, 0, (int)entries.size()),
                   std::make_move_iterator(inserted.begin()), std::make_move_iterator(inserted.end()));
}

// Moves the ungrouped items isWrapped picks into `group`, placed where the first of them was. False when none.
template <class Entries, class IsWrapped>
bool entries_wrap(Entries& entries, IsWrapped&& isWrapped, EntryGroup<Entries> group)
{
    auto first = std::ranges::find_if(entries, [&](auto& entry) {
        auto item = std::get_if<0>(&entry);
        return item && isWrapped(*item);
    });
    if (first == entries.end())
        return false;
    auto index = (int)(first - entries.begin());
    std::erase_if(entries, [&](auto& entry) {
        auto item = std::get_if<0>(&entry);
        if (!item || !isWrapped(*item))
            return false;
        group.items.push_back(std::move(*item));
        return true;
    });
    entries.insert(entries.begin() + index, std::move(group));
    return true;
}

// Dissolves the groups in groupIds, their items taking their place.
template <class Entries> void groups_unwrap(Entries& entries, const std::set<int>& groupIds)
{
    Entries result;
    for (auto& entry : entries)
    {
        auto group = std::get_if<1>(&entry);
        if (!group || !groupIds.contains(group->id))
            result.push_back(std::move(entry));
        else
            for (auto& item : group->items)
                result.emplace_back(std::move(item));
    }
    entries = std::move(result);
}

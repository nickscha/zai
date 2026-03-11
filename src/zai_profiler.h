#ifndef ZAI_PROFILER_H
#define ZAI_PROFILER_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Performance Profiler
 * #############################################################################
 */
#define ZAI_PROFILER_MAX_ENTRIES 512
#define ZAI_PROFILER_ENTRY_INVALID 0xFFFFFFFF

typedef struct zai_profiler_entry
{
    f64 time_ms_last;
    f64 time_ms_min;
    f64 time_ms_max;
    f64 time_ms_total;
    u32 line;
    u32 counter;
    s8 *name;
    s8 *file;

} zai_profiler_entry;

static zai_profiler_entry zai_profiler_entries[ZAI_PROFILER_MAX_ENTRIES];
static u32 zai_profiler_entries_count = 0;

ZAI_API f64 zai_profiler_time_ms(void);

ZAI_API ZAI_INLINE u32 zai_profiler_string_equals(s8 *a, s8 *b)
{
    while (*a && *b)
    {
        if (*a != *b)
        {
            return 0;
        }

        a++;
        b++;
    }

    return (*a == *b);
}

ZAI_API ZAI_INLINE u32 zai_profiler_find_entry(s8 *name)
{
    u32 i;

    for (i = 0; i < zai_profiler_entries_count; ++i)
    {
        if (zai_profiler_string_equals(zai_profiler_entries[i].name, name))
        {
            return i;
        }
    }

    return ZAI_PROFILER_ENTRY_INVALID;
}

ZAI_API ZAI_INLINE void zai_profiler_begin(s8 *name, s8 *file, u32 line)
{
    u32 entry_id = zai_profiler_find_entry(name);

    if (entry_id == ZAI_PROFILER_ENTRY_INVALID)
    {
        zai_profiler_entry entry = {0};
        entry.name = name;
        entry.file = file;
        entry.line = line;
        entry.counter += 1;
        entry.time_ms_last = zai_profiler_time_ms();

        if (zai_profiler_entries_count >= ZAI_PROFILER_MAX_ENTRIES)
        {
            return;
        }

        zai_profiler_entries[zai_profiler_entries_count++] = entry;
    }
    else
    {
        zai_profiler_entries[entry_id].counter += 1;
        zai_profiler_entries[entry_id].time_ms_last = zai_profiler_time_ms();
    }
}

ZAI_API ZAI_INLINE void zai_profiler_end(s8 *name)
{
    f64 time_ms_end = zai_profiler_time_ms();

    u32 entry_id = zai_profiler_find_entry(name);

    if (entry_id != ZAI_PROFILER_ENTRY_INVALID)
    {
        zai_profiler_entry *entry = &zai_profiler_entries[entry_id];

        f64 time_ms_last = time_ms_end - entry->time_ms_last;

        if (entry->counter == 1)
        {
            entry->time_ms_min = time_ms_last;
            entry->time_ms_max = time_ms_last;
        }

        if (time_ms_last < entry->time_ms_min)
        {
            entry->time_ms_min = time_ms_last;
        }

        if (time_ms_last > entry->time_ms_max)
        {
            entry->time_ms_max = time_ms_last;
        }

        entry->time_ms_last = time_ms_last;
        entry->time_ms_total += time_ms_last;
    }
}

#define ZAI_PROFILER_BEGIN(name) zai_profiler_begin(#name, __FILE__, __LINE__)
#define ZAI_PROFILER_END(name) zai_profiler_end(#name)

#endif /* ZAI_PROFILER_H */
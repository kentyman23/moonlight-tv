#include "hints.h"
#include "util/i18n.h"

#include <SDL.h>

#define HINTS_LEN 4
static const char *hints_list[HINTS_LEN] = {
        "To open overlay while streaming, hold BACK (or press EXIT for traditional remote).",
        "Frequent lagging on 5 GHz Wi-Fi? Try changing to another channel. See Help for details.",
        "Xbox One/Series controller can't be directly connected via cable or official adapter.",
        "If you have an ultra-wide monitor, you may need to change its resolution to fit 16:9 for streaming.",
};

const char *hints_obtain() {
    static int hint_idx = -1;
    if (hint_idx < 0) {
        hint_idx = (int) SDL_GetTicks() % HINTS_LEN;
    } else if (++hint_idx >= HINTS_LEN) {
        hint_idx = 0;
    }

    return locstr(hints_list[hint_idx]);
}

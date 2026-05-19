#include "GlobalEvents.h"

static GlobalEvents *s_gevents = nullptr;

GlobalEvents *GlobalEvents::instance()
{
    if (!s_gevents) s_gevents = new GlobalEvents;
    return s_gevents;
}

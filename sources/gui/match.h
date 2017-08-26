#ifndef __MATCH_H__
#define __MATCH_H__

#include <glib-object.h>

#define MATCH_TYPE (match_get_type())
G_DECLARE_FINAL_TYPE(Match, match, PICKUP_GUI, MATCH,
    GObject)

Match *match_new(void);

#endif//__MATCH_H__

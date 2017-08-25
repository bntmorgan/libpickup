#ifndef __MATCH_LIST_H__
#define __MATCH_LIST_H__

#include <glib-object.h>

#define MATCH_LIST_TYPE (match_list_get_type())
G_DECLARE_FINAL_TYPE(MatchList, match_list, PICKUP_GUI, MATCH_LIST,
    GObject)

MatchList *match_list_new(void);

#endif//__MATCH_LIST_H__

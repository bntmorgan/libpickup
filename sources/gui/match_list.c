#include <string.h>

#include <pickup/pickup.h>

#include "match_list.h"

enum {
  MATCH_LIST_PROP_MID = 1,
  MATCH_LIST_PROP_PID,
  MATCH_LIST_PROP_NAME,
  MATCH_LIST_PROP_DATE,
  MATCH_LIST_PROP_BIRTH,
  MATCH_LIST_LAST_PROPERTY
};

struct _MatchList {
  GObject parent;
  struct pickup_match m;
};

static GParamSpec *match_list_properties[MATCH_LIST_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(MatchList, match_list, G_TYPE_OBJECT)

static void match_list_init(MatchList *obj) { }

static void match_list_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  struct pickup_match *m = &((MatchList *)object)->m;

  switch (property_id) {
    case MATCH_LIST_PROP_MID:
      g_value_set_string(value, m->mid);
      break;
    case MATCH_LIST_PROP_PID:
      g_value_set_string(value, m->pid);
      break;
    case MATCH_LIST_PROP_NAME:
      g_value_set_string(value, m->name);
      break;
    case MATCH_LIST_PROP_DATE:
      g_value_set_int(value, m->date);
      break;
    case MATCH_LIST_PROP_BIRTH:
      g_value_set_int(value, m->birth);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void match_list_set_property(GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  struct pickup_match *m = &((MatchList *)object)->m;

  switch (property_id) {
    case MATCH_LIST_PROP_MID:
      strcpy(&m->mid[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_PID:
      strcpy(&m->pid[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_NAME:
      strcpy(&m->name[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_DATE:
      m->date = g_value_get_int(value);
      break;
    case MATCH_LIST_PROP_BIRTH:
      m->birth = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void match_list_finalize(GObject *obj) {
  G_OBJECT_CLASS(match_list_parent_class)->finalize(obj);
}

static void match_list_class_init(MatchListClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = match_list_get_property;
  object_class->set_property = match_list_set_property;
  object_class->finalize = match_list_finalize;

  match_list_properties[MATCH_LIST_PROP_MID] = g_param_spec_string("mid", "mid",
      "mid", NULL, G_PARAM_READWRITE);
  match_list_properties[MATCH_LIST_PROP_PID] = g_param_spec_string("pid", "pid",
      "pid", NULL, G_PARAM_READWRITE);
  match_list_properties[MATCH_LIST_PROP_NAME] = g_param_spec_string("name",
      "name", "name", NULL, G_PARAM_READWRITE);
  match_list_properties[MATCH_LIST_PROP_DATE] = g_param_spec_int("date", "date",
      "date", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  match_list_properties[MATCH_LIST_PROP_BIRTH] = g_param_spec_int("birth",
      "birth", "birth", 0, G_MAXINT, 0, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, MATCH_LIST_LAST_PROPERTY,
      match_list_properties);
}

#include <string.h>

#include <pickup/pickup.h>

#include "match.h"
#include "log.h"

enum {
  MATCH_PROP_MID = 1,
  MATCH_PROP_PID,
  MATCH_PROP_NAME,
  MATCH_PROP_DATE,
  MATCH_PROP_BIRTH,
  MATCH_PROP_IMAGES_COUNT,
  MATCH_PROP_IMAGES,
  MATCH_PROP_IMAGE,
  MATCH_PROP_IMAGE_INDEX,
  MATCH_PROP_IMAGE_PROGRESS,
  MATCH_PROP_MATCH,
  MATCH_LAST_PROPERTY
};

struct _Match {
  GObject parent;
  struct pickup_match m;
  char image[0x1000]; // XXX Ext 4 path max
  unsigned int image_index;
  int match;
  float image_progress;
};

static GParamSpec *match_properties[MATCH_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(Match, match, G_TYPE_OBJECT)

static void match_init(Match *obj) { }

static void match_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  Match *mo = (Match *)object;
  struct pickup_match *m = &((Match *)object)->m;

  switch (property_id) {
    case MATCH_PROP_MID:
      g_value_set_string(value, m->mid);
      break;
    case MATCH_PROP_PID:
      g_value_set_string(value, m->pid);
      break;
    case MATCH_PROP_NAME:
      g_value_set_string(value, m->name);
      break;
    case MATCH_PROP_DATE:
      g_value_set_int(value, m->date);
      break;
    case MATCH_PROP_BIRTH:
      g_value_set_int(value, m->birth);
      break;
    case MATCH_PROP_IMAGES_COUNT:
      g_value_set_int(value, m->images_count);
      break;
    case MATCH_PROP_IMAGES:
      g_value_set_pointer(value, m->images);
      break;
    case MATCH_PROP_IMAGE:
      g_value_set_string(value, mo->image);
      break;
    case MATCH_PROP_IMAGE_INDEX:
      g_value_set_int(value, mo->image_index);
      break;
    case MATCH_PROP_IMAGE_PROGRESS:
      g_value_set_float(value, mo->image_progress);
      break;
    case MATCH_PROP_MATCH:
      g_value_set_boolean(value, mo->match);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void match_set_property(GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  Match *mo = (Match *)object;
  struct pickup_match *m = &((Match *)object)->m;

  switch (property_id) {
    case MATCH_PROP_MID:
      strcpy(&m->mid[0], g_value_get_string(value));
      break;
    case MATCH_PROP_PID:
      strcpy(&m->pid[0], g_value_get_string(value));
      break;
    case MATCH_PROP_NAME:
      strcpy(&m->name[0], g_value_get_string(value));
      break;
    case MATCH_PROP_DATE:
      m->date = g_value_get_int(value);
      break;
    case MATCH_PROP_BIRTH:
      m->birth = g_value_get_int(value);
      break;
    case MATCH_PROP_IMAGES_COUNT:
      m->images_count = g_value_get_int(value);
      break;
    case MATCH_PROP_IMAGES:
      m->images = g_value_get_pointer(value);
      break;
    case MATCH_PROP_IMAGE:
      strcpy(&mo->image[0], g_value_get_string(value));
      break;
    case MATCH_PROP_IMAGE_INDEX:
      mo->image_index = g_value_get_int(value);
      break;
    case MATCH_PROP_IMAGE_PROGRESS:
      mo->image_progress = g_value_get_float(value);
      break;
    case MATCH_PROP_MATCH:
      mo->match = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void match_finalize(GObject *obj) {
  G_OBJECT_CLASS(match_parent_class)->finalize(obj);
}

static void match_class_init(MatchClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = match_get_property;
  object_class->set_property = match_set_property;
  object_class->finalize = match_finalize;

  match_properties[MATCH_PROP_MID] = g_param_spec_string("mid", "mid",
      "mid", NULL, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_PID] = g_param_spec_string("pid", "pid",
      "pid", NULL, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_NAME] = g_param_spec_string("name",
      "name", "name", NULL, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_DATE] = g_param_spec_int("date", "date",
      "date", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_BIRTH] = g_param_spec_int("birth",
      "birth", "birth", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_IMAGES_COUNT] = g_param_spec_int("images-count",
      "images-count", "images-count", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_IMAGES] = g_param_spec_pointer("images",
      "images", "images", G_PARAM_READWRITE);
  match_properties[MATCH_PROP_IMAGE] = g_param_spec_string("image",
      "image", "image", NULL, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_IMAGE_INDEX] = g_param_spec_int("image-index",
      "image-index", "image-index", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  match_properties[MATCH_PROP_IMAGE_PROGRESS] =
    g_param_spec_float("image-progress", "image-progress", "image-progress", 0.,
        1., 0., G_PARAM_READWRITE);
  match_properties[MATCH_PROP_MATCH] = g_param_spec_boolean("match",
      "match", "match", FALSE, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, MATCH_LAST_PROPERTY,
      match_properties);
}

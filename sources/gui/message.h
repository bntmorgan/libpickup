#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <glib-object.h>

#define MESSAGE_TYPE (message_get_type())
G_DECLARE_FINAL_TYPE(Message, message, PICKUP_GUI, MESSAGE, GObject)

Message *message_new(void);

#endif//__MESSAGE_H__

/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *
 * gstobject.h: Header for base GstObject
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_OBJECT_H__
#define __GST_OBJECT_H__

#include <gst/gstconfig.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define GST_TYPE_OBJECT			(gst_object_get_type ())
#define GST_IS_OBJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_OBJECT))
#define GST_IS_OBJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_OBJECT))
#define GST_OBJECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_OBJECT, GstObjectClass))
#define GST_OBJECT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_OBJECT, GstObject))
#define GST_OBJECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_OBJECT, GstObjectClass))
#define GST_OBJECT_CAST(obj)            ((GstObject*)(obj))
#define GST_OBJECT_CLASS_CAST(klass)    ((GstObjectClass*)(klass))

/**
 * GstObjectFlags:
 * @GST_OBJECT_FLAG_LAST: subclasses can add additional flags starting from this flag
 *
 * The standard flags that an gstobject may have.
 */
typedef enum
{
  /* padding */
  GST_OBJECT_FLAG_LAST = (1<<4)
} GstObjectFlags;

/**
 * GST_OBJECT_REFCOUNT:
 * @obj: a #GstObject
 *
 * Get access to the reference count field of the object.
 */
#define GST_OBJECT_REFCOUNT(obj)                (((GObject*)(obj))->ref_count)
/**
 * GST_OBJECT_REFCOUNT_VALUE:
 * @obj: a #GstObject
 *
 * Get the reference count value of the object.
 */
#define GST_OBJECT_REFCOUNT_VALUE(obj)          g_atomic_int_get ((gint *) &GST_OBJECT_REFCOUNT(obj))

/* we do a GST_OBJECT_CAST to avoid type checking, better call these
 * function with a valid object! */

/**
 * GST_OBJECT_GET_LOCK:
 * @obj: a #GstObject
 *
 * Acquire a reference to the mutex of this object.
 */
#define GST_OBJECT_GET_LOCK(obj)               (GST_OBJECT_CAST(obj)->lock)
/**
 * GST_OBJECT_LOCK:
 * @obj: a #GstObject to lock
 *
 * This macro will obtain a lock on the object, making serialization possible.
 * It blocks until the lock can be obtained.
 */
#define GST_OBJECT_LOCK(obj)                   g_mutex_lock(GST_OBJECT_GET_LOCK(obj))
/**
 * GST_OBJECT_TRYLOCK:
 * @obj: a #GstObject.
 *
 * This macro will try to obtain a lock on the object, but will return with
 * FALSE if it can't get it immediately.
 */
#define GST_OBJECT_TRYLOCK(obj)                g_mutex_trylock(GST_OBJECT_GET_LOCK(obj))
/**
 * GST_OBJECT_UNLOCK:
 * @obj: a #GstObject to unlock.
 *
 * This macro releases a lock on the object.
 */
#define GST_OBJECT_UNLOCK(obj)                 g_mutex_unlock(GST_OBJECT_GET_LOCK(obj))


/**
 * GST_OBJECT_NAME:
 * @obj: a #GstObject
 *
 * Get the name of this object
 */
#define GST_OBJECT_NAME(obj)            (GST_OBJECT_CAST(obj)->name)
/**
 * GST_OBJECT_PARENT:
 * @obj: a #GstObject
 *
 * Get the parent of this object
 */
#define GST_OBJECT_PARENT(obj)          (GST_OBJECT_CAST(obj)->parent)


/**
 * GST_OBJECT_FLAGS:
 * @obj: a #GstObject
 *
 * This macro returns the entire set of flags for the object.
 */
#define GST_OBJECT_FLAGS(obj)                  (GST_OBJECT_CAST (obj)->flags)
/**
 * GST_OBJECT_FLAG_IS_SET:
 * @obj: a #GstObject
 * @flag: Flag to check for
 *
 * This macro checks to see if the given flag is set.
 */
#define GST_OBJECT_FLAG_IS_SET(obj,flag)       ((GST_OBJECT_FLAGS (obj) & (flag)) == (flag))
/**
 * GST_OBJECT_FLAG_SET:
 * @obj: a #GstObject
 * @flag: Flag to set
 *
 * This macro sets the given bits.
 */
#define GST_OBJECT_FLAG_SET(obj,flag)          (GST_OBJECT_FLAGS (obj) |= (flag))
/**
 * GST_OBJECT_FLAG_UNSET:
 * @obj: a #GstObject
 * @flag: Flag to set
 *
 * This macro usets the given bits.
 */
#define GST_OBJECT_FLAG_UNSET(obj,flag)        (GST_OBJECT_FLAGS (obj) &= ~(flag))


typedef struct _GstObject GstObject;
typedef struct _GstObjectClass GstObjectClass;

/**
 * GstObject:
 * @lock: object LOCK
 * @name: The name of the object
 * @parent: this object's parent, weak ref
 * @flags: flags for this object
 *
 * GStreamer base object class.
 */
struct _GstObject {
  GInitiallyUnowned object;

  /*< public >*/ /* with LOCK */
  GMutex        *lock;        /* object LOCK */
  gchar         *name;        /* object name */
  GstObject     *parent;      /* this object's parent, weak ref */
  guint32        flags;

  /*< private >*/
  gpointer _gst_reserved;
};

/**
 * GstObjectClass:
 * @parent_class: parent
 * @path_string_separator: separator used by gst_object_get_path_string()
 * @deep_notify: default signal handler
 *
 * GStreamer base object class.
 */
struct _GstObjectClass {
  GInitiallyUnownedClass parent_class;

  const gchar	*path_string_separator;

  /* signals */
  void          (*deep_notify)      (GstObject * object, GstObject * orig, GParamSpec * pspec);

  /*< public >*/
  /* virtual methods for subclasses */

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

/* normal GObject stuff */
GType		gst_object_get_type		(void);

/* name routines */
gboolean	gst_object_set_name		(GstObject *object, const gchar *name);
gchar*		gst_object_get_name		(GstObject *object);

/* parentage routines */
gboolean	gst_object_set_parent		(GstObject *object, GstObject *parent);
GstObject*	gst_object_get_parent		(GstObject *object);
void		gst_object_unparent		(GstObject *object);
gboolean	gst_object_has_ancestor		(GstObject *object, GstObject *ancestor);

void            gst_object_default_deep_notify  (GObject *object, GstObject *orig,
                                                 GParamSpec *pspec, gchar **excluded_props);

/* refcounting + life cycle */
gpointer	gst_object_ref			(gpointer object);
void		gst_object_unref		(gpointer object);
gpointer        gst_object_ref_sink		(gpointer object);

/* replace object pointer */
gboolean        gst_object_replace		(GstObject **oldobj, GstObject *newobj);

/* printing out the 'path' of the object */
gchar *		gst_object_get_path_string	(GstObject *object);

/* misc utils */
gboolean	gst_object_check_uniqueness	(GList *list, const gchar *name);

G_END_DECLS

#endif /* __GST_OBJECT_H__ */


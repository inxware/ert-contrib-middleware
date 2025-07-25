/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstplugin.h: Header for plugin subsystem
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


#ifndef __GST_PLUGIN_H__
#define __GST_PLUGIN_H__

#include <gst/gstconfig.h>

#include <time.h> /* time_t */
#include <sys/types.h> /* off_t */
#include <sys/stat.h> /* off_t */
#include <gmodule.h>
#include <gst/gstobject.h>
#include <gst/gstmacros.h>
#include <gst/gststructure.h>

G_BEGIN_DECLS

typedef struct _GstPlugin GstPlugin;
typedef struct _GstPluginClass GstPluginClass;
typedef struct _GstPluginPrivate GstPluginPrivate;
typedef struct _GstPluginDesc GstPluginDesc;

/**
 * gst_plugin_error_quark:
 *
 * Get the error quark.
 *
 * Returns: The error quark used in GError messages
 */
GQuark gst_plugin_error_quark (void);
/**
 * GST_PLUGIN_ERROR:
 *
 * The error message category quark
 */
#define GST_PLUGIN_ERROR gst_plugin_error_quark ()

/**
 * GstPluginError:
 * @GST_PLUGIN_ERROR_MODULE: The plugin could not be loaded
 * @GST_PLUGIN_ERROR_DEPENDENCIES: The plugin has unresolved dependencies
 * @GST_PLUGIN_ERROR_NAME_MISMATCH: The plugin has already be loaded from a different file
 *
 * The plugin loading errors
 */
typedef enum
{
  GST_PLUGIN_ERROR_MODULE,
  GST_PLUGIN_ERROR_DEPENDENCIES,
  GST_PLUGIN_ERROR_NAME_MISMATCH
} GstPluginError;

/**
 * GstPluginFlags:
 * @GST_PLUGIN_FLAG_CACHED: Temporarily loaded plugins
 * @GST_PLUGIN_FLAG_BLACKLISTED: The plugin won't be scanned (again)
 *
 * The plugin loading state
 */
typedef enum
{
  GST_PLUGIN_FLAG_CACHED = (1<<0),
  GST_PLUGIN_FLAG_BLACKLISTED = (1<<1)
} GstPluginFlags;

/**
 * GstPluginDependencyFlags:
 * @GST_PLUGIN_DEPENDENCY_FLAG_NONE : no special flags
 * @GST_PLUGIN_DEPENDENCY_FLAG_RECURSE : recurse into subdirectories
 * @GST_PLUGIN_DEPENDENCY_FLAG_PATHS_ARE_DEFAULT_ONLY : use paths
 *         argument only if none of the environment variables is set
 * @GST_PLUGIN_DEPENDENCY_FLAG_FILE_NAME_IS_SUFFIX : interpret
 *         filename argument as filter suffix and check all matching files in
 *         the directory
 *
 * Flags used in connection with gst_plugin_add_dependency().
 *
 * Since: 0.10.22
 */
typedef enum {
  GST_PLUGIN_DEPENDENCY_FLAG_NONE = 0,
  GST_PLUGIN_DEPENDENCY_FLAG_RECURSE = (1 << 0),
  GST_PLUGIN_DEPENDENCY_FLAG_PATHS_ARE_DEFAULT_ONLY = (1 << 1),
  GST_PLUGIN_DEPENDENCY_FLAG_FILE_NAME_IS_SUFFIX = (1 << 2)
} GstPluginDependencyFlags;

/**
 * GstPluginInitFunc:
 * @plugin: The plugin object
 *
 * A plugin should provide a pointer to a function of this type in the
 * plugin_desc struct.
 * This function will be called by the loader at startup. One would then
 * register each #GstPluginFeature.
 *
 * Returns: %TRUE if plugin initialised successfully
 */
/* FIXME 0.11: Make return void */
typedef gboolean (*GstPluginInitFunc) (GstPlugin *plugin);

/**
 * GstPluginInitFullFunc:
 * @plugin: The plugin object
 * @user_data: extra data
 *
 * A plugin should provide a pointer to a function of either #GstPluginInitFunc
 * or this type in the plugin_desc struct.
 * The function will be called by the loader at startup. One would then
 * register each #GstPluginFeature. This version allows
 * user data to be passed to init function (useful for bindings).
 *
 * Returns: %TRUE if plugin initialised successfully
 *
 * Since: 0.10.24
 *
 */
/* FIXME 0.11: Merge with GstPluginInitFunc */
typedef gboolean (*GstPluginInitFullFunc) (GstPlugin *plugin, gpointer user_data);

/**
 * GstPluginDesc:
 * @major_version: the major version number of core that plugin was compiled for
 * @minor_version: the minor version number of core that plugin was compiled for
 * @name: a unique name of the plugin
 * @description: description of plugin
 * @plugin_init: pointer to the init function of this plugin.
 * @version: version of the plugin
 * @license: effective license of plugin
 * @source: source module plugin belongs to
 * @package: shipped package plugin belongs to
 * @origin: URL to provider of plugin
 * @release_datetime: date time string in ISO 8601 format (or rather, a
 *     subset thereof), or NULL. Allowed are the following formats:
 *     "YYYY-MM-DD" and "YYY-MM-DDTHH:MMZ" (with 'T' a separator and 'Z'
 *     indicating UTC/Zulu time). This field should be set via the
 *     GST_PACKAGE_RELEASE_DATETIME preprocessor macro (Since: 0.10.31)
 *
 * A plugin should export a variable of this type called plugin_desc. The plugin
 * loader will use the data provided there to initialize the plugin.
 *
 * The @licence parameter must be one of: LGPL, GPL, QPL, GPL/QPL, MPL,
 * BSD, MIT/X11, Proprietary, unknown.
 */
struct _GstPluginDesc {
  gint major_version;
  gint minor_version;
  const gchar *name;
  const gchar *description;
  GstPluginInitFunc plugin_init;
  const gchar *version;
  const gchar *license;
  const gchar *source;
  const gchar *package;
  const gchar *origin;
  const gchar *release_datetime;
  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};


#define GST_TYPE_PLUGIN   (gst_plugin_get_type())
#define GST_IS_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_PLUGIN))
#define GST_IS_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_PLUGIN))
#define GST_PLUGIN_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_PLUGIN, GstPluginClass))
#define GST_PLUGIN(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_PLUGIN, GstPlugin))
#define GST_PLUGIN_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_PLUGIN, GstPluginClass))
#define GST_PLUGIN_CAST(obj)           ((GstPlugin*)(obj))

/**
 * GstPlugin:
 *
 * The plugin object
 */
struct _GstPlugin {
  GstObject       object;

  /*< private >*/
  GstPluginDesc	desc;

  GstPluginDesc *orig_desc;

  unsigned int  flags;

  gchar *	filename;
  gchar *	basename;       /* base name (non-dir part) of plugin path */

  GModule *	module;		/* contains the module if plugin is loaded */

  off_t         file_size;
  time_t        file_mtime;
  gboolean      registered;     /* TRUE when the registry has seen a filename
                                 * that matches the plugin's basename */

  GstPluginPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstPluginClass {
  GstObjectClass  object_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

#ifdef GST_PACKAGE_RELEASE_DATETIME
#define __GST_PACKAGE_RELEASE_DATETIME GST_PACKAGE_RELEASE_DATETIME
#else
#define __GST_PACKAGE_RELEASE_DATETIME NULL
#endif

/**
 * GST_PLUGIN_DEFINE:
 * @major: major version number of the gstreamer-core that plugin was compiled for
 * @minor: minor version number of the gstreamer-core that plugin was compiled for
 * @name: short, but unique name of the plugin
 * @description: information about the purpose of the plugin
 * @init: function pointer to the plugin_init method with the signature of <code>static gboolean plugin_init (GstPlugin * plugin)</code>.
 * @version: full version string (e.g. VERSION from config.h)
 * @license: under which licence the package has been released, e.g. GPL, LGPL.
 * @package: the package-name (e.g. PACKAGE_NAME from config.h)
 * @origin: a description from where the package comes from (e.g. the homepage URL)
 *
 * This macro needs to be used to define the entry point and meta data of a
 * plugin. One would use this macro to export a plugin, so that it can be used
 * by other applications.
 *
 * The macro uses a define named PACKAGE for the #GstPluginDesc,source field.
 * When using autoconf, this is usually set automatically via the AC_INIT
 * macro, and set in config.h. If you are not using autoconf, you will need to
 * define PACKAGE yourself and set it to a short mnemonic string identifying
 * your application/package, e.g. 'someapp' or 'my-plugins-foo.
 *
 * If defined, the GST_PACKAGE_RELEASE_DATETIME will also be used for the
 * #GstPluginDesc,release_datetime field.
 */
#define GST_PLUGIN_DEFINE(major,minor,name,description,init,version,license,package,origin)	\
G_BEGIN_DECLS \
GST_PLUGIN_EXPORT GstPluginDesc gst_plugin_desc = {	\
  major,						\
  minor,						\
  name,							\
  (gchar *) description,				\
  init,							\
  version,						\
  license,						\
  PACKAGE,						\
  package,						\
  origin,						\
  __GST_PACKAGE_RELEASE_DATETIME,                       \
  GST_PADDING_INIT				        \
}; \
G_END_DECLS

/**
 * GST_LICENSE_UNKNOWN:
 *
 * To be used in GST_PLUGIN_DEFINE or GST_PLUGIN_DEFINE_STATIC if usure about
 * the licence.
 */
#define GST_LICENSE_UNKNOWN "unknown"


/* function for filters */
/**
 * GstPluginFilter:
 * @plugin: the plugin to check
 * @user_data: the user_data that has been passed on e.g. gst_registry_plugin_filter()
 *
 * A function that can be used with e.g. gst_registry_plugin_filter()
 * to get a list of plugins that match certain criteria.
 *
 * Returns: TRUE for a positive match, FALSE otherwise
 */
typedef gboolean        (*GstPluginFilter)              (GstPlugin *plugin,
                                                         gpointer user_data);

GType                   gst_plugin_get_type             (void);

gboolean		gst_plugin_register_static	(gint major_version,
                                                         gint minor_version,
                                                         const gchar *name,
                                                         const gchar *description,
                                                         GstPluginInitFunc init_func,
                                                         const gchar *version,
                                                         const gchar *license,
                                                         const gchar *source,
                                                         const gchar *package,
                                                         const gchar *origin);

gboolean		gst_plugin_register_static_full	(gint major_version,
                                                         gint minor_version,
                                                         const gchar *name,
                                                         const gchar *description,
                                                         GstPluginInitFullFunc init_full_func,
                                                         const gchar *version,
                                                         const gchar *license,
                                                         const gchar *source,
                                                         const gchar *package,
                                                         const gchar *origin,
                                                         gpointer user_data);

const gchar*		gst_plugin_get_name		(GstPlugin *plugin);
const gchar*		gst_plugin_get_description	(GstPlugin *plugin);
const gchar*		gst_plugin_get_filename		(GstPlugin *plugin);
const gchar*		gst_plugin_get_version		(GstPlugin *plugin);
const gchar*		gst_plugin_get_license		(GstPlugin *plugin);
const gchar*		gst_plugin_get_source		(GstPlugin *plugin);
const gchar*		gst_plugin_get_package		(GstPlugin *plugin);
const gchar*		gst_plugin_get_origin		(GstPlugin *plugin);
const GstStructure*	gst_plugin_get_cache_data	(GstPlugin * plugin);
void			gst_plugin_set_cache_data	(GstPlugin * plugin, GstStructure *cache_data);

GModule *		gst_plugin_get_module		(GstPlugin *plugin);
gboolean		gst_plugin_is_loaded		(GstPlugin *plugin);

gboolean		gst_plugin_name_filter		(GstPlugin *plugin, const gchar *name);

GstPlugin *		gst_plugin_load_file		(const gchar *filename, GError** error);

GstPlugin *             gst_plugin_load                 (GstPlugin *plugin);
GstPlugin *             gst_plugin_load_by_name         (const gchar *name);

void                    gst_plugin_add_dependency (GstPlugin    * plugin,
                                                   const gchar ** env_vars,
                                                   const gchar ** paths,
                                                   const gchar ** names,
                                                   GstPluginDependencyFlags flags);

void                    gst_plugin_add_dependency_simple (GstPlugin   * plugin,
                                                          const gchar * env_vars,
                                                          const gchar * paths,
                                                          const gchar * names,
                                                          GstPluginDependencyFlags flags);

void gst_plugin_list_free (GList *list);

G_END_DECLS

#endif /* __GST_PLUGIN_H__ */

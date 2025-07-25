/* -*- mode: C; c-file-style: "linux" -*- */
/* GdkPixbuf library - Main loading interface.
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Miguel de Icaza <miguel@gnu.org>
 *          Federico Mena-Quintero <federico@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include "gdk-pixbuf-private.h"
#include "gdk-pixbuf-loader.h"

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#undef STRICT
#endif

#define SNIFF_BUFFER_SIZE 4096
#define LOAD_BUFFER_SIZE 65536

#ifndef GDK_PIXBUF_USE_GIO_MIME 
static gint 
format_check (GdkPixbufModule *module, guchar *buffer, int size)
{
        int i, j;
        gchar m;
        GdkPixbufModulePattern *pattern;
        gboolean anchored;
        guchar *prefix;
        gchar *mask;

        for (pattern = module->info->signature; pattern->prefix; pattern++) {
                if (pattern->mask && pattern->mask[0] == '*') {
                        prefix = (guchar *)pattern->prefix + 1;
                        mask = pattern->mask + 1;
                        anchored = FALSE;
                }
                else {
                        prefix = (guchar *)pattern->prefix;
                        mask = pattern->mask;
                        anchored = TRUE;
                }
                for (i = 0; i < size; i++) {
                        for (j = 0; i + j < size && prefix[j] != 0; j++) {
                                m = mask ? mask[j] : ' ';
                                if (m == ' ') {
                                        if (buffer[i + j] != prefix[j])
                                                break;
                                }
                                else if (m == '!') {
                                        if (buffer[i + j] == prefix[j])
                                                break;
                                }
                                else if (m == 'z') {
                                        if (buffer[i + j] != 0)
                                                break;
                                }
                                else if (m == 'n') {
                                        if (buffer[i + j] == 0)
                                                break;
                                }
                        } 

                        if (prefix[j] == 0) 
                                return pattern->relevance;

                        if (anchored)
                                break;
                }
        }
        return 0;
}
#endif

G_LOCK_DEFINE_STATIC (init_lock);
G_LOCK_DEFINE_STATIC (threadunsafe_loader_lock);

gboolean
_gdk_pixbuf_lock (GdkPixbufModule *image_module)
{
        if (g_threads_got_initialized &&
            !(image_module->info->flags & GDK_PIXBUF_FORMAT_THREADSAFE)) {
                G_LOCK (threadunsafe_loader_lock);

                return TRUE;
        }

        return FALSE;
}
 
void
_gdk_pixbuf_unlock (GdkPixbufModule *image_module)
{
        if (!(image_module->info->flags & GDK_PIXBUF_FORMAT_THREADSAFE)) {
                G_UNLOCK (threadunsafe_loader_lock);
        }
}

static GSList *file_formats = NULL;

static void gdk_pixbuf_io_init (void);

static GSList *
get_file_formats (void)
{
        G_LOCK (init_lock);
        if (file_formats == NULL)
                gdk_pixbuf_io_init ();
        G_UNLOCK (init_lock);
        
        return file_formats;
}


#ifdef USE_GMODULE 

static gboolean
scan_string (const char **pos, GString *out)
{
        const char *p = *pos, *q = *pos;
        char *tmp, *tmp2;
        gboolean quoted;
        
        while (g_ascii_isspace (*p))
                p++;
        
        if (!*p)
                return FALSE;
        else if (*p == '"') {
                p++;
                quoted = FALSE;
                for (q = p; (*q != '"') || quoted; q++) {
                        if (!*q)
                                return FALSE;
                        quoted = (*q == '\\') && !quoted;
                }
                
                tmp = g_strndup (p, q - p);
                tmp2 = g_strcompress (tmp);
                g_string_truncate (out, 0);
                g_string_append (out, tmp2);
                g_free (tmp);
                g_free (tmp2);
        }
        
        q++;
        *pos = q;
        
        return TRUE;
}

static gboolean
scan_int (const char **pos, int *out)
{
        int i = 0;
        char buf[32];
        const char *p = *pos;
        
        while (g_ascii_isspace (*p))
                p++;
        
        if (*p < '0' || *p > '9')
                return FALSE;
        
        while ((*p >= '0') && (*p <= '9') && i < sizeof (buf)) {
                buf[i] = *p;
                i++;
                p++;
        }
        
        if (i == sizeof (buf))
                return FALSE;
        else
                buf[i] = '\0';
        
        *out = atoi (buf);
        
        *pos = p;

        return TRUE;
}

static gboolean
skip_space (const char **pos)
{
        const char *p = *pos;
        
        while (g_ascii_isspace (*p))
                p++;
  
        *pos = p;
        
        return !(*p == '\0');
}
  
#ifdef G_OS_WIN32

/* DllMain function needed to tuck away the gdk-pixbuf DLL handle */

static HMODULE gdk_pixbuf_dll;

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
        switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
                gdk_pixbuf_dll = (HMODULE) hinstDLL;
                break;
        }

  return TRUE;
}

static char *
get_toplevel (void)
{
  static char *toplevel = NULL;

  if (toplevel == NULL)
          toplevel = g_win32_get_package_installation_directory_of_module (gdk_pixbuf_dll);

  return toplevel;
}

static char *
get_sysconfdir (void)
{
  static char *sysconfdir = NULL;

  if (sysconfdir == NULL)
          sysconfdir = g_build_filename (get_toplevel (), "etc", NULL);

  return sysconfdir;
}

#undef GDK_PIXBUF_SYSCONFDIR
#define GDK_PIXBUF_SYSCONFDIR get_sysconfdir()

static void
correct_prefix (gchar **path)
{
  if (strncmp (*path, GDK_PIXBUF_PREFIX "/", strlen (GDK_PIXBUF_PREFIX "/")) == 0 ||
      strncmp (*path, GDK_PIXBUF_PREFIX "\\", strlen (GDK_PIXBUF_PREFIX "\\")) == 0)
    {
          gchar *tem = NULL;
      if (strlen(*path) > 5 && strncmp (*path - 5, ".libs", 5) == 0)
        {
          /* We are being run from inside the build tree, and shouldn't mess about. */
          return;
        }

      /* This is an entry put there by gdk-pixbuf-query-loaders on the
       * packager's system. On Windows a prebuilt gdk-pixbuf package can be
       * installed in a random location. The gdk-pixbuf.loaders file
       * distributed in such a package contains paths from the package
       * builder's machine. Replace the build-time prefix with the
       * installation prefix on this machine.
       */
      tem = *path;
      *path = g_strconcat (get_toplevel (), tem + strlen (GDK_PIXBUF_PREFIX), NULL);
      g_free (tem);
    }
}

#endif  /* G_OS_WIN32 */

static gchar *
gdk_pixbuf_get_module_file (void)
{
  gchar *result = g_strdup (g_getenv ("GDK_PIXBUF_MODULE_FILE"));

  if (!result)
          result = g_build_filename (GDK_PIXBUF_LIBDIR, "gdk-pixbuf-2.0", GDK_PIXBUF_BINARY_VERSION, "loaders.cache", NULL);

  return result;
}

#endif  /* USE_GMODULE */


static gboolean
gdk_pixbuf_load_module_unlocked (GdkPixbufModule *image_module,
                                 GError         **error);

static void 
gdk_pixbuf_io_init (void)
{
#ifdef USE_GMODULE
        GIOChannel *channel;
        gchar *line_buf;
        gsize term;
        GString *tmp_buf = g_string_new (NULL);
        gboolean have_error = FALSE;
        GdkPixbufModule *module = NULL;
        gchar *filename = gdk_pixbuf_get_module_file ();
        int flags;
        int n_patterns = 0;
        GdkPixbufModulePattern *pattern;
        GError *error = NULL;
#endif
        GdkPixbufModule *builtin_module ;

        /*  initialize on separate line to avoid compiler warnings in the
         *  common case of no compiled-in modules.
         */
        builtin_module = NULL;

#define load_one_builtin_module(format)                                 \
        builtin_module = g_new0 (GdkPixbufModule, 1);                   \
        builtin_module->module_name = #format;                          \
        if (gdk_pixbuf_load_module_unlocked (builtin_module, NULL))             \
                file_formats = g_slist_prepend (file_formats, builtin_module);\
        else                                                            \
                g_free (builtin_module)

#ifdef INCLUDE_ani
        load_one_builtin_module (ani);
#endif
#ifdef INCLUDE_png
        load_one_builtin_module (png);
#endif
#ifdef INCLUDE_bmp
        load_one_builtin_module (bmp);
#endif
#ifdef INCLUDE_wbmp
        load_one_builtin_module (wbmp);
#endif
#ifdef INCLUDE_gif
        load_one_builtin_module (gif);
#endif
#ifdef INCLUDE_ico
        load_one_builtin_module (ico);
#endif
#ifdef INCLUDE_jpeg
        load_one_builtin_module (jpeg);
#endif
#ifdef INCLUDE_pnm
        load_one_builtin_module (pnm);
#endif
#ifdef INCLUDE_ras
        load_one_builtin_module (ras);
#endif
#ifdef INCLUDE_tiff
        load_one_builtin_module (tiff);
#endif
#ifdef INCLUDE_xpm
        load_one_builtin_module (xpm);
#endif
#ifdef INCLUDE_xbm
        load_one_builtin_module (xbm);
#endif
#ifdef INCLUDE_tga
        load_one_builtin_module (tga);
#endif
#ifdef INCLUDE_pcx
        load_one_builtin_module (pcx);
#endif
#ifdef INCLUDE_icns
        load_one_builtin_module (icns);
#endif
#ifdef INCLUDE_jasper
        load_one_builtin_module (jasper);
#endif
#ifdef INCLUDE_qtif
        load_one_builtin_module (qtif);
#endif
#ifdef INCLUDE_gdiplus
        /* We don't bother having the GDI+ loaders individually selectable
         * for building in or not.
         */
        load_one_builtin_module (ico);
        load_one_builtin_module (wmf);
        load_one_builtin_module (emf);
        load_one_builtin_module (bmp);
        load_one_builtin_module (gif);
        load_one_builtin_module (jpeg);
        load_one_builtin_module (tiff);
#endif
#ifdef INCLUDE_gdip_png
        /* Except the gdip-png loader which normally isn't built at all even */
        load_one_builtin_module (png);
#endif

#undef load_one_builtin_module

#ifdef USE_GMODULE
        channel = g_io_channel_new_file (filename, "r",  &error);
        if (!channel) {
                /* Don't bother warning if we have some built-in loaders */
                if (file_formats == NULL)
                        g_warning ("Cannot open pixbuf loader module file '%s': %s",
                                   filename, error->message);
                g_string_free (tmp_buf, TRUE);
                g_free (filename);
                return;
        }
        
        while (!have_error && g_io_channel_read_line (channel, &line_buf, NULL, &term, NULL) == G_IO_STATUS_NORMAL) {
                const char *p;
                
                p = line_buf;

                line_buf[term] = 0;

                if (!skip_space (&p)) {
                                /* Blank line marking the end of a module
                                 */
                        if (module && *p != '#') {
#ifdef G_OS_WIN32
                                correct_prefix (&module->module_path);
#endif
                                file_formats = g_slist_prepend (file_formats, module);
                                module = NULL;
                        }
                        
                        goto next_line;
                }

                if (*p == '#') 
                        goto next_line;
                
                if (!module) {
                                /* Read a module location
                                 */
                        module = g_new0 (GdkPixbufModule, 1);
                        n_patterns = 0;
                        
                        if (!scan_string (&p, tmp_buf)) {
                                g_warning ("Error parsing loader info in '%s'\n  %s", 
                                           filename, line_buf);
                                have_error = TRUE;
                        }
                        module->module_path = g_strdup (tmp_buf->str);
                }
                else if (!module->module_name) {
                        module->info = g_new0 (GdkPixbufFormat, 1);
                        if (!scan_string (&p, tmp_buf)) {
                                g_warning ("Error parsing loader info in '%s'\n  %s", 
                                           filename, line_buf);
                                have_error = TRUE;
                        }
                        module->info->name =  g_strdup (tmp_buf->str);
                        module->module_name = module->info->name;

                        if (!scan_int (&p, &flags)) {
                                g_warning ("Error parsing loader info in '%s'\n  %s", 
                                           filename, line_buf);
                                have_error = TRUE;
                        }
                        module->info->flags = flags;
                        
                        if (!scan_string (&p, tmp_buf)) {
                                g_warning ("Error parsing loader info in '%s'\n  %s", 
                                           filename, line_buf);
                                have_error = TRUE;
                        }                       
                        if (tmp_buf->str[0] != 0)
                                module->info->domain = g_strdup (tmp_buf->str);

                        if (!scan_string (&p, tmp_buf)) {
                                g_warning ("Error parsing loader info in '%s'\n  %s", 
                                           filename, line_buf);
                                have_error = TRUE;
                        }                       
                        module->info->description = g_strdup (tmp_buf->str);

                        if (scan_string (&p, tmp_buf)) {
                                module->info->license = g_strdup (tmp_buf->str);
                        }
                }
                else if (!module->info->mime_types) {
                        int n = 1;
                        module->info->mime_types = g_new0 (gchar*, 1);
                        while (scan_string (&p, tmp_buf)) {
                                if (tmp_buf->str[0] != 0) {
                                        module->info->mime_types =
                                                g_realloc (module->info->mime_types, (n + 1) * sizeof (gchar*));
                                        module->info->mime_types[n - 1] = g_strdup (tmp_buf->str);
                                        module->info->mime_types[n] = NULL;
                                        n++;
                                }
                        }
                }
                else if (!module->info->extensions) {
                        int n = 1;
                        module->info->extensions = g_new0 (gchar*, 1);
                        while (scan_string (&p, tmp_buf)) {
                                if (tmp_buf->str[0] != 0) {
                                        module->info->extensions =
                                                g_realloc (module->info->extensions, (n + 1) * sizeof (gchar*));
                                        module->info->extensions[n - 1] = g_strdup (tmp_buf->str);
                                        module->info->extensions[n] = NULL;
                                        n++;
                                }
                        }
                }
                else {
                        n_patterns++;
                        module->info->signature = (GdkPixbufModulePattern *)
                                g_realloc (module->info->signature, (n_patterns + 1) * sizeof (GdkPixbufModulePattern));
                        pattern = module->info->signature + n_patterns;
                        pattern->prefix = NULL;
                        pattern->mask = NULL;
                        pattern->relevance = 0;
                        pattern--;
                        if (!scan_string (&p, tmp_buf))
                                goto context_error;
                        pattern->prefix = g_strdup (tmp_buf->str);
                        
                        if (!scan_string (&p, tmp_buf))
                                goto context_error;
                        if (*tmp_buf->str)
                                pattern->mask = g_strdup (tmp_buf->str);
                        else
                                pattern->mask = NULL;
                        
                        if (!scan_int (&p, &pattern->relevance))
                                goto context_error;
                        
                        goto next_line;

                context_error:
                        g_free (pattern->prefix);
                        g_free (pattern->mask);
                        g_free (pattern);
                        g_warning ("Error parsing loader info in '%s'\n  %s", 
                                   filename, line_buf);
                        have_error = TRUE;
                }
        next_line:
                g_free (line_buf);
        }
        g_string_free (tmp_buf, TRUE);
        g_io_channel_unref (channel);
        g_free (filename);
#endif
}


#define module(type) \
  extern void _gdk_pixbuf__##type##_fill_info   (GdkPixbufFormat *info);   \
  extern void _gdk_pixbuf__##type##_fill_vtable (GdkPixbufModule *module)

module (png);
module (jpeg);
module (gif);
module (ico);
module (ani);
module (ras);
module (xpm);
module (tiff);
module (pnm);
module (bmp);
module (wbmp);
module (xbm);
module (tga);
module (pcx);
module (icns);
module (jasper);
module (qtif);
module (gdip_ico);
module (gdip_wmf);
module (gdip_emf);
module (gdip_bmp);
module (gdip_gif);
module (gdip_jpeg);
module (gdip_png);
module (gdip_tiff);

#undef module

/* actually load the image handler - gdk_pixbuf_get_module only get a */
/* reference to the module to load, it doesn't actually load it       */
/* perhaps these actions should be combined in one function           */
static gboolean
gdk_pixbuf_load_module_unlocked (GdkPixbufModule *image_module,
                                 GError         **error)
{
        GdkPixbufModuleFillInfoFunc fill_info = NULL;
        GdkPixbufModuleFillVtableFunc fill_vtable = NULL;
                
        if (image_module->module != NULL)
               return TRUE;

#define try_module(format,id)                                           \
        if (fill_info == NULL &&                                        \
            strcmp (image_module->module_name, #format) == 0) {         \
                fill_info = _gdk_pixbuf__##id##_fill_info;              \
                fill_vtable = _gdk_pixbuf__##id##_fill_vtable;  \
        }
#ifdef INCLUDE_png      
        try_module (png,png);
#endif
#ifdef INCLUDE_bmp
        try_module (bmp,bmp);
#endif
#ifdef INCLUDE_wbmp
        try_module (wbmp,wbmp);
#endif
#ifdef INCLUDE_gif
        try_module (gif,gif);
#endif
#ifdef INCLUDE_ico
        try_module (ico,ico);
#endif
#ifdef INCLUDE_ani
        try_module (ani,ani);
#endif
#ifdef INCLUDE_jpeg
        try_module (jpeg,jpeg);
#endif
#ifdef INCLUDE_pnm
        try_module (pnm,pnm);
#endif
#ifdef INCLUDE_ras
        try_module (ras,ras);
#endif
#ifdef INCLUDE_tiff
        try_module (tiff,tiff);
#endif
#ifdef INCLUDE_xpm
        try_module (xpm,xpm);
#endif
#ifdef INCLUDE_xbm
        try_module (xbm,xbm);
#endif
#ifdef INCLUDE_tga
        try_module (tga,tga);
#endif
#ifdef INCLUDE_pcx
        try_module (pcx,pcx);
#endif
#ifdef INCLUDE_icns
        try_module (icns,icns);
#endif
#ifdef INCLUDE_jasper
        try_module (jasper,jasper);
#endif
#ifdef INCLUDE_qtif
        try_module (qtif,qtif);
#endif
#ifdef INCLUDE_gdiplus
        try_module (ico,gdip_ico);
        try_module (wmf,gdip_wmf);
        try_module (emf,gdip_emf);
        try_module (bmp,gdip_bmp);
        try_module (gif,gdip_gif);
        try_module (jpeg,gdip_jpeg);
        try_module (tiff,gdip_tiff);
#endif
#ifdef INCLUDE_gdip_png
        try_module (png,gdip_png);
#endif

#undef try_module
        
        if (fill_vtable) {
                image_module->module = (void *) 1;
                (* fill_vtable) (image_module);
                if (image_module->info == NULL) {
                        image_module->info = g_new0 (GdkPixbufFormat, 1);
                        (* fill_info) (image_module->info);
                }
                return TRUE;
        }
        else 
#ifdef USE_GMODULE
        {
                char *path;
                GModule *module;
                gpointer sym;

                path = image_module->module_path;
                module = g_module_open (path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

                if (!module) {
                        g_set_error (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     _("Unable to load image-loading module: %s: %s"),
                                     path, g_module_error ());
                        return FALSE;
                }

                image_module->module = module;        
        
                if (g_module_symbol (module, "fill_vtable", &sym)) {
                        fill_vtable = (GdkPixbufModuleFillVtableFunc) sym;
                        (* fill_vtable) (image_module);
                        return TRUE;
                } else {
                        g_set_error (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     _("Image-loading module %s does not export the proper interface; perhaps it's from a different gdk-pixbuf version?"),
                                     path);
                        return FALSE;
                }
        }
#else
        g_set_error (error,
                     GDK_PIXBUF_ERROR,
                     GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                     _("Image type '%s' is not supported"),
                     image_module->module_name);
        return FALSE;
#endif  /* !USE_GMODULE */
}


gboolean
_gdk_pixbuf_load_module (GdkPixbufModule *image_module,
                         GError         **error)
{
        gboolean ret;
        gboolean locked = FALSE;

        /* be extra careful, maybe the module initializes
         * the thread system
         */
        if (g_threads_got_initialized) {
                G_LOCK (init_lock);
                locked = TRUE;
        }

        ret = gdk_pixbuf_load_module_unlocked (image_module, error);

        if (locked)
                G_UNLOCK (init_lock);

        return ret;
}



GdkPixbufModule *
_gdk_pixbuf_get_named_module (const char *name,
                              GError **error)
{
        GSList *modules;

        for (modules = get_file_formats (); modules; modules = g_slist_next (modules)) {
                GdkPixbufModule *module = (GdkPixbufModule *)modules->data;

                if (module->info->disabled)
                        continue;

                if (!strcmp (name, module->module_name))
                        return module;
        }

        g_set_error (error,
                     GDK_PIXBUF_ERROR,
                     GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                     _("Image type '%s' is not supported"),
                     name);
        
        return NULL;
}

GdkPixbufModule *
_gdk_pixbuf_get_module (guchar *buffer, guint size,
                        const gchar *filename,
                        GError **error)
{
        GSList *modules;

        GdkPixbufModule *selected = NULL;
        gchar *display_name = NULL;
#ifdef GDK_PIXBUF_USE_GIO_MIME
        gchar *mime_type;
        gchar **mimes;
        gchar *type;
        gint j;
        gboolean uncertain;

        mime_type = g_content_type_guess (NULL, buffer, size, &uncertain);
        if (uncertain)
                mime_type = g_content_type_guess (filename, buffer, size, NULL);

        for (modules = get_file_formats (); modules; modules = g_slist_next (modules)) {
                GdkPixbufModule *module = (GdkPixbufModule *)modules->data;
                GdkPixbufFormat *info = module->info;

                if (info->disabled)
                        continue;

                mimes = info->mime_types;
                for (j = 0; mimes[j] != NULL; j++) {
                        type = g_content_type_from_mime_type (mimes[j]);
                        if (g_content_type_equals (type, mime_type)) {
                                g_free (type);
                                selected = module;
                                break;
                        }
                        g_free (type);
                }
        }
        g_free (mime_type);
#else
        gint score, best = 0;

        for (modules = get_file_formats (); modules; modules = g_slist_next (modules)) {
                GdkPixbufModule *module = (GdkPixbufModule *)modules->data;

                if (module->info->disabled)
                        continue;

                score = format_check (module, buffer, size);
                if (score > best) {
                        best = score; 
                        selected = module;
                }
                if (score >= 100) 
                        break;
        }
#endif

        if (selected != NULL)
                return selected;

        if (filename)
        {
                display_name = g_filename_display_name (filename);
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                             _("Couldn't recognize the image file format for file '%s'"),
                             display_name);
                g_free (display_name);
        }
        else
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                     _("Unrecognized image file format"));


        return NULL;
}


static void
prepared_notify (GdkPixbuf *pixbuf, 
                 GdkPixbufAnimation *anim, 
                 gpointer user_data)
{
        if (pixbuf != NULL)
                g_object_ref (pixbuf);
        *((GdkPixbuf **)user_data) = pixbuf;
}

GdkPixbuf *
_gdk_pixbuf_generic_image_load (GdkPixbufModule *module,
                                FILE *f,
                                GError **error)
{
        guchar buffer[LOAD_BUFFER_SIZE];
        size_t length;
        GdkPixbuf *pixbuf = NULL;
        GdkPixbufAnimation *animation = NULL;
        gpointer context;
        gboolean locked;

        locked = _gdk_pixbuf_lock (module);

        if (module->load != NULL) {
                pixbuf = (* module->load) (f, error);
        } else if (module->begin_load != NULL) {
                
                context = module->begin_load (NULL, prepared_notify, NULL, &pixbuf, error);
        
                if (!context)
                        goto out;
                
                while (!feof (f) && !ferror (f)) {
                        length = fread (buffer, 1, sizeof (buffer), f);
                        if (length > 0)
                                if (!module->load_increment (context, buffer, length, error)) {
                                        module->stop_load (context, NULL);
                                        if (pixbuf != NULL) {
                                                g_object_unref (pixbuf);
                                                pixbuf = NULL;
                                        }
                                        goto out;
                                }
                }
                
                if (!module->stop_load (context, error)) {
                        if (pixbuf != NULL) {
                                g_object_unref (pixbuf);
                                pixbuf = NULL;
                        }
                }
        } else if (module->load_animation != NULL) {
                animation = (* module->load_animation) (f, error);
                if (animation != NULL) {
                        pixbuf = gdk_pixbuf_animation_get_static_image (animation);

                        g_object_ref (pixbuf);
                        g_object_unref (animation);
                }
        }

 out:
        if (locked)
                _gdk_pixbuf_unlock (module);
        return pixbuf;
}

/**
 * gdk_pixbuf_new_from_file:
 * @filename: Name of file to load, in the GLib file name encoding
 * @error: Return location for an error
 *
 * Creates a new pixbuf by loading an image from a file.  The file format is
 * detected automatically. If %NULL is returned, then @error will be set.
 * Possible errors are in the #GDK_PIXBUF_ERROR and #G_FILE_ERROR domains.
 *
 * Return value: A newly-created pixbuf with a reference count of 1, or %NULL if
 * any of several error conditions occurred:  the file could not be opened,
 * there was no loader for the file's format, there was not enough memory to
 * allocate the image buffer, or the image file contained invalid data.
 **/
GdkPixbuf *
gdk_pixbuf_new_from_file (const char *filename,
                          GError    **error)
{
        GdkPixbuf *pixbuf;
        int size;
        FILE *f;
        guchar buffer[SNIFF_BUFFER_SIZE];
        GdkPixbufModule *image_module;
        gchar *display_name;

        g_return_val_if_fail (filename != NULL, NULL);
        g_return_val_if_fail (error == NULL || *error == NULL, NULL);
        
        display_name = g_filename_display_name (filename);      

        f = g_fopen (filename, "rb");
        if (!f) {
                gint save_errno = errno;
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (save_errno),
                             _("Failed to open file '%s': %s"),
                             display_name,
                             g_strerror (save_errno));
                g_free (display_name);
                return NULL;
        }

        size = fread (&buffer, 1, sizeof (buffer), f);
        if (size == 0) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                             _("Image file '%s' contains no data"),
                             display_name);
                g_free (display_name);
                fclose (f);
                return NULL;
        }

        image_module = _gdk_pixbuf_get_module (buffer, size, filename, error);
        if (image_module == NULL) {
                g_free (display_name);
                fclose (f);
                return NULL;
        }

        if (!_gdk_pixbuf_load_module (image_module, error)) {
                g_free (display_name);
                fclose (f);
                return NULL;
        }

        fseek (f, 0, SEEK_SET);
        pixbuf = _gdk_pixbuf_generic_image_load (image_module, f, error);
        fclose (f);

        if (pixbuf == NULL && error != NULL && *error == NULL) {

                /* I don't trust these crufty longjmp()'ing image libs
                 * to maintain proper error invariants, and I don't
                 * want user code to segfault as a result. We need to maintain
                 * the invariant that error gets set if NULL is returned.
                 */

                g_warning ("Bug! gdk-pixbuf loader '%s' didn't set an error on failure.", image_module->module_name);
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
                             _("Failed to load image '%s': reason not known, probably a corrupt image file"),
                             display_name);
        } else if (error != NULL && *error != NULL) {

          /* Add the filename to the error message */
          GError *e = *error;
          gchar  *old;

          old = e->message;
          e->message = g_strdup_printf (_("Failed to load image '%s': %s"),
                                        display_name,
                                        old);
          g_free (old);
        }

        g_free (display_name);
        return pixbuf;
}

#ifdef G_OS_WIN32

#undef gdk_pixbuf_new_from_file
GdkPixbuf *
gdk_pixbuf_new_from_file (const char *filename,
                          GError    **error)
{
        gchar *utf8_filename =
                g_locale_to_utf8 (filename, -1, NULL, NULL, error);
        GdkPixbuf *retval;

        if (utf8_filename == NULL)
                return NULL;

        retval = gdk_pixbuf_new_from_file_utf8 (utf8_filename, error);

        g_free (utf8_filename);

        return retval;
}
#endif


/**
 * gdk_pixbuf_new_from_file_at_size:
 * @filename: Name of file to load, in the GLib file name encoding
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @error: Return location for an error
 *
 * Creates a new pixbuf by loading an image from a file.  
 * The file format is detected automatically. If %NULL is returned, then 
 * @error will be set. Possible errors are in the #GDK_PIXBUF_ERROR and 
 * #G_FILE_ERROR domains.
 *
 * The image will be scaled to fit in the requested size, preserving
 * the image's aspect ratio. Note that the returned pixbuf may be smaller
 * than @width x @height, if the aspect ratio requires it. To load
 * and image at the requested size, regardless of aspect ratio, use
 * gdk_pixbuf_new_from_file_at_scale().
 *
 * Return value: A newly-created pixbuf with a reference count of 1, or 
 * %NULL if any of several error conditions occurred:  the file could not 
 * be opened, there was no loader for the file's format, there was not 
 * enough memory to allocate the image buffer, or the image file contained 
 * invalid data.
 *
 * Since: 2.4
 **/
GdkPixbuf *
gdk_pixbuf_new_from_file_at_size (const char *filename,
                                  int         width, 
                                  int         height,
                                  GError    **error)
{
        return gdk_pixbuf_new_from_file_at_scale (filename, 
                                                  width, height, 
                                                  TRUE, error);
}

#ifdef G_OS_WIN32

#undef gdk_pixbuf_new_from_file_at_size

GdkPixbuf *
gdk_pixbuf_new_from_file_at_size (const char *filename,
                                  int         width, 
                                  int         height,
                                  GError    **error)
{
        gchar *utf8_filename =
                g_locale_to_utf8 (filename, -1, NULL, NULL, error);
        GdkPixbuf *retval;

        if (utf8_filename == NULL)
                return NULL;

        retval = gdk_pixbuf_new_from_file_at_size_utf8 (utf8_filename,
                                                        width, height,
                                                        error);

        g_free (utf8_filename);

        return retval;
}
#endif

typedef struct {
        gint width;
        gint height;
        gboolean preserve_aspect_ratio;
} AtScaleData; 

static void
at_scale_size_prepared_cb (GdkPixbufLoader *loader, 
                           int              width,
                           int              height,
                           gpointer         data)
{
        AtScaleData *info = data;

        g_return_if_fail (width > 0 && height > 0);

        if (info->preserve_aspect_ratio && 
            (info->width > 0 || info->height > 0)) {
                if (info->width < 0)
                {
                        width = width * (double)info->height/(double)height;
                        height = info->height;
                }
                else if (info->height < 0)
                {
                        height = height * (double)info->width/(double)width;
                        width = info->width;
                }
                else if ((double)height * (double)info->width >
                         (double)width * (double)info->height) {
                        width = 0.5 + (double)width * (double)info->height / (double)height;
                        height = info->height;
                } else {
                        height = 0.5 + (double)height * (double)info->width / (double)width;
                        width = info->width;
                }
        } else {
                if (info->width > 0)
                        width = info->width;
                if (info->height > 0)
                        height = info->height;
        }
        
        width = MAX (width, 1);
        height = MAX (height, 1);

        gdk_pixbuf_loader_set_size (loader, width, height);
}

/**
 * gdk_pixbuf_new_from_file_at_scale:
 * @filename: Name of file to load, in the GLib file name encoding
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: %TRUE to preserve the image's aspect ratio
 * @error: Return location for an error
 *
 * Creates a new pixbuf by loading an image from a file.  The file format is
 * detected automatically. If %NULL is returned, then @error will be set.
 * Possible errors are in the #GDK_PIXBUF_ERROR and #G_FILE_ERROR domains.
 * The image will be scaled to fit in the requested size, optionally preserving
 * the image's aspect ratio. 
 *
 * When preserving the aspect ratio, a @width of -1 will cause the image
 * to be scaled to the exact given height, and a @height of -1 will cause
 * the image to be scaled to the exact given width. When not preserving
 * aspect ratio, a @width or @height of -1 means to not scale the image 
 * at all in that dimension. Negative values for @width and @height are 
 * allowed since 2.8.
 *
 * Return value: A newly-created pixbuf with a reference count of 1, or %NULL 
 * if any of several error conditions occurred:  the file could not be opened,
 * there was no loader for the file's format, there was not enough memory to
 * allocate the image buffer, or the image file contained invalid data.
 *
 * Since: 2.6
 **/
GdkPixbuf *
gdk_pixbuf_new_from_file_at_scale (const char *filename,
                                   int         width, 
                                   int         height,
                                   gboolean    preserve_aspect_ratio,
                                   GError    **error)
{

        GdkPixbufLoader *loader;
        GdkPixbuf       *pixbuf;
        guchar buffer[LOAD_BUFFER_SIZE];
        int length;
        FILE *f;
        AtScaleData info;
        GdkPixbufAnimation *animation;
        GdkPixbufAnimationIter *iter;
        gboolean has_frame;

        g_return_val_if_fail (filename != NULL, NULL);
        g_return_val_if_fail (width > 0 || width == -1, NULL);
        g_return_val_if_fail (height > 0 || height == -1, NULL);

        f = g_fopen (filename, "rb");
        if (!f) {
                gint save_errno = errno;
                gchar *display_name = g_filename_display_name (filename);
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (save_errno),
                             _("Failed to open file '%s': %s"),
                             display_name,
                             g_strerror (save_errno));
                g_free (display_name);
                return NULL;
        }

        loader = gdk_pixbuf_loader_new ();

        info.width = width;
        info.height = height;
        info.preserve_aspect_ratio = preserve_aspect_ratio;

        g_signal_connect (loader, "size-prepared", 
                          G_CALLBACK (at_scale_size_prepared_cb), &info);

        has_frame = FALSE;
        while (!has_frame && !feof (f) && !ferror (f)) {
                length = fread (buffer, 1, sizeof (buffer), f);
                if (length > 0)
                        if (!gdk_pixbuf_loader_write (loader, buffer, length, error)) {
                                gdk_pixbuf_loader_close (loader, NULL);
                                fclose (f);
                                g_object_unref (loader);
                                return NULL;
                        }
                
                animation = gdk_pixbuf_loader_get_animation (loader);
                if (animation) {
                        iter = gdk_pixbuf_animation_get_iter (animation, NULL);
                        if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter)) {
                                has_frame = TRUE;
                        }
                        g_object_unref (iter);
                }
        }

        fclose (f);

        if (!gdk_pixbuf_loader_close (loader, error) && !has_frame) {
                g_object_unref (loader);
                return NULL;
        }

        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

        if (!pixbuf) {
                gchar *display_name = g_filename_display_name (filename);
                g_object_unref (loader);
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
                             _("Failed to load image '%s': reason not known, probably a corrupt image file"),
                             display_name);
                g_free (display_name);
                return NULL;
        }

        g_object_ref (pixbuf);

        g_object_unref (loader);

        return pixbuf;
}

#ifdef G_OS_WIN32

#undef gdk_pixbuf_new_from_file_at_scale

GdkPixbuf *
gdk_pixbuf_new_from_file_at_scale (const char *filename,
                                   int         width, 
                                   int         height,
                                   gboolean    preserve_aspect_ratio,
                                   GError    **error)
{
        gchar *utf8_filename =
                g_locale_to_utf8 (filename, -1, NULL, NULL, error);
        GdkPixbuf *retval;

        if (utf8_filename == NULL)
                return NULL;

        retval = gdk_pixbuf_new_from_file_at_scale_utf8 (utf8_filename,
                                                         width, height,
                                                         preserve_aspect_ratio,
                                                         error);

        g_free (utf8_filename);

        return retval;
}
#endif


static GdkPixbuf *
load_from_stream (GdkPixbufLoader  *loader,
                  GInputStream     *stream,
                  GCancellable     *cancellable,
                  GError          **error)
{
        GdkPixbuf *pixbuf;
        gssize n_read;
        guchar buffer[LOAD_BUFFER_SIZE];
        gboolean res;

        res = TRUE;
        while (1) { 
                n_read = g_input_stream_read (stream, 
                                              buffer, 
                                              sizeof (buffer), 
                                              cancellable, 
                                              error);
                if (n_read < 0) {
                        res = FALSE;
                        error = NULL; /* Ignore further errors */
                        break;
                }

                if (n_read == 0)
                        break;

                if (!gdk_pixbuf_loader_write (loader, 
                                              buffer, 
                                              n_read, 
                                              error)) {
                        res = FALSE;
                        error = NULL;
                        break;
                }
        }

        if (!gdk_pixbuf_loader_close (loader, error)) {
                res = FALSE;
                error = NULL;
        }

        pixbuf = NULL;
        if (res) {
                pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
                if (pixbuf)
                        g_object_ref (pixbuf);
        }

        return pixbuf;
}


/**
 * gdk_pixbuf_new_from_stream_at_scale:
 * @stream:  a #GInputStream to load the pixbuf from
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: %TRUE to preserve the image's aspect ratio
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @error: Return location for an error
 *
 * Creates a new pixbuf by loading an image from an input stream.  
 *
 * The file format is detected automatically. If %NULL is returned, then 
 * @error will be set. The @cancellable can be used to abort the operation
 * from another thread. If the operation was cancelled, the error 
 * %GIO_ERROR_CANCELLED will be returned. Other possible errors are in 
 * the #GDK_PIXBUF_ERROR and %G_IO_ERROR domains. 
 *
 * The image will be scaled to fit in the requested size, optionally 
 * preserving the image's aspect ratio. When preserving the aspect ratio, 
 * a @width of -1 will cause the image to be scaled to the exact given 
 * height, and a @height of -1 will cause the image to be scaled to the 
 * exact given width. When not preserving aspect ratio, a @width or 
 * @height of -1 means to not scale the image at all in that dimension.
 *
 * The stream is not closed.
 *
 * Return value: A newly-created pixbuf, or %NULL if any of several error 
 * conditions occurred: the file could not be opened, the image format is 
 * not supported, there was not enough memory to allocate the image buffer, 
 * the stream contained invalid data, or the operation was cancelled.
 *
 * Since: 2.14
 */
GdkPixbuf *
gdk_pixbuf_new_from_stream_at_scale (GInputStream  *stream,
                                     gint           width,
                                     gint           height,
                                     gboolean       preserve_aspect_ratio,
                                     GCancellable  *cancellable,
                                     GError       **error)
{
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf;
        AtScaleData info;

        loader = gdk_pixbuf_loader_new ();

        info.width = width;
        info.height = height;
        info.preserve_aspect_ratio = preserve_aspect_ratio;

        g_signal_connect (loader, "size-prepared", 
                          G_CALLBACK (at_scale_size_prepared_cb), &info);

        pixbuf = load_from_stream (loader, stream, cancellable, error);
        g_object_unref (loader);

        return pixbuf;
}

/**
 * gdk_pixbuf_new_from_stream:
 * @stream:  a #GInputStream to load the pixbuf from
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @error: Return location for an error
 *
 * Creates a new pixbuf by loading an image from an input stream.  
 *
 * The file format is detected automatically. If %NULL is returned, then 
 * @error will be set. The @cancellable can be used to abort the operation
 * from another thread. If the operation was cancelled, the error 
 * %GIO_ERROR_CANCELLED will be returned. Other possible errors are in 
 * the #GDK_PIXBUF_ERROR and %G_IO_ERROR domains. 
 *
 * The stream is not closed.
 *
 * Return value: A newly-created pixbuf, or %NULL if any of several error 
 * conditions occurred: the file could not be opened, the image format is 
 * not supported, there was not enough memory to allocate the image buffer, 
 * the stream contained invalid data, or the operation was cancelled.
 *
 * Since: 2.14
 **/
GdkPixbuf *
gdk_pixbuf_new_from_stream (GInputStream  *stream,
                            GCancellable  *cancellable,
                            GError       **error)
{
        GdkPixbuf *pixbuf;
        GdkPixbufLoader *loader;

        loader = gdk_pixbuf_loader_new ();
        pixbuf = load_from_stream (loader, stream, cancellable, error);
        g_object_unref (loader);

        return pixbuf;
}

static void
info_cb (GdkPixbufLoader *loader, 
         int              width,
         int              height,
         gpointer         data)
{
        struct {
                GdkPixbufFormat *format;
                int width;
                int height;
        } *info = data;

        g_return_if_fail (width > 0 && height > 0);

        info->format = gdk_pixbuf_loader_get_format (loader);
        info->width = width;
        info->height = height;

        gdk_pixbuf_loader_set_size (loader, 0, 0);
}

/**
 * gdk_pixbuf_get_file_info:
 * @filename: The name of the file to identify.
 * @width: Return location for the width of the image, or %NULL
 * @height: Return location for the height of the image, or %NULL
 * 
 * Parses an image file far enough to determine its format and size.
 * 
 * Returns: A #GdkPixbufFormat describing the image format of the file 
 *    or %NULL if the image format wasn't recognized. The return value 
 *    is owned by GdkPixbuf and should not be freed.
 *
 * Since: 2.4
 **/
GdkPixbufFormat *
gdk_pixbuf_get_file_info (const gchar  *filename,
                          gint         *width, 
                          gint         *height)
{
        GdkPixbufLoader *loader;
        guchar buffer[SNIFF_BUFFER_SIZE];
        int length;
        FILE *f;
        struct {
                GdkPixbufFormat *format;
                gint width;
                gint height;
        } info;

        g_return_val_if_fail (filename != NULL, NULL);

        f = g_fopen (filename, "rb");
        if (!f)
                return NULL;

        loader = gdk_pixbuf_loader_new ();

        info.format = NULL;
        info.width = -1;
        info.height = -1;
                
        g_signal_connect (loader, "size-prepared", G_CALLBACK (info_cb), &info);

        while (!feof (f) && !ferror (f)) {
                length = fread (buffer, 1, sizeof (buffer), f);
                if (length > 0) {
                        if (!gdk_pixbuf_loader_write (loader, buffer, length, NULL))
                                break;
                }
                if (info.format != NULL)
                        break;
        }

        fclose (f);
        gdk_pixbuf_loader_close (loader, NULL);
        g_object_unref (loader);

        if (width) 
                *width = info.width;
        if (height) 
                *height = info.height;

        return info.format;
}

/**
 * gdk_pixbuf_new_from_xpm_data:
 * @data: Pointer to inline XPM data.
 *
 * Creates a new pixbuf by parsing XPM data in memory.  This data is commonly
 * the result of including an XPM file into a program's C source.
 *
 * Return value: A newly-created pixbuf with a reference count of 1.
 **/
GdkPixbuf *
gdk_pixbuf_new_from_xpm_data (const char **data)
{
        GdkPixbuf *(* load_xpm_data) (const char **data);
        GdkPixbuf *pixbuf;
        GError *error = NULL;
        GdkPixbufModule *xpm_module;
        gboolean locked;

        g_return_val_if_fail (data != NULL, NULL);

        xpm_module = _gdk_pixbuf_get_named_module ("xpm", &error);
        if (xpm_module == NULL) {
                g_warning ("Error loading XPM image loader: %s", error->message);
                g_error_free (error);
                return NULL;
        }

        if (!_gdk_pixbuf_load_module (xpm_module, &error)) {
                g_warning ("Error loading XPM image loader: %s", error->message);
                g_error_free (error);
                return NULL;
        }

        locked = _gdk_pixbuf_lock (xpm_module);

        if (xpm_module->load_xpm_data == NULL) {
                g_warning ("gdk-pixbuf XPM module lacks XPM data capability");
                pixbuf = NULL;
        } else {
                load_xpm_data = xpm_module->load_xpm_data;
                pixbuf = (* load_xpm_data) (data);
        }
        
        if (locked)
                _gdk_pixbuf_unlock (xpm_module);
        return pixbuf;
}

static void
collect_save_options (va_list   opts,
                      gchar  ***keys,
                      gchar  ***vals)
{
  gchar *key;
  gchar *val;
  gchar *next;
  gint count;

  count = 0;
  *keys = NULL;
  *vals = NULL;
  
  next = va_arg (opts, gchar*);
  while (next)
    {
      key = next;
      val = va_arg (opts, gchar*);

      ++count;

      /* woo, slow */
      *keys = g_realloc (*keys, sizeof(gchar*) * (count + 1));
      *vals = g_realloc (*vals, sizeof(gchar*) * (count + 1));
      
      (*keys)[count-1] = g_strdup (key);
      (*vals)[count-1] = g_strdup (val);

      (*keys)[count] = NULL;
      (*vals)[count] = NULL;
      
      next = va_arg (opts, gchar*);
    }
}

static gboolean
save_to_file_callback (const gchar *buf,
                       gsize        count,
                       GError     **error,
                       gpointer     data)
{
        FILE *filehandle = data;
        gsize n;

        n = fwrite (buf, 1, count, filehandle);
        if (n != count) {
                gint save_errno = errno;
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (save_errno),
                             _("Error writing to image file: %s"),
                             g_strerror (save_errno));
                return FALSE;
        }
        return TRUE;
}

static gboolean
gdk_pixbuf_real_save (GdkPixbuf     *pixbuf, 
                      FILE          *filehandle, 
                      const char    *type, 
                      gchar        **keys,
                      gchar        **values,
                      GError       **error)
{
        gboolean ret;
        GdkPixbufModule *image_module = NULL;       
        gboolean locked;

        image_module = _gdk_pixbuf_get_named_module (type, error);

        if (image_module == NULL)
                return FALSE;
       
        if (!_gdk_pixbuf_load_module (image_module, error))
                return FALSE;

        locked = _gdk_pixbuf_lock (image_module);

        if (image_module->save) {
                /* save normally */
                ret = (* image_module->save) (filehandle, pixbuf,
                                              keys, values,
                                              error);
        } else if (image_module->save_to_callback) {
                /* save with simple callback */
                ret = (* image_module->save_to_callback) (save_to_file_callback,
                                                          filehandle, pixbuf,
                                                          keys, values,
                                                          error);
        } else {
                /* can't save */
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION,
                             _("This build of gdk-pixbuf does not support saving the image format: %s"),
                             type);
                ret = FALSE;
        }

        if (locked)
                _gdk_pixbuf_unlock (image_module);
        return ret;
}

#define TMP_FILE_BUF_SIZE 4096

static gboolean
save_to_callback_with_tmp_file (GdkPixbufModule   *image_module,
                                GdkPixbuf         *pixbuf,
                                GdkPixbufSaveFunc  save_func,
                                gpointer           user_data,
                                gchar            **keys,
                                gchar            **values,
                                GError           **error)
{
        int fd;
        FILE *f = NULL;
        gboolean retval = FALSE;
        gchar *buf = NULL;
        gsize n;
        gchar *filename = NULL;
        gboolean locked;

        buf = g_try_malloc (TMP_FILE_BUF_SIZE);
        if (buf == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Insufficient memory to save image to callback"));
                goto end;
        }

        fd = g_file_open_tmp ("gdkpixbuf-save-tmp.XXXXXX", &filename, error);
        if (fd == -1)
                goto end;
        f = fdopen (fd, "wb+");
        if (f == NULL) {
                gint save_errno = errno;
                g_set_error_literal (error,
                                     G_FILE_ERROR,
                                     g_file_error_from_errno (save_errno),
                                     _("Failed to open temporary file"));
                goto end;
        }

        locked = _gdk_pixbuf_lock (image_module);
        retval = (image_module->save) (f, pixbuf, keys, values, error);
        if (locked)
                _gdk_pixbuf_unlock (image_module);
        if (!retval)
                goto end;

        rewind (f);
        for (;;) {
                n = fread (buf, 1, TMP_FILE_BUF_SIZE, f);
                if (n > 0) {
                        if (!save_func (buf, n, error, user_data))
                                goto end;
                }
                if (n != TMP_FILE_BUF_SIZE) 
                        break;
        }
        if (ferror (f)) {
                gint save_errno = errno;
                g_set_error_literal (error,
                                     G_FILE_ERROR,
                                     g_file_error_from_errno (save_errno),
                                     _("Failed to read from temporary file"));
                goto end;
        }
        retval = TRUE;

 end:
        /* cleanup and return retval */
        if (f)
                fclose (f);
        if (filename) {
                g_unlink (filename);
                g_free (filename);
        }
        g_free (buf);

        return retval;
}

static gboolean
gdk_pixbuf_real_save_to_callback (GdkPixbuf         *pixbuf,
                                  GdkPixbufSaveFunc  save_func,
                                  gpointer           user_data,
                                  const char        *type, 
                                  gchar            **keys,
                                  gchar            **values,
                                  GError           **error)
{
        gboolean ret;
        GdkPixbufModule *image_module = NULL;       
        gboolean locked;

        image_module = _gdk_pixbuf_get_named_module (type, error);

        if (image_module == NULL)
                return FALSE;
       
        if (!_gdk_pixbuf_load_module (image_module, error))
                return FALSE;

        locked = _gdk_pixbuf_lock (image_module);

        if (image_module->save_to_callback) {
                /* save normally */
                ret = (* image_module->save_to_callback) (save_func, user_data, 
                                                          pixbuf, keys, values,
                                                          error);
        } else if (image_module->save) {
                /* use a temporary file */
                ret = save_to_callback_with_tmp_file (image_module, pixbuf,
                                                      save_func, user_data, 
                                                      keys, values,
                                                      error);
        } else {
                /* can't save */
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION,
                             _("This build of gdk-pixbuf does not support saving the image format: %s"),
                             type);
                ret = FALSE;
        }

        if (locked)
                _gdk_pixbuf_unlock (image_module);
        return ret;
}

 
/**
 * gdk_pixbuf_save:
 * @pixbuf: a #GdkPixbuf.
 * @filename: name of file to save.
 * @type: name of file format.
 * @error: (allow-none): return location for error, or %NULL
 * @Varargs: list of key-value save options
 *
 * Saves pixbuf to a file in format @type. By default, "jpeg", "png", "ico" 
 * and "bmp" are possible file formats to save in, but more formats may be
 * installed. The list of all writable formats can be determined in the 
 * following way:
 *
 * |[
 * void add_if_writable (GdkPixbufFormat *data, GSList **list)
 * {
 *   if (gdk_pixbuf_format_is_writable (data))
 *     *list = g_slist_prepend (*list, data);
 * }
 * 
 * GSList *formats = gdk_pixbuf_get_formats ();
 * GSList *writable_formats = NULL;
 * g_slist_foreach (formats, add_if_writable, &writable_formats);
 * g_slist_free (formats);
 * ]|
 *
 * If @error is set, %FALSE will be returned. Possible errors include 
 * those in the #GDK_PIXBUF_ERROR domain and those in the #G_FILE_ERROR domain.
 *
 * The variable argument list should be %NULL-terminated; if not empty,
 * it should contain pairs of strings that modify the save
 * parameters. For example:
 * <informalexample><programlisting>
 * gdk_pixbuf_save (pixbuf, handle, "jpeg", &amp;error,
 *                  "quality", "100", NULL);
 * </programlisting></informalexample>
 *
 * Currently only few parameters exist. JPEG images can be saved with a
 * "quality" parameter; its value should be in the range [0,100].
 *
 * Text chunks can be attached to PNG images by specifying parameters of
 * the form "tEXt::key", where key is an ASCII string of length 1-79.
 * The values are UTF-8 encoded strings. The PNG compression level can
 * be specified using the "compression" parameter; it's value is in an
 * integer in the range of [0,9].
 *
 * ICC color profiles can also be embedded into PNG and TIFF images.
 * The "icc-profile" value should be the complete ICC profile encoded
 * into base64.
 *
 * <informalexample><programlisting>
 * gchar *contents;
 * gchar *contents_encode;
 * gsize length;
 * g_file_get_contents ("/home/hughsie/.color/icc/L225W.icm", &contents, &length, NULL);
 * contents_encode = g_base64_encode ((const guchar *) contents, length);
 * gdk_pixbuf_save (pixbuf, handle, "png", &amp;error,
 *                  "icc-profile", contents_encode,
 *                  NULL);
 * </programlisting></informalexample>
 *
 * TIFF images recognize a "compression" option which acceps an integer value.
 * Among the codecs are 1 None, 2 Huffman, 5 LZW, 7 JPEG and 8 Deflate, see
 * the libtiff documentation and tiff.h for all supported codec values.
 *
 * ICO images can be saved in depth 16, 24, or 32, by using the "depth"
 * parameter. When the ICO saver is given "x_hot" and "y_hot" parameters,
 * it produces a CUR instead of an ICO.
 *
 * Return value: whether an error was set
 **/

gboolean
gdk_pixbuf_save (GdkPixbuf  *pixbuf, 
                 const char *filename, 
                 const char *type, 
                 GError    **error,
                 ...)
{
        gchar **keys = NULL;
        gchar **values = NULL;
        va_list args;
        gboolean result;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
        
        va_start (args, error);
        
        collect_save_options (args, &keys, &values);
        
        va_end (args);

        result = gdk_pixbuf_savev (pixbuf, filename, type,
                                   keys, values,
                                   error);

        g_strfreev (keys);
        g_strfreev (values);

        return result;
}

#ifdef G_OS_WIN32

#undef gdk_pixbuf_save

gboolean
gdk_pixbuf_save (GdkPixbuf  *pixbuf, 
                 const char *filename, 
                 const char *type, 
                 GError    **error,
                 ...)
{
        char *utf8_filename;
        gchar **keys = NULL;
        gchar **values = NULL;
        va_list args;
        gboolean result;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
        
        utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, error);

        if (utf8_filename == NULL)
                return FALSE;

        va_start (args, error);
        
        collect_save_options (args, &keys, &values);
        
        va_end (args);

        result = gdk_pixbuf_savev_utf8 (pixbuf, utf8_filename, type,
                                        keys, values,
                                        error);

        g_free (utf8_filename);

        g_strfreev (keys);
        g_strfreev (values);

        return result;
}

#endif

/**
 * gdk_pixbuf_savev:
 * @pixbuf: a #GdkPixbuf.
 * @filename: name of file to save.
 * @type: name of file format.
 * @option_keys: name of options to set, %NULL-terminated
 * @option_values: values for named options
 * @error: (allow-none): return location for error, or %NULL
 *
 * Saves pixbuf to a file in @type, which is currently "jpeg", "png", "tiff", "ico" or "bmp".
 * If @error is set, %FALSE will be returned. 
 * See gdk_pixbuf_save () for more details.
 *
 * Return value: whether an error was set
 **/

gboolean
gdk_pixbuf_savev (GdkPixbuf  *pixbuf, 
                  const char *filename, 
                  const char *type,
                  char      **option_keys,
                  char      **option_values,
                  GError    **error)
{
        FILE *f = NULL;
        gboolean result;
       
        g_return_val_if_fail (filename != NULL, FALSE);
        g_return_val_if_fail (type != NULL, FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
       
        f = g_fopen (filename, "wb");
        
        if (f == NULL) {
                gint save_errno = errno;
                gchar *display_name = g_filename_display_name (filename);
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (save_errno),
                             _("Failed to open '%s' for writing: %s"),
                             display_name,
                             g_strerror (save_errno));
                g_free (display_name);
                return FALSE;
        }

       
       result = gdk_pixbuf_real_save (pixbuf, f, type,
                                      option_keys, option_values,
                                      error);
       
       
       if (!result) {
               g_return_val_if_fail (error == NULL || *error != NULL, FALSE);
               fclose (f);
               g_unlink (filename);
               return FALSE;
       }

       if (fclose (f) < 0) {
               gint save_errno = errno;
               gchar *display_name = g_filename_display_name (filename);
               g_set_error (error,
                            G_FILE_ERROR,
                            g_file_error_from_errno (save_errno),
                            _("Failed to close '%s' while writing image, all data may not have been saved: %s"),
                            display_name,
                            g_strerror (save_errno));
               g_free (display_name);
               return FALSE;
       }
       
       return TRUE;
}

#ifdef G_OS_WIN32

#undef gdk_pixbuf_savev

gboolean
gdk_pixbuf_savev (GdkPixbuf  *pixbuf, 
                  const char *filename, 
                  const char *type,
                  char      **option_keys,
                  char      **option_values,
                  GError    **error)
{
        char *utf8_filename;
        gboolean retval;

        g_return_val_if_fail (filename != NULL, FALSE);
       
        utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, error);

        if (utf8_filename == NULL)
                return FALSE;

        retval = gdk_pixbuf_savev_utf8 (pixbuf, utf8_filename, type,
                                        option_keys, option_values, error);

        g_free (utf8_filename);

        return retval;
}

#endif

/**
 * gdk_pixbuf_save_to_callback:
 * @pixbuf: a #GdkPixbuf.
 * @save_func: (scope call): a function that is called to save each block of data that
 *   the save routine generates.
 * @user_data: user data to pass to the save function.
 * @type: name of file format.
 * @error: (allow-none): return location for error, or %NULL
 * @Varargs: list of key-value save options
 *
 * Saves pixbuf in format @type by feeding the produced data to a 
 * callback. Can be used when you want to store the image to something 
 * other than a file, such as an in-memory buffer or a socket.  
 * If @error is set, %FALSE will be returned. Possible errors
 * include those in the #GDK_PIXBUF_ERROR domain and whatever the save
 * function generates.
 *
 * See gdk_pixbuf_save() for more details.
 *
 * Return value: whether an error was set
 *
 * Since: 2.4
 **/
gboolean
gdk_pixbuf_save_to_callback    (GdkPixbuf  *pixbuf,
                                GdkPixbufSaveFunc save_func,
                                gpointer user_data,
                                const char *type, 
                                GError    **error,
                                ...)
{
        gchar **keys = NULL;
        gchar **values = NULL;
        va_list args;
        gboolean result;
        
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
        
        va_start (args, error);
        
        collect_save_options (args, &keys, &values);
        
        va_end (args);

        result = gdk_pixbuf_save_to_callbackv (pixbuf, save_func, user_data,
                                               type, keys, values,
                                               error);

        g_strfreev (keys);
        g_strfreev (values);

        return result;
}

/**
 * gdk_pixbuf_save_to_callbackv:
 * @pixbuf: a #GdkPixbuf.
 * @save_func: (scope call): a function that is called to save each block of data that
 *   the save routine generates.
 * @user_data: user data to pass to the save function.
 * @type: name of file format.
 * @option_keys: name of options to set, %NULL-terminated
 * @option_values: values for named options
 * @error: (allow-none): return location for error, or %NULL
 *
 * Saves pixbuf to a callback in format @type, which is currently "jpeg",
 * "png", "tiff", "ico" or "bmp".  If @error is set, %FALSE will be returned. See
 * gdk_pixbuf_save_to_callback () for more details.
 *
 * Return value: whether an error was set
 *
 * Since: 2.4
 **/
gboolean
gdk_pixbuf_save_to_callbackv   (GdkPixbuf  *pixbuf, 
                                GdkPixbufSaveFunc save_func,
                                gpointer user_data,
                                const char *type,
                                char      **option_keys,
                                char      **option_values,
                                GError    **error)
{
        gboolean result;
        
       
        g_return_val_if_fail (save_func != NULL, FALSE);
        g_return_val_if_fail (type != NULL, FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
       
       result = gdk_pixbuf_real_save_to_callback (pixbuf,
                                                  save_func, user_data, type,
                                                  option_keys, option_values,
                                                  error);
       
       if (!result) {
               g_return_val_if_fail (error == NULL || *error != NULL, FALSE);
               return FALSE;
       }

       return TRUE;
}

/**
 * gdk_pixbuf_save_to_buffer:
 * @pixbuf: a #GdkPixbuf.
 * @buffer: location to receive a pointer to the new buffer.
 * @buffer_size: location to receive the size of the new buffer.
 * @type: name of file format.
 * @error: (allow-none): return location for error, or %NULL
 * @Varargs: list of key-value save options
 *
 * Saves pixbuf to a new buffer in format @type, which is currently "jpeg",
 * "png", "tiff", "ico" or "bmp".  This is a convenience function that uses
 * gdk_pixbuf_save_to_callback() to do the real work. Note that the buffer 
 * is not nul-terminated and may contain embedded  nuls.
 * If @error is set, %FALSE will be returned and @buffer will be set to
 * %NULL. Possible errors include those in the #GDK_PIXBUF_ERROR
 * domain.
 *
 * See gdk_pixbuf_save() for more details.
 *
 * Return value: whether an error was set
 *
 * Since: 2.4
 **/
gboolean
gdk_pixbuf_save_to_buffer      (GdkPixbuf  *pixbuf,
                                gchar     **buffer,
                                gsize      *buffer_size,
                                const char *type, 
                                GError    **error,
                                ...)
{
        gchar **keys = NULL;
        gchar **values = NULL;
        va_list args;
        gboolean result;
        
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
        
        va_start (args, error);
        
        collect_save_options (args, &keys, &values);
        
        va_end (args);

        result = gdk_pixbuf_save_to_bufferv (pixbuf, buffer, buffer_size,
                                             type, keys, values,
                                             error);

        g_strfreev (keys);
        g_strfreev (values);

        return result;
}

struct SaveToBufferData {
        gchar *buffer;
        gsize len, max;
};

static gboolean
save_to_buffer_callback (const gchar *data,
                         gsize count,
                         GError **error,
                         gpointer user_data)
{
        struct SaveToBufferData *sdata = user_data;
        gchar *new_buffer;
        gsize new_max;

        if (sdata->len + count > sdata->max) {
                new_max = MAX (sdata->max*2, sdata->len + count);
                new_buffer = g_try_realloc (sdata->buffer, new_max);
                if (!new_buffer) {
                        g_set_error_literal (error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                             _("Insufficient memory to save image into a buffer"));
                        return FALSE;
                }
                sdata->buffer = new_buffer;
                sdata->max = new_max;
        }
        memcpy (sdata->buffer + sdata->len, data, count);
        sdata->len += count;
        return TRUE;
}

/**
 * gdk_pixbuf_save_to_bufferv:
 * @pixbuf: a #GdkPixbuf.
 * @buffer: location to receive a pointer to the new buffer.
 * @buffer_size: location to receive the size of the new buffer.
 * @type: name of file format.
 * @option_keys: name of options to set, %NULL-terminated
 * @option_values: values for named options
 * @error: (allow-none): return location for error, or %NULL
 *
 * Saves pixbuf to a new buffer in format @type, which is currently "jpeg",
 * "tiff", "png", "ico" or "bmp".  See gdk_pixbuf_save_to_buffer() 
 * for more details.
 *
 * Return value: whether an error was set
 *
 * Since: 2.4
 **/
gboolean
gdk_pixbuf_save_to_bufferv     (GdkPixbuf  *pixbuf,
                                gchar     **buffer,
                                gsize      *buffer_size,
                                const char *type, 
                                char      **option_keys,
                                char      **option_values,
                                GError    **error)
{
        static const gint initial_max = 1024;
        struct SaveToBufferData sdata;

        *buffer = NULL;
        *buffer_size = 0;

        sdata.buffer = g_try_malloc (initial_max);
        sdata.max = initial_max;
        sdata.len = 0;
        if (!sdata.buffer) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Insufficient memory to save image into a buffer"));
                return FALSE;
        }

        if (!gdk_pixbuf_save_to_callbackv (pixbuf,
                                           save_to_buffer_callback, &sdata,
                                           type, option_keys, option_values,
                                           error)) {
                g_free (sdata.buffer);
                return FALSE;
        }

        *buffer = sdata.buffer;
        *buffer_size = sdata.len;
        return TRUE;
}

typedef struct {
        GOutputStream *stream;
        GCancellable  *cancellable;
} SaveToStreamData;

static gboolean
save_to_stream (const gchar  *buffer,
                gsize         count,
                GError      **error,
                gpointer      data)
{
        SaveToStreamData *sdata = (SaveToStreamData *)data;
        gsize remaining;
        gssize written;
        GError *my_error = NULL;

        remaining = count;
        written = 0;
        while (remaining > 0) {
                buffer += written;
                remaining -= written;
                written = g_output_stream_write (sdata->stream, 
                                                 buffer, remaining, 
                                                 sdata->cancellable, 
                                                 &my_error);
                if (written < 0) {
                        if (!my_error) {
                                g_set_error_literal (error,
                                                     G_IO_ERROR, 0,
                                                     _("Error writing to image stream"));
                        }
                        else {
                                g_propagate_error (error, my_error);
                        }
                        return FALSE;
                }
        }

        return TRUE;
}

/** 
 * gdk_pixbuf_save_to_stream:
 * @pixbuf: a #GdkPixbuf
 * @stream: a #GOutputStream to save the pixbuf to
 * @type: name of file format
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @error: (allow-none): return location for error, or %NULL
 * @Varargs: list of key-value save options
 *
 * Saves @pixbuf to an output stream.
 *
 * Supported file formats are currently "jpeg", "tiff", "png", "ico" or 
 * "bmp". See gdk_pixbuf_save_to_buffer() for more details.
 *
 * The @cancellable can be used to abort the operation from another 
 * thread. If the operation was cancelled, the error %GIO_ERROR_CANCELLED 
 * will be returned. Other possible errors are in the #GDK_PIXBUF_ERROR 
 * and %G_IO_ERROR domains. 
 *
 * The stream is not closed.
 *
 * Returns: %TRUE if the pixbuf was saved successfully, %FALSE if an
 *     error was set.
 *
 * Since: 2.14
 */
gboolean
gdk_pixbuf_save_to_stream (GdkPixbuf      *pixbuf,
                           GOutputStream  *stream,
                           const char     *type,
                           GCancellable   *cancellable,
                           GError        **error,
                           ...)
{
        gboolean res;
        gchar **keys = NULL;
        gchar **values = NULL;
        va_list args;
        SaveToStreamData data;

        va_start (args, error);
        collect_save_options (args, &keys, &values);
        va_end (args);

        data.stream = stream;
        data.cancellable = cancellable;

        res = gdk_pixbuf_save_to_callbackv (pixbuf, save_to_stream, 
                                            &data, type, 
                                            keys, values, 
                                            error);

        g_strfreev (keys);
        g_strfreev (values);

        return res;
}

/**
 * gdk_pixbuf_format_get_name:
 * @format: a #GdkPixbufFormat
 *
 * Returns the name of the format.
 * 
 * Return value: the name of the format. 
 *
 * Since: 2.2
 */
gchar *
gdk_pixbuf_format_get_name (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, NULL);

        return g_strdup (format->name);
}

/**
 * gdk_pixbuf_format_get_description:
 * @format: a #GdkPixbufFormat
 *
 * Returns a description of the format.
 * 
 * Return value: a description of the format.
 *
 * Since: 2.2
 */
gchar *
gdk_pixbuf_format_get_description (GdkPixbufFormat *format)
{
        gchar *domain;
        const gchar *description;
        g_return_val_if_fail (format != NULL, NULL);

        if (format->domain != NULL) 
                domain = format->domain;
        else 
                domain = GETTEXT_PACKAGE;
        description = g_dgettext (domain, format->description);

        return g_strdup (description);
}

/**
 * gdk_pixbuf_format_get_mime_types:
 * @format: a #GdkPixbufFormat
 *
 * Returns the mime types supported by the format.
 * 
 * Return value: (transfer full): a %NULL-terminated array of mime types which must be freed with 
 * g_strfreev() when it is no longer needed.
 *
 * Since: 2.2
 */
gchar **
gdk_pixbuf_format_get_mime_types (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, NULL);

        return g_strdupv (format->mime_types);
}

/**
 * gdk_pixbuf_format_get_extensions:
 * @format: a #GdkPixbufFormat
 *
 * Returns the filename extensions typically used for files in the 
 * given format.
 * 
 * Return value: (transfer full): a %NULL-terminated array of filename extensions which must be
 * freed with g_strfreev() when it is no longer needed.
 *
 * Since: 2.2
 */
gchar **
gdk_pixbuf_format_get_extensions (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, NULL);

        return g_strdupv (format->extensions);
}

/**
 * gdk_pixbuf_format_is_writable:
 * @format: a #GdkPixbufFormat
 *
 * Returns whether pixbufs can be saved in the given format.
 * 
 * Return value: whether pixbufs can be saved in the given format.
 *
 * Since: 2.2
 */
gboolean
gdk_pixbuf_format_is_writable (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, FALSE);

        return (format->flags & GDK_PIXBUF_FORMAT_WRITABLE) != 0;
}

/**
 * gdk_pixbuf_format_is_scalable:
 * @format: a #GdkPixbufFormat
 *
 * Returns whether this image format is scalable. If a file is in a 
 * scalable format, it is preferable to load it at the desired size, 
 * rather than loading it at the default size and scaling the 
 * resulting pixbuf to the desired size.
 * 
 * Return value: whether this image format is scalable.
 *
 * Since: 2.6
 */
gboolean
gdk_pixbuf_format_is_scalable (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, FALSE);

        return (format->flags & GDK_PIXBUF_FORMAT_SCALABLE) != 0;
}

/**
 * gdk_pixbuf_format_is_disabled:
 * @format: a #GdkPixbufFormat
 *
 * Returns whether this image format is disabled. See
 * gdk_pixbuf_format_set_disabled().
 * 
 * Return value: whether this image format is disabled.
 *
 * Since: 2.6
 */
gboolean   
gdk_pixbuf_format_is_disabled (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, FALSE);

        return format->disabled;        
}

/**
 * gdk_pixbuf_format_set_disabled:
 * @format: a #GdkPixbufFormat
 * @disabled: %TRUE to disable the format @format
 *
 * Disables or enables an image format. If a format is disabled, 
 * gdk-pixbuf won't use the image loader for this format to load 
 * images. Applications can use this to avoid using image loaders 
 * with an inappropriate license, see gdk_pixbuf_format_get_license().
 *
 * Since: 2.6
 */
void 
gdk_pixbuf_format_set_disabled (GdkPixbufFormat *format,
                                gboolean         disabled)
{
        g_return_if_fail (format != NULL);
        
        format->disabled = disabled != FALSE;
}

/**
 * gdk_pixbuf_format_get_license:
 * @format: a #GdkPixbufFormat
 *
 * Returns information about the license of the image loader for the format. The
 * returned string should be a shorthand for a wellknown license, e.g. "LGPL",
 * "GPL", "QPL", "GPL/QPL", or "other" to indicate some other license.  This
 * string should be freed with g_free() when it's no longer needed.
 *
 * Returns: a string describing the license of @format. 
 *
 * Since: 2.6
 */
gchar*
gdk_pixbuf_format_get_license (GdkPixbufFormat *format)
{
        g_return_val_if_fail (format != NULL, NULL);

        return g_strdup (format->license);
}

GdkPixbufFormat *
_gdk_pixbuf_get_format (GdkPixbufModule *module)
{
        g_return_val_if_fail (module != NULL, NULL);

        return module->info;
}

/**
 * gdk_pixbuf_get_formats:
 *
 * Obtains the available information about the image formats supported
 * by GdkPixbuf.
 *
 * Returns: (transfer container): (element-type GdkPixbufFormat): A list of
 * #GdkPixbufFormat<!-- -->s describing the supported
 * image formats.  The list should be freed when it is no longer needed,
 * but the structures themselves are owned by #GdkPixbuf and should not be
 * freed.
 *
 * Since: 2.2
 */
GSList *
gdk_pixbuf_get_formats (void)
{
        GSList *result = NULL;
        GSList *modules;

        for (modules = get_file_formats (); modules; modules = g_slist_next (modules)) {
                GdkPixbufModule *module = (GdkPixbufModule *)modules->data;
                GdkPixbufFormat *info = _gdk_pixbuf_get_format (module);
                result = g_slist_prepend (result, info);
        }

        return result;
}

/**
 * gdk_pixbuf_format_copy:
 * @format: a #GdkPixbufFormat
 *
 * Creates a copy of @format
 *
 * Return value: the newly allocated copy of a #GdkPixbufFormat. Use
 *   gdk_pixbuf_format_free() to free the resources when done
 *
 * Since: 2.22
 */
GdkPixbufFormat *
gdk_pixbuf_format_copy (const GdkPixbufFormat *format)
{
        if (G_LIKELY (format != NULL))
                return g_slice_dup (GdkPixbufFormat, format);

        return NULL;
}

/**
 * gdk_pixbuf_format_free:
 * @format: a #GdkPixbufFormat
 *
 * Frees the resources allocated when copying a #GdkPixbufFormat
 * using gdk_pixbuf_format_copy()
 *
 * Since: 2.22
 */
void
gdk_pixbuf_format_free (GdkPixbufFormat *format)
{
        if (G_LIKELY (format != NULL))
                g_slice_free (GdkPixbufFormat, format);
}

GType
gdk_pixbuf_format_get_type (void)
{
        static volatile gsize format_id__volatile = 0;
        if (g_once_init_enter (&format_id__volatile)) {
                GType format_id =
                        g_boxed_type_register_static (g_intern_static_string ("GdkPixbufFormat"),
                                                      (GBoxedCopyFunc) gdk_pixbuf_format_copy,
                                                      (GBoxedFreeFunc) gdk_pixbuf_format_free);
                g_once_init_leave (&format_id__volatile, format_id);
        }
        return format_id__volatile;
}

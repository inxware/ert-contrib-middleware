/* This file is automatically generated. Any changes made here will be lost. */

/* This header defines simple perlapi-ish macros for creating SV wrappers
 * and extracting the GPerl value from SV wrappers.  These macros are used
 * by the autogenerated typemaps, and are defined here so that you can use
 * the same logic anywhere in your code (e.g., if you handle the argument
 * stack by hand instead of using the typemap). */

#ifdef GNOME_TYPE_CANVAS_BPATH
  /* GtkObject derivative GnomeCanvasBpath */
# define SvGnomeCanvasBpath(sv)	((GnomeCanvasBpath*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_BPATH))
# define newSVGnomeCanvasBpath(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasBpath GnomeCanvasBpath_ornull;
# define SvGnomeCanvasBpath_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasBpath(sv) : NULL)
# define newSVGnomeCanvasBpath_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_BPATH */

#ifdef GNOME_TYPE_CANVAS_ITEM
  /* GtkObject derivative GnomeCanvasItem */
# define SvGnomeCanvasItem(sv)	((GnomeCanvasItem*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_ITEM))
# define newSVGnomeCanvasItem(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasItem GnomeCanvasItem_ornull;
# define SvGnomeCanvasItem_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasItem(sv) : NULL)
# define newSVGnomeCanvasItem_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_ITEM */

#ifdef GNOME_TYPE_CANVAS_GROUP
  /* GtkObject derivative GnomeCanvasGroup */
# define SvGnomeCanvasGroup(sv)	((GnomeCanvasGroup*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_GROUP))
# define newSVGnomeCanvasGroup(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasGroup GnomeCanvasGroup_ornull;
# define SvGnomeCanvasGroup_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasGroup(sv) : NULL)
# define newSVGnomeCanvasGroup_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_GROUP */

#ifdef GNOME_TYPE_CANVAS
  /* GtkObject derivative GnomeCanvas */
# define SvGnomeCanvas(sv)	((GnomeCanvas*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS))
# define newSVGnomeCanvas(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvas GnomeCanvas_ornull;
# define SvGnomeCanvas_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvas(sv) : NULL)
# define newSVGnomeCanvas_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS */

#ifdef GNOME_TYPE_CANVAS_LINE
  /* GtkObject derivative GnomeCanvasLine */
# define SvGnomeCanvasLine(sv)	((GnomeCanvasLine*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_LINE))
# define newSVGnomeCanvasLine(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasLine GnomeCanvasLine_ornull;
# define SvGnomeCanvasLine_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasLine(sv) : NULL)
# define newSVGnomeCanvasLine_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_LINE */

#ifdef GNOME_TYPE_CANVAS_PIXBUF
  /* GtkObject derivative GnomeCanvasPixbuf */
# define SvGnomeCanvasPixbuf(sv)	((GnomeCanvasPixbuf*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_PIXBUF))
# define newSVGnomeCanvasPixbuf(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasPixbuf GnomeCanvasPixbuf_ornull;
# define SvGnomeCanvasPixbuf_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasPixbuf(sv) : NULL)
# define newSVGnomeCanvasPixbuf_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_PIXBUF */

#ifdef GNOME_TYPE_CANVAS_POLYGON
  /* GtkObject derivative GnomeCanvasPolygon */
# define SvGnomeCanvasPolygon(sv)	((GnomeCanvasPolygon*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_POLYGON))
# define newSVGnomeCanvasPolygon(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasPolygon GnomeCanvasPolygon_ornull;
# define SvGnomeCanvasPolygon_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasPolygon(sv) : NULL)
# define newSVGnomeCanvasPolygon_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_POLYGON */

#ifdef GNOME_TYPE_CANVAS_RE
  /* GtkObject derivative GnomeCanvasRE */
# define SvGnomeCanvasRE(sv)	((GnomeCanvasRE*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_RE))
# define newSVGnomeCanvasRE(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasRE GnomeCanvasRE_ornull;
# define SvGnomeCanvasRE_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasRE(sv) : NULL)
# define newSVGnomeCanvasRE_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_RE */

#ifdef GNOME_TYPE_CANVAS_RECT
  /* GtkObject derivative GnomeCanvasRect */
# define SvGnomeCanvasRect(sv)	((GnomeCanvasRect*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_RECT))
# define newSVGnomeCanvasRect(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasRect GnomeCanvasRect_ornull;
# define SvGnomeCanvasRect_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasRect(sv) : NULL)
# define newSVGnomeCanvasRect_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_RECT */

#ifdef GNOME_TYPE_CANVAS_ELLIPSE
  /* GtkObject derivative GnomeCanvasEllipse */
# define SvGnomeCanvasEllipse(sv)	((GnomeCanvasEllipse*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_ELLIPSE))
# define newSVGnomeCanvasEllipse(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasEllipse GnomeCanvasEllipse_ornull;
# define SvGnomeCanvasEllipse_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasEllipse(sv) : NULL)
# define newSVGnomeCanvasEllipse_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_ELLIPSE */

#ifdef GNOME_TYPE_CANVAS_RICH_TEXT
  /* GtkObject derivative GnomeCanvasRichText */
# define SvGnomeCanvasRichText(sv)	((GnomeCanvasRichText*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_RICH_TEXT))
# define newSVGnomeCanvasRichText(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasRichText GnomeCanvasRichText_ornull;
# define SvGnomeCanvasRichText_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasRichText(sv) : NULL)
# define newSVGnomeCanvasRichText_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_RICH_TEXT */

#ifdef GNOME_TYPE_CANVAS_SHAPE
  /* GtkObject derivative GnomeCanvasShape */
# define SvGnomeCanvasShape(sv)	((GnomeCanvasShape*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_SHAPE))
# define newSVGnomeCanvasShape(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasShape GnomeCanvasShape_ornull;
# define SvGnomeCanvasShape_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasShape(sv) : NULL)
# define newSVGnomeCanvasShape_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_SHAPE */

#ifdef GNOME_TYPE_CANVAS_TEXT
  /* GtkObject derivative GnomeCanvasText */
# define SvGnomeCanvasText(sv)	((GnomeCanvasText*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_TEXT))
# define newSVGnomeCanvasText(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasText GnomeCanvasText_ornull;
# define SvGnomeCanvasText_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasText(sv) : NULL)
# define newSVGnomeCanvasText_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_TEXT */

#ifdef GNOME_TYPE_CANVAS_WIDGET
  /* GtkObject derivative GnomeCanvasWidget */
# define SvGnomeCanvasWidget(sv)	((GnomeCanvasWidget*)gperl_get_object_check (sv, GNOME_TYPE_CANVAS_WIDGET))
# define newSVGnomeCanvasWidget(val)	(gtk2perl_new_gtkobject (GTK_OBJECT (val)))
  typedef GnomeCanvasWidget GnomeCanvasWidget_ornull;
# define SvGnomeCanvasWidget_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasWidget(sv) : NULL)
# define newSVGnomeCanvasWidget_ornull(val)	(((val) == NULL) ? &PL_sv_undef : gtk2perl_new_gtkobject (GTK_OBJECT (val)))
#endif /* GNOME_TYPE_CANVAS_WIDGET */

#ifdef GNOME_TYPE_CANVAS_POINTS
  /* GBoxed GnomeCanvasPoints */
  typedef GnomeCanvasPoints GnomeCanvasPoints_ornull;
# define SvGnomeCanvasPoints(sv)	((GnomeCanvasPoints *) gperl_get_boxed_check ((sv), GNOME_TYPE_CANVAS_POINTS))
# define SvGnomeCanvasPoints_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasPoints (sv) : NULL)
  typedef GnomeCanvasPoints GnomeCanvasPoints_own;
  typedef GnomeCanvasPoints GnomeCanvasPoints_copy;
  typedef GnomeCanvasPoints GnomeCanvasPoints_own_ornull;
# define newSVGnomeCanvasPoints(val)	(gperl_new_boxed ((gpointer) (val), GNOME_TYPE_CANVAS_POINTS, FALSE))
# define newSVGnomeCanvasPoints_ornull(val)	((val) ? newSVGnomeCanvasPoints(val) : &PL_sv_undef)
# define newSVGnomeCanvasPoints_own(val)	(gperl_new_boxed ((gpointer) (val), GNOME_TYPE_CANVAS_POINTS, TRUE))
# define newSVGnomeCanvasPoints_copy(val)	(gperl_new_boxed_copy ((gpointer) (val), GNOME_TYPE_CANVAS_POINTS))
# define newSVGnomeCanvasPoints_own_ornull(val)	((val) ? newSVGnomeCanvasPoints_own(val) : &PL_sv_undef)
#endif /* GNOME_TYPE_CANVAS_POINTS */

#ifdef GNOME_TYPE_CANVAS_PATH_DEF
  /* GBoxed GnomeCanvasPathDef */
  typedef GnomeCanvasPathDef GnomeCanvasPathDef_ornull;
# define SvGnomeCanvasPathDef(sv)	((GnomeCanvasPathDef *) gperl_get_boxed_check ((sv), GNOME_TYPE_CANVAS_PATH_DEF))
# define SvGnomeCanvasPathDef_ornull(sv)	(gperl_sv_is_defined (sv) ? SvGnomeCanvasPathDef (sv) : NULL)
  typedef GnomeCanvasPathDef GnomeCanvasPathDef_own;
  typedef GnomeCanvasPathDef GnomeCanvasPathDef_copy;
  typedef GnomeCanvasPathDef GnomeCanvasPathDef_own_ornull;
# define newSVGnomeCanvasPathDef(val)	(gperl_new_boxed ((gpointer) (val), GNOME_TYPE_CANVAS_PATH_DEF, FALSE))
# define newSVGnomeCanvasPathDef_ornull(val)	((val) ? newSVGnomeCanvasPathDef(val) : &PL_sv_undef)
# define newSVGnomeCanvasPathDef_own(val)	(gperl_new_boxed ((gpointer) (val), GNOME_TYPE_CANVAS_PATH_DEF, TRUE))
# define newSVGnomeCanvasPathDef_copy(val)	(gperl_new_boxed_copy ((gpointer) (val), GNOME_TYPE_CANVAS_PATH_DEF))
# define newSVGnomeCanvasPathDef_own_ornull(val)	((val) ? newSVGnomeCanvasPathDef_own(val) : &PL_sv_undef)
#endif /* GNOME_TYPE_CANVAS_PATH_DEF */

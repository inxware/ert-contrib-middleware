
#ifndef ____gst_camerabin_marshal_MARSHAL_H__
#define ____gst_camerabin_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:INT,INT,INT,INT (gstcamerabin-marshal.list:4) */
extern void __gst_camerabin_marshal_VOID__INT_INT_INT_INT (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);

/* VOID:INT,INT (gstcamerabin-marshal.list:5) */
extern void __gst_camerabin_marshal_VOID__INT_INT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* BOOLEAN:STRING (gstcamerabin-marshal.list:6) */
extern void __gst_camerabin_marshal_BOOLEAN__STRING (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

G_END_DECLS

#endif /* ____gst_camerabin_marshal_MARSHAL_H__ */


#ifndef __UFO_FILTER_WRITER_H
#define __UFO_FILTER_WRITER_H

#include <glib.h>

#include <ufo/ufo-filter.h>
#include <ufo/ufo-filter-sink.h>

#define UFO_TYPE_FILTER_WRITER             (ufo_filter_writer_get_type())
#define UFO_FILTER_WRITER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_FILTER_WRITER, UfoFilterWriter))
#define UFO_IS_FILTER_WRITER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_FILTER_WRITER))
#define UFO_FILTER_WRITER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_FILTER_WRITER, UfoFilterWriterClass))
#define UFO_IS_FILTER_WRITER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_FILTER_WRITER))
#define UFO_FILTER_WRITER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_FILTER_WRITER, UfoFilterWriterClass))

typedef struct _UfoFilterWriter           UfoFilterWriter;
typedef struct _UfoFilterWriterClass      UfoFilterWriterClass;
typedef struct _UfoFilterWriterPrivate    UfoFilterWriterPrivate;

struct _UfoFilterWriter {
    /*< private >*/
    UfoFilterSink parent_instance;

    UfoFilterWriterPrivate *priv;
};

/**
 * UfoFilterWriterClass:
 *
 * #UfoFilterWriter class
 */
struct _UfoFilterWriterClass {
    /*< private >*/
    UfoFilterSinkClass parent_class;
};

GType ufo_filter_writer_get_type(void);
UfoFilter *ufo_filter_plugin_new(void);

#endif

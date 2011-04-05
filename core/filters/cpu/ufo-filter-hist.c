#include <gmodule.h>
#include "ufo-filter-hist.h"
#include "ufo-filter.h"
#include "ufo-buffer.h"


struct _UfoFilterHistPrivate {
    guint32 num_bins;
    guint32 *bins;
};

GType ufo_filter_hist_get_type(void) G_GNUC_CONST;

/* Inherit from UFO_TYPE_FILTER */
G_DEFINE_TYPE(UfoFilterHist, ufo_filter_hist, UFO_TYPE_FILTER);

#define UFO_FILTER_HIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_FILTER_HIST, UfoFilterHistPrivate))

static void activated(EthosPlugin *plugin)
{
    g_message("Hist plugin activated");
}

static void deactivated(EthosPlugin *plugin)
{
    g_message("Hist plugin deactivated");
}

/* 
 * virtual methods 
 */
static void ufo_filter_hist_process(UfoFilter *self, UfoBuffer *input, UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_FILTER(self));
    g_message("processing data");

    /* call parent */
    UfoFilterHistClass *klass = UFO_FILTER_HIST_GET_CLASS(self);
    UfoFilterClass *parent_class = g_type_class_peek_parent(klass);
    parent_class->process(UFO_FILTER(self), input, output);
}

static void ufo_filter_hist_class_init(UfoFilterHistClass *klass)
{
    UfoFilterClass *filter_class = UFO_FILTER_CLASS(klass);
    EthosPluginClass *plugin_class = ETHOS_PLUGIN_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    filter_class->process = ufo_filter_hist_process;
    plugin_class->activated = activated;
    plugin_class->deactivated = deactivated;

    /* install private data */
    g_type_class_add_private(object_class, sizeof(UfoFilterHistPrivate));
}

static void ufo_filter_hist_init(UfoFilterHist *self)
{
    /* init public fields */

    /* init private fields */
    UfoFilterHistPrivate *priv = UFO_FILTER_HIST_GET_PRIVATE(self);
    priv->num_bins = 256;
    priv->bins = g_malloc(sizeof(guint32) * priv->num_bins);
    for (int i = 0; i < priv->num_bins; i++)
        priv->bins[i] = 0;
}

G_MODULE_EXPORT EthosPlugin *ethos_plugin_register(void)
{
    return g_object_new(UFO_TYPE_FILTER_HIST, NULL);
}
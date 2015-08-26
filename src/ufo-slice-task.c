/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <string.h>
#include "ufo-slice-task.h"


struct _UfoSliceTaskPrivate {
    UfoBuffer *copy;
    gsize size;
    guint current;
    guint last;

    cl_context context;
    cl_command_queue cmd_queue;
    cl_kernel kernel;
    cl_mem in_mem;
    cl_mem out_mem;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoSliceTask, ufo_slice_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_SLICE_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_SLICE_TASK, UfoSliceTaskPrivate))

enum {
    PROP_0,
    N_PROPERTIES
};

UfoNode *
ufo_slice_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_SLICE_TASK, NULL));
}

static void
ufo_slice_task_setup (UfoTask *task,
                      UfoResources *resources,
                      GError **error)
{
    UfoSliceTaskPrivate *priv;
    UfoGpuNode *node;

    priv = UFO_SLICE_TASK_GET_PRIVATE (task);
    node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (task)));

    priv->context = ufo_resources_get_context (resources);
    priv->cmd_queue = ufo_gpu_node_get_cmd_queue (node);

    priv->kernel = ufo_resources_get_kernel (resources, "default.cl", "convert_u16", error);

    if (priv->kernel != NULL)
        UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->kernel));
}

static void
ufo_slice_task_get_requisition (UfoTask *task,
                                UfoBuffer **inputs,
                                UfoRequisition *requisition)
{
    UfoSliceTaskPrivate *priv;
    UfoRequisition in_req;

    ufo_buffer_get_requisition (inputs[0], &in_req);

    requisition->n_dims = 2;
    requisition->dims[0] = in_req.dims[0];
    requisition->dims[1] = in_req.dims[1];

    priv = UFO_SLICE_TASK_GET_PRIVATE (task);
    priv->current = 0;
    priv->last = in_req.dims[2];
    priv->size = in_req.dims[0] * in_req.dims[1] * sizeof(gfloat);
}

static guint
ufo_slice_task_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_slice_task_get_num_dimensions (UfoTask *task,
                                   guint input)
{
    return 3;
}

static UfoTaskMode
ufo_slice_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_REDUCTOR | UFO_TASK_MODE_GPU;
}

#undef NEW
#define NEW
static gboolean
ufo_slice_task_process (UfoTask *task,
                        UfoBuffer **inputs,
                        UfoBuffer *output,
                        UfoRequisition *requisition)
{
    UfoSliceTaskPrivate *priv;

    priv = UFO_SLICE_TASK_GET_PRIVATE (task);
    priv->copy = ufo_buffer_dup (inputs[0]);

#ifndef NEW
    /* Force CPU memory */
    ufo_buffer_get_host_array (priv->copy, NULL);

    /* Move data */
    ufo_buffer_copy (inputs[0], priv->copy);
#else
    if (ufo_buffer_get_location(inputs[0]) == 1) 
    {
        priv->in_mem = ufo_buffer_get_device_array (inputs[0], priv->cmd_queue);
        priv->out_mem = ufo_buffer_get_device_array (output, priv->cmd_queue);
    } 
    else 
    {
        priv->in_mem = NULL;
        /* Force CPU memory */
        ufo_buffer_get_host_array (priv->copy, NULL);
        /* Move data */
        ufo_buffer_copy (inputs[0], priv->copy);
    }
#endif

    return FALSE;
}

static gboolean
ufo_slice_task_generate (UfoTask *task,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    UfoSliceTaskPrivate *priv;
    gfloat *src;
    gfloat *dst;

    priv = UFO_SLICE_TASK_GET_PRIVATE (task);

    if (priv->current == priv->last) {
        priv->current = 0;
        return FALSE;
    }
#ifndef NEW
    src = ufo_buffer_get_host_array (priv->copy, NULL);
    dst = ufo_buffer_get_host_array (output, NULL);
    memcpy (dst, src + priv->current * priv->size / sizeof(gfloat), priv->size);
    priv->current++;
#else
    if (priv->in_mem) {
        cl_mem out_mem = ufo_buffer_get_device_array (output, priv->cmd_queue);

        UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->kernel, 0, sizeof (cl_mem), &priv->in_mem));
        UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->kernel, 1, sizeof (cl_mem), &out_mem));

        UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->cmd_queue,
                                                           priv->kernel,
                                                           2, NULL, requisition->dims, NULL,
                                                           0, NULL, NULL));
    }
    else
    {
        src = ufo_buffer_get_host_array (priv->copy, NULL);
        dst = ufo_buffer_get_host_array (output, NULL);
        memcpy (dst, src + priv->current * priv->size / sizeof(gfloat), priv->size);
    }
    priv->current++;
#endif

    return TRUE;
}


static void
ufo_slice_task_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
ufo_slice_task_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
ufo_slice_task_finalize (GObject *object)
{
    UfoSliceTaskPrivate *priv;

    priv = UFO_SLICE_TASK_GET_PRIVATE (object);

    if (priv->kernel) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->kernel));
        priv->kernel = NULL;
    }

    G_OBJECT_CLASS (ufo_slice_task_parent_class)->finalize (object);
}

static void
ufo_slice_task_dispose (GObject *object)
{
    UfoSliceTaskPrivate *priv;

    priv = UFO_SLICE_TASK_GET_PRIVATE (object);

    if (priv->copy) {
        g_object_unref (priv->copy);
        priv->copy = NULL;
    }

    G_OBJECT_CLASS (ufo_slice_task_parent_class)->dispose (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_slice_task_setup;
    iface->get_num_inputs = ufo_slice_task_get_num_inputs;
    iface->get_num_dimensions = ufo_slice_task_get_num_dimensions;
    iface->get_mode = ufo_slice_task_get_mode;
    iface->get_requisition = ufo_slice_task_get_requisition;
    iface->process = ufo_slice_task_process;
    iface->generate = ufo_slice_task_generate;
}

static void
ufo_slice_task_class_init (UfoSliceTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_slice_task_set_property;
    gobject_class->get_property = ufo_slice_task_get_property;
    gobject_class->dispose = ufo_slice_task_dispose;
    gobject_class->finalize = ufo_slice_task_finalize;

    g_type_class_add_private (gobject_class, sizeof(UfoSliceTaskPrivate));
}

static void
ufo_slice_task_init(UfoSliceTask *self)
{
    self->priv = UFO_SLICE_TASK_GET_PRIVATE(self);
    self->priv->copy = NULL;
    self->priv->current = 0;
}

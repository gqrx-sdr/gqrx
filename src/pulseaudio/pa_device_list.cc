/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include "pa_device_list.h"

pa_device::pa_device(unsigned int idx, string name, string desc) :
    d_index(idx), d_name(name), d_description(desc)
{
}

pa_device::~pa_device()
{
}


pa_device_list::pa_device_list()
{
    populate_device_list();
}

pa_device_list::~pa_device_list()
{
    d_sources.clear();
    d_sinks.clear();
}

/** \brief Populate pulseaudio device list.
  *
  * This method iterates through all the input and output decives
  * and stores them for later retrieval using getInputDevices() and
  * getOutputDevices().
  */
int pa_device_list::populate_device_list()
{
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_operation *pa_op;
    pa_context *pa_ctx;

    // State variables to keep track of our requests
    int state = 0;
    int pa_ready = 0;

    // Create a mainloop API and connection to the default server
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "test");

    // Connect to the pulseaudio server
    pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS , NULL);

    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);


    // Now we'll enter into an infinite loop until we get the data we receive
    // or if there's an error
    for (;;) {
        // We can't do anything until PA is ready, so just iterate the mainloop
        // and continue
        if (pa_ready == 0) {
            pa_mainloop_iterate(pa_ml, 1, NULL);
            continue;
        }
        // We couldn't get a connection to the server, so exit out
        if (pa_ready == 2) {
            pa_context_disconnect(pa_ctx);
            pa_context_unref(pa_ctx);
            pa_mainloop_free(pa_ml);
            return -1;
        }
        // At this point, we're connected to the server and ready to make
        // requests
        switch (state) {
            // State 0: we haven't done anything yet
            case 0:
                // This sends an operation to the server.  pa_sinklist_info is
                // our callback function and a pointer to our devicelist will
                // be passed to the callback The operation ID is stored in the
                // pa_op variable
                pa_op = pa_context_get_sink_info_list(pa_ctx,
                                                      pa_sinklist_cb,
                                                      this);

                // Update state for next iteration through the loop
                state++;
                break;
            case 1:
                // Now we wait for our operation to complete.  When it's
                // complete our pa_output_devicelist is filled out, and we move
                // along to the next state
                if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                    pa_operation_unref(pa_op);

                    // Now we perform another operation to get the source
                    // (input device) list just like before.  This time we pass
                    // a pointer to our input structure
                    pa_op = pa_context_get_source_info_list(pa_ctx,
                                                            pa_sourcelist_cb,
                                                            this);
                    // Update the state so we know what to do next
                    state++;
                }
                break;
            case 2:
                if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                    // Now we're done, clean up and disconnect and return
                    pa_operation_unref(pa_op);
                    pa_context_disconnect(pa_ctx);
                    pa_context_unref(pa_ctx);
                    pa_mainloop_free(pa_ml);
                    return 0;
                }
                break;
            default:
                // We should never see this state
                //fprintf(stderr, "in state %d\n", state);
                return -1;
        }
        // Iterate the main loop and go again.  The second argument is whether
        // or not the iteration should block until something is ready to be
        // done.  Set it to zero for non-blocking.
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }
}


/*! \brief Pulseaudio state change callback.
 *
 * This callback gets called when our context changes state.  We really only
 * care about when it's ready or if it has failed.
 */
void pa_device_list::pa_state_cb(pa_context *c, void *userdata)
{
    pa_context_state_t state;
    int *pa_ready = (int *)userdata;

    state = pa_context_get_state(c);
    switch  (state) {
    // There are just here for reference
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        *pa_ready = 2;
        break;
    case PA_CONTEXT_READY:
        *pa_ready = 1;
        break;
    }
}


/*! \brief Callback function to populate audio sink list
 *  \param ctx The pulseaudio context.
 *  \param info The sink info.
 *  \param eol If greateer than 0 we have reached the end of list.
 *  \param userdata Pointer to "this" PaDeviceList.
 *
 * pa_mainloop will call this function when it's ready to tell us about a sink.
 * It is necessary to have this method declared as static in order to use it as
 * a C-callback function. For the same reason, a pointer to "this" is passed in
 * the usedata parameter, otherwise we wouldn't be able to access non-static members.
 */
void pa_device_list::pa_sinklist_cb(pa_context *ctx, const pa_sink_info *info, int eol, void *userdata)
{
    (void) ctx;

    pa_device_list *pdl = reinterpret_cast<pa_device_list *>(userdata);

    // exit if we have reached the end of the list
    if (eol > 0) {
        return;
    }

    pdl->d_sinks.push_back(pa_device(info->index, info->name, info->description));
}


/*! \brief Callback function to populate audio source list
 *  \param ctx The pulseaudio context.
 *  \param info The source info.
 *  \param eol If greateer than 0 we have reached the end of list.
 *  \param userdata User data (currently NULL).
 *
 * pa_mainloop will call this function when it's ready to tell us about a source.
 * It is necessary to have this method declared as static in order to use it as
 * a C-callback function. For the same reason, a pointer to "this" is passed in
 * the usedata parameter, otherwise we wouldn't be able to access non-static members.
 */
void pa_device_list::pa_sourcelist_cb(pa_context *ctx, const pa_source_info *info, int eol, void *userdata)
{
    pa_device_list *pdl = reinterpret_cast<pa_device_list *>(userdata);

    (void) ctx;

    // exit if we have reached the end of the list
    if (eol > 0) {
        return;
    }

    pdl->d_sources.push_back(pa_device(info->index, info->name, info->description));
}

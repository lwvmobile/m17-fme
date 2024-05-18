/*-------------------------------------------------------------------------------
 * pulse-devices.c
 * M17 Project - Pulse Audio Sink and Source Enumeration
 *
 * based on gist found here
 * https://gist.github.com/andrewrk/6470f3786d05999fcb48
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

#ifdef USE_PULSEAUDIO

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata)
{
  pa_context_state_t state;
  int *pa_ready = userdata;

  state = pa_context_get_state(c);
  switch (state)
  {
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

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata)
{
  pa_devicelist_t *pa_devicelist = userdata;
  int ctr = 0;

  UNUSED(c);

  // If eol is set to a positive number, you're at the end of the list
  if (eol > 0)
    return;

  // We know we've allocated 16 slots to hold devices.  Loop through our
  // structure and find the first one that's "uninitialized."  Copy the
  // contents into it and we're done.  If we receive more than 16 devices,
  // they're going to get dropped.  You could make this dynamically allocate
  // space for the device list, but this is a simple example.

  for (ctr = 0; ctr < 16; ctr++)
  {
    if (! pa_devicelist[ctr].initialized)
    {
      strncpy(pa_devicelist[ctr].name, l->name, 511);
      strncpy(pa_devicelist[ctr].description, l->description, 255);
      pa_devicelist[ctr].index = l->index;
      pa_devicelist[ctr].initialized = 1;
      break;
    }
  }
}

// See above.  This callback is pretty much identical to the previous
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata)
{
  pa_devicelist_t *pa_devicelist = userdata;
  int ctr = 0;

  UNUSED(c);

  if (eol > 0)
    return;

  for (ctr = 0; ctr < 16; ctr++)
  {
    if (! pa_devicelist[ctr].initialized)
    {
      strncpy(pa_devicelist[ctr].name, l->name, 511);
      strncpy(pa_devicelist[ctr].description, l->description, 255);
      pa_devicelist[ctr].index = l->index;
      pa_devicelist[ctr].initialized = 1;
      break;
    }
  }
}

int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output)
{
  // Define our pulse audio loop and connection variables
  pa_mainloop *pa_ml;
  pa_mainloop_api *pa_mlapi;
  pa_operation *pa_op;
  pa_context *pa_ctx;


  // We'll need these state variables to keep track of our requests
  int state = 0;
  int pa_ready = 0;

  // Initialize our device lists
  memset(input, 0, sizeof(pa_devicelist_t) * 16);
  memset(output, 0, sizeof(pa_devicelist_t) * 16);

  // Create a mainloop API and connection to the default server
  pa_ml = pa_mainloop_new();
  pa_mlapi = pa_mainloop_get_api(pa_ml);
  pa_ctx = pa_context_new(pa_mlapi, "test");

  // This function connects to the pulse server
  pa_context_connect(pa_ctx, NULL, 0, NULL);


  // This function defines a callback so the server will tell us it's state.
  // Our callback will wait for the state to be ready.  The callback will
  // modify the variable to 1 so we know when we have a connection and it's
  // ready.
  // If there's an error, the callback will set pa_ready to 2
  pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

  // Now we'll enter into an infinite loop until we get the data we receive
  // or if there's an error
  for (;;)
  {
    // We can't do anything until PA is ready, so just iterate the mainloop
    // and continue
    if (pa_ready == 0)
    {
        pa_mainloop_iterate(pa_ml, 1, NULL);
        continue;
    }
    // We couldn't get a connection to the server, so exit out
    if (pa_ready == 2)
    {
        pa_context_disconnect(pa_ctx);
        pa_context_unref(pa_ctx);
        pa_mainloop_free(pa_ml);
        return -1;
    }
    // At this point, we're connected to the server and ready to make
    // requests
    switch (state)
    {
      // State 0: we haven't done anything yet
      case 0:
        // This sends an operation to the server.  pa_sinklist_info is
        // our callback function and a pointer to our devicelist will
        // be passed to the callback The operation ID is stored in the
        // pa_op variable
        pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, output);

        // Update state for next iteration through the loop
        state++;
        break;
      case 1:
        // Now we wait for our operation to complete.  When it's
        // complete our pa_output_devicelist is filled out, and we move
        // along to the next state
        if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) 
        {
          pa_operation_unref(pa_op);

          // Now we perform another operation to get the source
          // (input device) list just like before.  This time we pass
          // a pointer to our input structure
          pa_op = pa_context_get_source_info_list(pa_ctx, pa_sourcelist_cb, input);
          // Update the state so we know what to do next
          state++;
        }
        break;
      case 2:
        if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE)
        {
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
        fprintf(stderr, "in state %d\n", state);
        return -1;
    }

    // Iterate the main loop and go again.  The second argument is whether
    // or not the iteration should block until something is ready to be
    // done.  Set it to zero for non-blocking.
    pa_mainloop_iterate(pa_ml, 1, NULL);
    
  }
}

int pulse_list() {

  fprintf (stderr, "\n");
  int ctr;

  // This is where we'll store the input device list
  pa_devicelist_t pa_input_devicelist[16];

  // This is where we'll store the output device list
  pa_devicelist_t pa_output_devicelist[16];

  if (pa_get_devicelist(pa_input_devicelist, pa_output_devicelist) < 0)
  {
    fprintf(stderr, "failed to get device list\n");
    return 1;
  }

  for (ctr = 0; ctr < 16; ctr++)
  {
    if (! pa_output_devicelist[ctr].initialized)
      break;

    printf("=======[ Output Device #%d ]=======\n", ctr+1);
    printf("Description: %s\n", pa_output_devicelist[ctr].description);
    printf("Name: %s\n", pa_output_devicelist[ctr].name);
    printf("Index: %d\n", pa_output_devicelist[ctr].index);
    printf("\n");
  }

  for (ctr = 0; ctr < 16; ctr++)
  {
    if (! pa_input_devicelist[ctr].initialized)
      break;

    printf("=======[ Input Device #%d ]=======\n", ctr+1);
    printf("Description: %s\n", pa_input_devicelist[ctr].description);
    printf("Name: %s\n", pa_input_devicelist[ctr].name);
    printf("Index: %d\n", pa_input_devicelist[ctr].index);
    printf("\n");
  }
  return 0;
}

#endif
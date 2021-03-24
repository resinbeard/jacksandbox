/* jackdiff.c
This file is a part of 'jackdiff'
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

'jackdiff' is a simple JACK client for learning and testing audio code.

Copyright 2014 murray foster */

#include<math.h>
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<unistd.h>

#include "rtqueue.h"
#include <jack/jack.h> 
 
static jack_default_audio_sample_t ** outs ;
static jack_default_audio_sample_t ** ins ;
int samples_can_process = 0;

rtqueue_t *fifo_in;
rtqueue_t *fifo_out;

jack_client_t *client=NULL;
pthread_t dspthreadid;
int jack_sr;
const size_t sample_size = sizeof (jack_default_audio_sample_t) ;

struct selfobj {
  double x1;
  double x2;
  double y1;
  double y2;
};

struct selfobj self;

int freq = 300;
int Q = 150;
float a = 1.0;

jack_port_t **output_port ;
static jack_port_t **input_port ;



float dsplogic( float insample ) {
  float outsample = 0.0;

  printf("insample: %f\n", insample);
  outsample = insample;

  return outsample;
}

static void*
dspthread(void *arg)
{
  while(1) {
    rtqueue_enq(fifo_out, dsplogic(rtqueue_deq(fifo_in)));
  }
}
 
static int
process(jack_nframes_t nframes, void *arg)
{
  float sample = 0; 
  unsigned i, n, x; 
  int sample_count = 0;

  /* allocate all output buffers */
  for(i = 0; i < 1; i++)
    {
      outs [i] = jack_port_get_buffer (output_port[i], nframes);
      memset(outs[i], 0, nframes * sample_size);
      ins [i] = jack_port_get_buffer (input_port[i], nframes);
    }

  for ( i = 0; i < nframes; i++) {
    if( !rtqueue_isempty(fifo_out) ) {
      outs[0][i] = rtqueue_deq(fifo_out);
    }
    else {
      outs[0][i]=0;
    }
    rtqueue_enq(fifo_in,ins[0][i]);
  }
  return 0 ;
} /* process */

static void
jack_shutdown (void *arg)
{
  (void) arg ;
  exit (1) ;
} /* jack_shutdown */

void
allocate_ports(int channels, int channels_in)
{
  int i = 0;
  char name [16];
  /* allocate output ports */
  output_port = calloc (channels, sizeof (jack_port_t *)) ;
  outs = calloc (channels, sizeof (jack_default_audio_sample_t *)) ;
  for (i = 0 ; i < channels; i++)
    {     
      snprintf (name, sizeof (name), "out_%d", i + 1) ;
      output_port [i] = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0) ;
    }
  
  /* allocate input ports */
  size_t in_size = channels_in * sizeof (jack_default_audio_sample_t*);
  input_port = (jack_port_t **) malloc (sizeof (jack_port_t *) * channels_in);
  ins = (jack_default_audio_sample_t **) malloc (in_size);
  memset(ins, 0, in_size);
  
  for( i = 0; i < channels_in; i++)
    {      
      snprintf( name, sizeof(name), "in_%d", i + 1);
      input_port[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }
} /* allocate_ports */

int
set_callbacks()
{
  /* Set up callbacks. */
  jack_set_process_callback (client, process, NULL) ;
  jack_on_shutdown (client, jack_shutdown, 0) ;
  return 0;
} /* set_callbacks */

int
jack_setup(char *client_name)
{
  /* create jack client */
  if ((client = jack_client_open(client_name, JackNullOption,NULL)) == 0)
    {
      fprintf (stderr, "Jack server not running?\n") ;
      return 1 ;
    } ;

  /* store jack server's samplerate */
  jack_sr = jack_get_sample_rate (client) ;

  fifo_in = rtqueue_init(9999999);
  fifo_out = rtqueue_init(9999999);

  return 0;
} /* jack_setup */

int
activate_client()
{
  /* Activate client. */
  if (jack_activate (client))
    {	
      fprintf (stderr, "Cannot activate client.\n") ;
      return 1 ;
    }
  return 0;
} /* activate_client */

int main(void)
{
  jack_setup("jackdiff");
  set_callbacks();
  if (activate_client() == 1)
    return 1;
  allocate_ports(1, 1);
  pthread_create(&dspthreadid, NULL, dspthread, 0);

  while(1) {
    sleep(1);
    freq = 0.25;
        sleep(1);
    freq = 0.01;
        sleep(1);
    freq = 0.5;
        sleep(1);

  };

  return 0;
}

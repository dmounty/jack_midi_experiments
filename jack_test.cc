#include <iostream>

#include <unistd.h>
#include <cstdlib>
#include <cstring>

#include <jack/jack.h>


jack_port_t *input_port;
jack_port_t *output_port;


int process(jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer(output_port, nframes);
  jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer(input_port, nframes);

  memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);

  return 0;
}

int srate(jack_nframes_t nframes, void *arg)

{
  std::cout << "the sample rate is now " << nframes << "/sec" << std::endl;
  return 0;
}

void error(const char *desc)
{
  std::cerr << "JACK error: " << desc << std::endl;
}

void jack_shutdown(void *arg)
{
  exit(1);
}

int main(int argc, char *argv[])
{
  // This is the pointer to the(yet to be created) jack client
  jack_client_t *client;

  // These are pointers to our port names
  const char **ports;


  if(argc < 2) {
    std::cerr << "usage: jack_test <client_name>" << std::endl;
    return 1;
  }

  // Register a callback to handle jack errors
  jack_set_error_function(error);

  // Create our jack client(named after the argument)
  if((client = jack_client_open(argv[1], JackNoStartServer, NULL)) == 0) {
    std::cerr << "jack server not running?" << std::endl;
    return 1;
  }

  // Register a callback to process(where we spend most of our time)
  jack_set_process_callback(client, process, 0);

  // Register a callback for when the sample rate changes
  jack_set_sample_rate_callback(client, srate, 0);

  // Register a callback for if the jack server shuts down
  jack_on_shutdown(client, jack_shutdown, 0);

  std::cout << "engine sample rate: " << jack_get_sample_rate(client) << std::endl;

  // Create two audio ports
  input_port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  output_port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  // Activate the client
  if(jack_activate(client)) {
    std::cerr << "cannot activate client";
    return 1;
  }

  // Get an array of all physical capture ports
  if((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
    std::cerr << "Cannot find any physical capture ports" << std::endl;
    exit(1);
  }

  // Connect our input_port to the first physical capture port we found
  if(jack_connect(client, ports[0], jack_port_name(input_port))) {
    std::cerr << "cannot connect input ports" << std::endl;
  }

  free(ports);

  //Get an array of all physical output ports
  if((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
    std::cerr << "Cannot find any physical playback ports" << std::endl;
    exit(1);
  }

  // Connect out output port to all physical output ports (NULL terminated array)
  int i=0;
  while(ports[i]!=NULL){
    if(jack_connect(client, jack_port_name(output_port), ports[i])) {
      std::cerr << "cannot connect output ports" << std::endl;
    }
    i++;
  }

  free(ports);

  // We spin here slowly while jack sends us callbacks to the functions we registered
  for(;;)
    sleep(1);

  // We won't actually reach this code
  jack_client_close(client);
  return 0;
}

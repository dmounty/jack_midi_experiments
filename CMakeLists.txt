cmake_minimum_required (VERSION 2.6)
project (Jack)
# The version number.

find_package(PkgConfig REQUIRED)
pkg_search_module(JACK REQUIRED jack)
pkg_search_module(SNDFILE REQUIRED sndfile)

add_executable(jack_echo jack_echo.cc)
# add the executable
target_link_libraries(jack_echo ${JACK_LIBRARIES})
target_include_directories(jack_echo PUBLIC ${JACK_INCLUDE_DIRS})
target_compile_options(jack_echo PUBLIC ${JACK_CFLAGS_OTHER})

add_executable(jack_midi_stripe jack_midi_stripe.cc)
# add the executable
target_link_libraries(jack_midi_stripe ${JACK_LIBRARIES})
target_include_directories(jack_midi_stripe PUBLIC ${JACK_INCLUDE_DIRS})
target_compile_options(jack_midi_stripe PUBLIC ${JACK_CFLAGS_OTHER})

add_executable( jack_midi_synth
  jack_midi_synth.cc
  jack_midi_synth_app.cc
  jack_midi_synth_envelopes.cc
  jack_midi_synth_filters.cc
  jack_midi_synth_logic.cc
  jack_midi_synth_oscillators.cc
  jack_midi_synth_sample.cc
  jack_midi_synth_sample_manager.cc
  jack_midi_synth_voice.cc
)
# add the executable
target_link_libraries(jack_midi_synth ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES})
target_include_directories(jack_midi_synth PUBLIC ${JACK_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
target_compile_options(jack_midi_synth PUBLIC ${JACK_CFLAGS_OTHER} ${SNDFILE_CFLAGS_OTHER})

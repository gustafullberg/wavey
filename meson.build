project('wavey', 'cpp', default_options : ['cpp_std=c++17'])
gtkmm = dependency('gtkmm-3.0')
gl = dependency('gl')
glm = dependency('glm')
portaudio = dependency('portaudio-2.0')
sndfile = dependency('sndfile')
fftw = dependency('fftw3f')
openmp = dependency('openmp')
src = files(
  'audio_buffer.cpp',
  'audio_system.cpp',
  'gpu_spectrogram.cpp',
  'gpu_track_label.cpp',
  'gpu_waveform.cpp',
  'gui.cpp',
  'label_renderer.cpp',
  'main.cpp',
  'primitive_renderer.cpp',
  'shader.cpp',
  'spectrogram.cpp',
  'spectrogram_shader.cpp',
  'state.cpp',
  'track_label.cpp',
  'wave_shader.cpp',
  'zoom_window.cpp'
  )
executable('wavey', src, dependencies : [gtkmm, gl, glm, portaudio, sndfile, fftw, openmp], install : true)

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

src = files(
  'audio_buffer.cpp',
  'audio_mixer.cpp',
  'audio_system.cpp',
  'file_load_server.cpp',
  'file_notification.cpp',
  'gpu_spectrogram.cpp',
  'gpu_waveform.cpp',
  'low_res_waveform.cpp',
  'low_res_waveform.hpp',
  'main.cpp',
  'primitive_renderer.cpp',
  'renderer.cpp',
  'sample_line_shader.cpp',
  'sample_point_shader.cpp',
  'shader.cpp',
  'spectrogram.cpp',
  'spectrogram_shader.cpp',
  'spectrum_state.cpp',
  'spectrum_window.cpp',
  'state.cpp',
  'wave_shader.cpp',
  'zoom_window.cpp'
  )

executable('wavey', src, dependencies : [gl, glm, sndfile, fftw, openmp, sdl2, imgui], install : true)

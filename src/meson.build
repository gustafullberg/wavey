src = files(
  'audio_buffer.cpp',
  'audio_mixer.cpp',
  'audio_system.cpp',
  'file_notification.cpp',
  'gpu_label.cpp',
  'gpu_spectrogram.cpp',
  'gpu_waveform.cpp',
  'label.cpp',
  'label_renderer.cpp',
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
  'state.cpp',
  'wave_shader.cpp',
  'zoom_window.cpp'
  )

executable('wavey', src, dependencies : [gtkmm, gl, glm, portaudio, sndfile, fftw, openmp, sdl2, imgui], install : true, cpp_args : ['-DGTKMM_DISABLE_DEPRECATED', '-DGDKMM_DISABLE_DEPRECATED', '-DGLIBMM_DISABLE_DEPRECATED', '-DGIOMM_DISABLE_DEPRECATED'])

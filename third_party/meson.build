imgui_src = files(
  'imgui/imgui.cpp',
  'imgui/imgui_demo.cpp',
  'imgui/imgui_draw.cpp',
  'imgui/imgui_tables.cpp',
  'imgui/imgui_widgets.cpp',
  'imgui/backends/imgui_impl_opengl3.cpp',
  'imgui/backends/imgui_impl_sdl.cpp',
)
imgui_inc = include_directories(
  'imgui',
  'imgui/backends',
)
imgui_lib = static_library('imgui', imgui_src, include_directories: imgui_inc, dependencies : [sdl2, dl])
imgui = declare_dependency(link_with: imgui_lib, include_directories: imgui_inc)



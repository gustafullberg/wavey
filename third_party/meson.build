imgui_src = files(
  'imgui/imgui.cpp',
  'imgui/imgui_demo.cpp',
  'imgui/imgui_draw.cpp',
  'imgui/imgui_tables.cpp',
  'imgui/imgui_widgets.cpp',
  'imgui/misc/cpp/imgui_stdlib.cpp',
  'imgui/backends/imgui_impl_opengl3.cpp',
  'imgui/backends/imgui_impl_sdl3.cpp',
)
imgui_inc = include_directories(
  'imgui',
  'imgui/backends',
)
imgui_lib = static_library('imgui', imgui_src, include_directories: imgui_inc, dependencies : [sdl3, dl])
imgui = declare_dependency(link_with: imgui_lib, include_directories: imgui_inc)




implot_src = files(
  'implot/implot.h', 
  'implot/implot_internal.h', 
  'implot/implot.cpp', 
  'implot/implot_items.cpp',
  'implot/implot_demo.cpp',
)
implot_inc = include_directories(
  'implot'
)
implot_lib = static_library('implot', implot_src, include_directories: implot_inc, dependencies : [imgui, dl])
implot= declare_dependency(link_with: implot_lib, include_directories: implot_inc)

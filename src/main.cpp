#include <gtkmm.h>
#include "audio_system.hpp"
#include "gui.hpp"
#include "spectrum_window.hpp"
#include "state.hpp"
#include "spectrum_state.hpp"

int main(int argc, char** argv) {
    AudioSystem audio;
    State state(&audio);
    SpectrumState spectrum_state;
    for (int i = 1; i < argc; i++) {
        state.LoadFile(argv[i]);
    }
    argc = 1;
    auto app = Gtk::Application::create(argc, argv, "com.github.gustafullberg.wavey",
                                        Gio::APPLICATION_NON_UNIQUE);
    Gui gui(&state);
    SpectrumWindow spectrum_window(&spectrum_state);
    gui.signal_add_spectrum().connect(sigc::mem_fun(&spectrum_window, &SpectrumWindow::AddSpectrumFromTrack));
    app->add_window(spectrum_window);
    app->run(gui);
    return 0;
}

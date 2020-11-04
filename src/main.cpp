#include <gtkmm.h>
#include "gui.hpp"

int main(int argc, char** argv) {
    AudioSystem audio;
    State state(&audio);
    for (int i = 1; i < argc; i++) {
        state.LoadFile(argv[i]);
    }
    argc = 1;
    auto app = Gtk::Application::create(argc, argv, "com.github.gustafullberg.wavey",
                                        Gio::APPLICATION_NON_UNIQUE);
    Gui gui(&state);
    app->run(gui);
    return 0;
}

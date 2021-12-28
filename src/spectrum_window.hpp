#ifndef SPECTRUM_WINDOW_HPP_
#define SPECTRUM_WINDOW_HPP_

#include <gtkmm.h>

#include "state.hpp"

class SpectrumState;

class SpectrumWindow: public Gtk::Window {
   public:
    explicit SpectrumWindow(SpectrumState* state);

  void AddSpectrumFromTrack(const Track& t, float begin, float end, int channel);

   private:

    SpectrumState* state_ = nullptr;


};

#endif

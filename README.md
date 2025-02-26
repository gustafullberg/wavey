# wavey
Linux tool to analyze and play audio files.

### Screenshots
![Waveform](https://user-images.githubusercontent.com/5707617/213416739-2d8cf37e-f925-4758-b508-1a54c92ae93a.png)

![Bark-scale spectrogram](https://user-images.githubusercontent.com/5707617/213416746-eac57b6d-2677-47c8-abc3-4e0a2df03fbf.png)

![Samples](https://user-images.githubusercontent.com/5707617/213416751-5f335dcc-6281-4389-a66a-156ecf15e360.png)

### Usage
#### General
- ``Ctrl+W`` - Close the selected file
- ``Ctrl+Shift+W`` - Close all files
- ``Ctrl+R`` - Reload all modified files
- ``Ctrl+Q`` - Quit
- ``Space`` - Start / stop playback from cursor
- ``Shift+Space`` - Start / stop looping playback from cursor

#### Selection
- ``Mouse click`` - Set cursor
- ``Mouse drag`` - Select part of file
- ``Home`` - Move cursor to the beginning
- ``End`` - Move cursor to the end
- ``Up``, ``Down`` or ``mouse hover`` - Select file for playback
- ``Shift+Up``or ``Shift+Down`` - Move selected track in the list
- ``Ctrl+Up``or ``Ctrl+Down`` - Change displayed channel on the selected track 

#### View
- ``s`` - Spectrogram / waveform view
- ``b`` - Bark scale / linear spectrograms
- ``z`` - Single file / all files view
- ``Shift+z`` - Single channel / all channels view
- ``f`` - Follow mode (view follows cursor during playback)
- ``d`` - Display the waveform in db scale.

#### Zoom
- ``Mouse wheel`` - Zoom in / out
- ``Ctrl+F`` - Maximum zoom out
- ``Ctrl+E`` - Zoom to selection
- ``Plus`` - Zoom in
- ``Minus`` - Zoom out
- ``Ctrl + mouse wheel`` - Vertical zoom in / out
- ``Ctrl+Plus`` - Vertical zoom in
- ``Ctrl+Minus`` - Vertical zoom out
- ``Ctrl+0`` - Reset vertical zoom
- ``Left`` - Pan left
- ``Right`` - Pan right
- ``Shift+Z`` - Toggle only show the current channel on selected track 

### Build
#### Build dependencies Debian
``` apt-get install libgl-dev libglm-dev portaudio19-dev libsndfile1-dev libfftw3-dev ```

#### Build dependencies Arch Linux
``` pacman -S glm portaudio libsndfile fftw ```


#### Get the source, build and install
```
git clone https://github.com/gustafullberg/wavey.git
cd wavey
git submodule update --init
mkdir build
cd build
meson ..
ninja install
```

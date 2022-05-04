# wavey
Linux tool to analyze and play audio files.

### Screenshots
![Bark-scale spectrogram](https://user-images.githubusercontent.com/5707617/131128638-0470e049-801a-494b-a492-65e153ca4040.png)

![Waveform](https://user-images.githubusercontent.com/5707617/131128660-0270f8ac-b391-4508-abb8-35d7441c5d9b.png)

### Usage
#### General
- ``Ctrl+O`` - Open files
- ``Ctrl+S`` - Save selection to new file
- ``Ctrl+W`` - Close the selected file
- ``Ctrl+Shift+W`` - Close all files
- ``Ctrl+R`` - Reload all modified files
- ``r`` - Toggle file auto-reload
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
``` apt-get install libgtkmm-3.0-dev libgl-dev libglm-dev portaudio19-dev libsndfile1-dev libfftw3-dev ```
#### Build dependencies Arch Linux
``` pacman -S gtkmm3 glm portaudio libsndfile fftw ```

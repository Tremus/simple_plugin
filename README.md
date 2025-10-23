Quick and dirty juce synth with a filter
GUI shows a graph displaying the frequency of audio callbacks 

Build:

- `git submodule update --init --depth=1`
- `cmake -B build -G Ninja .` (Or whatever generator you like)
- `cmake --build build`

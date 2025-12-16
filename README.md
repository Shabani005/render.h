# An Experiment of how simple rendering can be, condensed into 2 STB-style single header files.

## Briefing: simplistic software rendering library written aiming to support as wide of a range of systems as possible (by not depending on C std lib).

- render.h > platform independent, barebones
- renderfull.h > still simple, but shows how you can build on top of render.h and build abstractions

The idea is to just use render.h and provide the platform dependent custom implementations on top of it, which is displayed in renderfull.h

Examples of how it can be used in the terminal, web browser canvas (via WASM) and raylib as a window provider in the /examples folder

- To build Examples (requires raylib to be installed)
```bash
  cc -o builder builder.c
  ./builder
```

- The experimentation from this library indirectly gave birth to [ByteFrame](https://github.com/Chip-Biscuit/ByteFrame)

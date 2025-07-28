# Asteroids
An asteroids-like created in c using [raylib](https://raylib.com)

## Build Instructions
### Local
```bash
# Clone repository
git clone https://github.com/uvd540/asteroids-c
# Navigate to cloned repository
cd asteroids-c
# Setup build system
cmake -B build
# Build the executable
cmake --build build
# Run!
./build/asteroids
```
### Web
Requires the emscripten toolchain to be installed
```bash
# Clone repository
git clone https://github.com/uvd540/asteroids-c
# Navigate to cloned repository
cd asteroids-c
# Setup build system
emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXECUTABLE_SUFFIX=".html"
# Build: Creates files that can be uploaded to itch.io for example
emmake make
```


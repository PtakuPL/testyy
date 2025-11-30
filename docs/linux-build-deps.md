# Linux build dependencies

The following packages satisfied missing dependencies reported by CMake on a clean Ubuntu 24.04 environment:

```
apt-get install -y \
  libharfbuzz-dev libfribidi-dev libprotobuf-dev protobuf-compiler \
  libphysfs-dev nlohmann-json3-dev libpugixml-dev libfmt-dev \
  libopenal-dev libvorbis-dev libglew-dev libluajit-5.1-dev
```

If additional packages are required, rerun configuration with `cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo` to view any missing dependency messages.
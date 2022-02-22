# FLTK Demo - Editor

https://www.fltk.org/doc-1.4/editor.html

### Install FLTK on macOS with CMake

https://github.com/fltk/fltk/blob/master/README.macOS.md#how-to-build-fltk-using-cmake-and-make

### Compile under macOS
with c++ compiler:
```
c++ -I/usr/local/include -o 'editor' 'main.cpp' /usr/local/lib/libfltk.a -I/usr/local/include/FL/images -lm -framework Cocoa
```


or with fltk-config script:

`fltk-config --compile main.cpp`

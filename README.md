# Setup

## MacOS

1. Install Homebrew

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

2. Install raylib

```bash
brew install raylib
```

3. Use raylib in your project

```c
#include "raylib.h"
```

4. Compile and run

```bash
# Apple silicon
gcc main.c -o game -L/opt/homebrew/lib -I/opt/homebrew/include -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

# Intel chip
gcc main.c -o game -L/usr/local/lib -I/usr/local/include -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
```

Directories change depending on the hardware being used.
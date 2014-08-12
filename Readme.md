# Introduction

For an introduction to what this code is and does, please see [this article][0].

[0]: <http://www.gabrielgambetta.com/remakes.html>

# Usage

    cd goody
    make
    ./goody

Requires the [SDL][1] headers and libraries to be installed. Also requires a sane development platform (i.e. not Windows)

[1]: <http://www.libsdl.org>

# Structure of the code

**`lib/generator/`** - Templates and opcode spec for the x86 emulator fetch/decode loop, which is automatically generated.

**`lib/`** - The emulator code itself, including x86, CGA and devices.

**`tools/runner`** - The runner/debugger. Look at the source to see the available commands. You can put startup commands in `runner.cmd`.

**`tools/disassemble`** - The disassembler. Takes a prefix as a parameter, not a filename, e.g. `prefix`, and disassembles `prefix.com` into `prefix.asm`, optionally reading `prefix.cfg`.

The disassembler works by following code paths from the entry point and disassembling all the reachable paths. But because it doesn't actually run the code, it misses entry points accessible through jump tables, e.g. `JMP BX`. You can manually add `EntryPoint` commands in the `cfg` file.

Disassembly can be done incrementally. If `prefix.asm` already exists, the disassembler will _merge_ the comments in the existing file into the new disassembly. So you can disassemble, add an entry point to the `cfg` file, run the disassembler again, and not lose your previous work. `runner` can also generate a list of the entry points it actually runs through.


**`goody/`** - The Goody remake proof-of-concept.

# License

The **code** is licensed under the Whatever/Credit license: _you may do whatever you want with the code; if you make something cool, credit is appreciated._

The **assets** are either public domain or licensed under various Creative Commons licenses. Please see the original licenses at the following links: [1][100], [2][101], [3][102], [4][103], [5][104].

[100]: <http://opengameart.org/content/dawn-of-the-gods>
[101]: <http://opengameart.org/content/classical-ruin-tiles>
[102]: <http://opengameart.org/content/pixel-art-castle-tileset>
[103]: <http://opengameart.org/content/2d-platformer-side-scroller-stone-fence-street-lamp>
[104]: <http://opengameart.org/content/platformer-art-replacement-gui-text>

The file **goody.com** should _not_ be redistributed. I got the unofficial blessing from Gonzo Suarez, one of the original authors, and it can be found in abandonware sites, but still. Don't redistribute it.


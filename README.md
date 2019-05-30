mididebug.lv2 - MIDI Message Generator
======================================

mididebug.lv2 is an instrumention tool to generate arbitrary MIDI
messages up to 3 bytes in length. It's mainly intented for
http://moddevices.com/ but works in any LV2 plugin-host.

Install
-------

Compiling mididebug.lv2 requires the LV2 SDK, gnu-make, and a c-compiler.

```bash
  git clone git://github.com/x42/mididebug.lv2.git
  cd mididebug.lv2
  make
  sudo make install PREFIX=/usr
```

To build the the MOD GUI use `make MOD=1`

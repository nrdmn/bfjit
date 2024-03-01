# bfjit

This is a brainfuck interpreter that uses LLJIT.

## Building

```
meson setup build
meson compile -C build
```

Depending on your llvm installation, you may have to setup with `--prefer-static`.

## Usage

Run a bf file with `bfjit <filename>`.

To dump the AST, run `bfjit --dump-ast <filename>`.

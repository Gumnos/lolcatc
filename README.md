# `lolcatc(6)`

## About

Clones most of the functionality of
[`lolcat(6)`](https://github.com/busyloop/lolcat/)
providing 8-bit and 24-bit rainbow coloration
of files and/or stdin.
Most notably, this all happens in a single binary written in C,
rather than requiring the entire Ruby runtime
like the original does.

## Building

`lolcatc(6)` should only use features from the C standard library.
On a BSD system, it should be as simple as

    $ make

However, at this time, it looks like the `/usr/lib/libm.a`
wasn't built with `-fPIC` so the `make` command currently fails.

Alternatively, you can use

    $ cc -lm -o lolcatc lolcatc.c

(don't forget to include the math `-lm` library).

When building on [OpenBSD](https://openbsd.org),
it makes use of the `pledge(2)` system-call
to reduce privileges to only the required
file-reading and stdio operations.

## Usage

Behaving similar to `cat(1)`,
you can specify multiple files on the command-line,
using `-` to indicate `stdin`:

    $ lolcatc file.txt
    $ generate_body | lolcatc header.txt - body.txt

or use `stdin`:

    $ ls | lolcatc

## Options

You can obtain the list of available options
with `-h`:

    $ lolcat -h
    lolcatc [options] [filename]
     -v  Version
     -h  This help
     -p  Spread (default=3.00)
     -F  Frequency (default=0.10)
     -S  Seed (default=0)
     -i  Invert foreground/background
     -t  24-bit color output
     -f  Force color even when stdout is not a tty

The `-p` (spread) and `-F` (frequency) control aspects
of the rainbow-nature.
You can experiment with the values to see what you prefer.

The `-t` (true-color) option, produces more beautiful output
if your terminal supports it.

If you pipe output to other commands,
`lolcatc(6)` will detect this and effectively act like `cat(1)`.
However, you can use the `-f` option to force it to output color.
Most notably, this proves useful for piping to `less(1)`:

    $ lolcatc -f -t jabberwocky.txt | less -R

Lastly, the `-i` option creates the rainbow out of the
**background** color instead of the foreground color.

## Unimplemented features of `lolcat(6)`

The original `lolcat(6)` offers animation.
This will never offer that functionality.

The original `lolcat(6)` also offers the
ability to set the `seed` (with the `-S` option)
which `lolcatc(6)` currently accepts,
but does not currently use.
At some point in the future, perhaps.

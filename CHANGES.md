# vbig Change History

## Release 3

* Builds against newer versions of Nettle.
* [Fixed mishandling of bytes remaining counter](https://github.com/ewxrjk/vbig/issues/6).
* [Improve progress reporting when flushing](https://github.com/ewxrjk/vbig/issues/5).
* Tidy up usage message.
* Don’t create new output with the (deprecated) `arcfour` RNG.

## Release 1

* New RNGs have been introduced. Use the `--rng` option to choose between them.
* The default RNG is `aes-ctr-drbg-128`.
* On Linux and macOS, `vbig` attempts to identify risky filenames and will not write to them without the `--force` option.
* Improved documentation.
* Improved testing.
* New dependencies: Nettle, jsoncpp (Linux only), tinyxml (macOS only).

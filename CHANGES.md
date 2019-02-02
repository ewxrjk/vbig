# vbig Change History

## Release 1

* New RNGs have been introduced. Use the `--rng` option to choose between them.
* The default RNG is `aes-ctr-drbg-128`.
* On Linux and macOS, `vbig` attempts to identify risky filenames and will not write to them without the `--force` option.
* Improved documentation.
* Improved testing.
* New dependencies: Nettle, jsoncpp (Linux only), tinyxml (macOS only).

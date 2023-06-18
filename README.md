# minitar

Tiny and easy-to-use C library to read/write tar (specifically, the [ustar](https://www.ibm.com/docs/en/zos/2.3.0?topic=formats-tar-format-tar-archives#taf) variant, which is a bit old but simple, and newer tar variants (pax, GNU tar) are mostly backwards-compatible with it) archives.

No third-party dependencies, only a minimally capable standard C library (pretty much only requires a basic subset of the C FILE API, apart from other simple functions). 

Aims to be bloat-free (currently a bit above 500 LoC), fast and optimized, and as portable between systems as possible (has its own implementation of some non-standard functions, such as [strlcpy](https://linux.die.net/man/3/strlcpy) or [basename](https://linux.die.net/man/3/basename)).

Does not include support for compressed archives. You'll have to pass those through another program or library to decompress them before minitar can handle them.

## Example

```c
#include <stdio.h>
#include <minitar.h>

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return 1;
	}
	struct minitar mp;
	if(minitar_open(argv[1], &mp) != 0)
	{
		perror(argv[1]);
		return 1;
	}
	struct minitar_entry entry;
	do {
		if(minitar_read_entry(&mp, &entry) == 0) { 
			printf("%s\n", entry.metadata.path);
		} else break;
	} while(1);
	minitar_close(&mp);
}
```

The output of this example program when running it with an uncompressed tar archive is identical to that of `tar --list -f archive.tar` with the same uncompressed archive. And in most cases, it's faster as well!

See [examples](examples/) for more examples using minitar.

## Project structure

The user-facing API (functions defined in `minitar.h` and documented in [API.md](docs/API.md)) is implemented in `src/tar.c`. Utility and internally-used functions live in `src/util.c`.

## Documentation

See the [build instructions](docs/Build.md) to start using minitar.

See the [API documentation](docs/API.md) for a full description of all functions and types.

## Error handling

When a fatal error occurs, minitar calls the function `minitar_handle_panic()` with a message describing the error.
The default implementation of this function prints the error message out to standard error and aborts.

You might want to handle errors differently. Well, you can override the panic function! Just create a function with the following signature:

`noreturn void minitar_handle_panic(const char* message)`

and put your error handling code in there. This function will automatically override the default one used by minitar.

This function needs to have C linkage and be unmangled. If you're using other languages, this might not be the case, for example, a C++ implementation would need the following signature:

`extern "C" [[noreturn]] void minitar_handle_panic(const char* message)`

and a Rust implementation would need:

```
#[no_mangle]
pub extern "C" fn minitar_handle_panic(message: *const u8) -> !
```

## License

`minitar` is free and open-source software under the [BSD-2-Clause](LICENSE) license.

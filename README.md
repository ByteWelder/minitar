# minitar

Tiny and easy-to-use C library to parse tar (specifically, the newer [USTAR](https://www.ibm.com/docs/en/zos/2.3.0?topic=formats-tar-format-tar-archives#taf) variant, which is the one pretty much everybody uses) archives. 

No third-party dependencies, only a minimally capable standard C library (file IO, number parsing, malloc() and friends, string functions). 

Aims to be as portable between systems as possible (has its own implementation of some non-standard functions, such as [strlcpy](https://linux.die.net/man/3/strlcpy), [strndup](https://linux.die.net/man/3/strndup) or [basename](https://linux.die.net/man/3/basename)).

Very minimal and bloat-free, currently less than 500 lines :)

Does not include support for compressed archives. You'll have to pass those through another program or library to decompress them before minitar can handle them.

## Example

```
#include <stdio.h>
#include <minitar.h>

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return 1;
	}
	struct minitar* mp = minitar_open(argv[1]);
	if(!mp)
	{
		perror(argv[1]);
		return 1;
	}
	struct minitar_entry* entry;
	do {
		entry = minitar_read_entry(mp);
		if(entry) { 
			printf("%s\n", entry->metadata.path);
			minitar_free_entry(entry);
		}
	} while(entry);
	minitar_close(mp);
}
```

The output of this example program when running it with an uncompressed tar archive is identical to that of `tar --list -f archive.tar` with the same uncompressed archive.

## Project structure

The user-facing API (functions defined in `minitar.h` and documented in this README) is implemented in `src/tar.c`. Utility and internally-used functions live in `src/util.c`.

## Functions
### minitar_open
`struct minitar* minitar_open(const char* pathname)`

Opens a tar archive for reading, and returns a heap-allocated `struct minitar` which must be freed with `minitar_close()` after using it. If opening the file or allocating the struct fails, returns NULL.

A `struct minitar` is opaque, and should only be passed to other minitar functions. You should not care about its contents.

### minitar_read_entry
`struct minitar_entry* minitar_read_entry(struct minitar* mp)`

Reads the next entry from a `struct minitar` which should be the return value of a previous call to `minitar_open()`. The return value is a heap-allocated `struct minitar_entry`, which should be freed with `minitar_free_entry()` when no longer needed. 

This structure consists of the file metadata (in the `metadata` field), and other internally-used values.

To read the contents of an entry, you should allocate a buffer large enough to hold `metadata.size` bytes and pass it to `minitar_read_contents()`.

This function returns NULL on end-of-file (when all entries have been read).

### minitar_free_entry
`void minitar_free_entry(struct minitar_entry* entry)`

Frees the heap-allocated `struct minitar_entry`. The pointer passed to `minitar_free_entry()` should be the return value of a previous call to `minitar_read_entry()`, `minitar_find_by_name()`, `minitar_find_by_path()` or `minitar_find_any_of()`.

### minitar_rewind
`void minitar_rewind(struct minitar* mp)`

Rewinds the `struct minitar` back to the beginning of the archive file, which means that the next call to `minitar_read_entry()` will return the first entry instead of the entry after the last read entry.

### minitar_find_by_name
`struct minitar_entry* minitar_find_by_name(struct minitar* mp, const char* name)`

Returns the first entry with a matching name, or NULL if none are found. The return value is a `struct minitar_entry`, which is heap-allocated and should be freed after use with `minitar_free_entry()`. This structure is already documented in the entry documenting `minitar_read_entry()`.

This function starts searching from the current archive position, which means that to find a matching entry in the entire archive `minitar_rewind()` should be called on it first.

The state of `mp` after `minitar_find_by_name()` returns is unspecified, but a successive call to `minitar_find_by_name()` will return the next matching entry, if there is one. (Calling `minitar_find_by_name()` in a loop until it returns NULL will return all matching entries.)

In order to perform other minitar operations on the archive, `minitar_rewind()` should probably be called first, to get a known state.

### minitar_find_by_path
`struct minitar_entry* minitar_find_by_path(struct minitar* mp, const char* path)`

Same as `minitar_find_by_name()`, but matches the full path inside the archive instead of the file name.


### minitar_find_any_of
`struct minitar_entry* minitar_find_any_of(struct minitar* mp, enum minitar_file_type type)`

Same as `minitar_find_by_name()`, but matches the file type instead of the name. As with `minitar_find_by_name()`, this function starts searching from the current archive position and calling it in a loop until it returns NULL will return all matching entries.

### minitar_read_contents
`size_t minitar_read_contents(struct minitar* mp, struct minitar_entry* entry, char* buf, size_t max)`

Reads up to `max` bytes of an entry's contents from the archive stream `mp` and stores them into `buf`.

This function can be called as many times as desired, and at any given point in time, provided both `mp` and `entry` are valid. (`mp` should be the return value of a previous call to `minitar_open()`, and `entry` the return value of a previous call to `minitar_read_entry()`, `minitar_find_by_name()`, `minitar_find_by_path()` or `minitar_find_any_of()`).

This function returns the number of bytes read, or 0 on error. 0 might also be a successful return value (if `max` is 0 or the entry's size is 0, for example), which means `errno` should be checked to see if 0 means error or simply 0 bytes read.

`minitar_read_contents()` only reads up to `metadata.size`, regardless of the value in `max`.

The contents are not null-terminated. If you want null-termination (keep in mind the contents might not be ASCII and might contain null bytes before the end), just do `buf[nread] = 0;`. In that case, the value of `max` should be one less than the size of the buffer, to make sure the zero byte is not written past the end of `buf` if `max` bytes are read.

### minitar_close
`int minitar_close(struct minitar* mp)`

Closes the tar archive file `mp` points to and frees the heap memory it was using. The pointer passed to `minitar_close()` should be the return value of a previous call to `minitar_open()`.

Returns 0 on success, everything else is failure and you should check `errno`.

## Types

### minitar_file_type
`enum minitar_file_type`

This enum lists all supported file types:

`MTAR_REGULAR`: Regular files

`MTAR_DIRECTORY`: Directories

Other file types supported in tar archives, such as block/character devices, FIFOs, or symlinks, are not supported and minitar will throw an error when encountering one of them.

### minitar_entry_metadata
`struct minitar_entry_metadata`

This structure represents an entry's metadata, with the following fields:

`path`: A string representing the full path of the entry within the archive. (`char[]`)

`name`: A string representing the base name of the entry (the last component of its path). (`char[]`)

`mode`: An integer representing the permissions of the entry. (`mode_t`)

`uid`: An integer representing the user ID of the entry's owner. (`uid_t`)

`gid`: An integer representing the group ID of the entry's owner. (`gid_t`)

`size`: An integer representing the size of the entry's contents in bytes. (`size_t`)

`mtime`: A UNIX timestamp representing the last time the entry was modified. (`time_t`)

`type`: An enum representing the type of the entry. (`enum minitar_file_type`)

`uname`: A string representing the username of the entry's owner. (`char[]`)

`gname`: A string representing the group name of the entry's owner. (`char[]`)

### minitar_entry
`struct minitar_entry`

An entry in a tar archive. Fields:

`metadata`: The entry's metadata. (`struct minitar_entry_metadata`)

`position`: Reserved for internal use. (`fpos_t`)

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
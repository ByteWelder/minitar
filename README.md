# minitar

Tiny C library to interact with tar archives

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
			printf("Found file %s\n", entry->metadata.name);
			minitar_free_entry(entry);
		}
	} while(entry);
	minitar_close(mp);
}
```

This program will list out the files in a tar archive :)

## Functions
### minitar_open
`struct minitar* minitar_open(const char* filename)`

Opens a tar archive for reading, and returns a heap-allocated `struct minitar` which must be freed with `minitar_close()` after using it. If opening the file or allocating the struct fails, returns NULL.

A `struct minitar` is opaque, and should only be passed to other minitar functions. You should not care about its contents.

### minitar_read_entry
`struct minitar_entry* minitar_read_entry(struct minitar* mp)`

Reads the next entry from a `struct minitar` which should be the return value of a previous call to `minitar_open()`. The return value is a heap-allocated `struct minitar_entry`, which should be freed with `minitar_free_entry()` when no longer needed. 

This structure consists of the file metadata (in the `metadata` field), and a heap-allocated pointer to the file's contents (the `ptr` field), of size metadata.size + a NULL character, for convenience. This means you can use normal C string functions if you're expecting an ASCII file. Other kinds of files may have NULL characters before the end of the file, so you should assume the length of `ptr` is `metadata.size` and not `strlen(ptr)`.

This pointer will be freed when calling `minitar_free_entry()`, so if you're intending to use it later, copy its contents somewhere else.

This function returns NULL on end-of-file (when all entries have been read).

### minitar_free_entry
`void minitar_free_entry(struct minitar_entry* entry)`

Frees the heap-allocated `struct minitar_entry` and the file contents stored inside it. The pointer passed to `minitar_free_entry()` should be the return value of a previous call to `minitar_read_entry()`, `minitar_find_by_name()` or `minitar_find_any_of()`.

### minitar_rewind
`void minitar_rewind(struct minitar* mp)`

Rewinds the `struct minitar` back to the beginning of the archive file, which means that the next call to `minitar_read_entry()` will return the first entry instead of the entry after the last read entry.

### minitar_find_by_name
`struct minitar_entry* minitar_find_by_name(struct minitar* mp, const char* name)`

Returns the first entry with a matching name, or NULL if none are found. The return value is a `struct minitar_entry`, which is heap-allocated and should be freed after use with `minitar_free_entry()`. This structure is already documented in the entry documenting `minitar_read_entry()`.

This function starts searching from the current archive position, which means that to find a matching entry in the entire archive `minitar_rewind()` should be called on it first.

The state of `mp` after `minitar_find_by_name()` returns is unspecified, but a successive call to `minitar_find_by_name()` will return the next matching entry, if there is one. (Calling `minitar_find_by_name()` in a loop until it returns NULL will return all matching entries.)

In order to perform other minitar operations on the archive, `minitar_rewind()` should probably be called first, to get a known state.

### minitar_find_any_of
`struct minitar_entry* minitar_find_any_of(struct minitar* mp, enum minitar_file_type type)`

Does the same thing as `minitar_find_by_name()`, but matches the file type instead of the name. As with `minitar_find_by_name()`, this function starts searching from the current archive position and calling it in a loop until it returns NULL will return all matching entries.

### minitar_close
`int minitar_close(struct minitar* mp)`

Closes the tar archive file `mp` points to and frees the heap memory it was using. The pointer passed to `minitar_close()` should be the return value of a previous call to `minitar_open()`.

Returns 0 on success, everything else is failure and you should check `errno`.

## Types

### minitar_file_type
`enum minitar_file_type`

This enum lists all supported file types:

`MTAR_REGULAR`: Regular files

`MTAR_BLKDEV`: Block special devices

`MTAR_CHRDEV`: Character special devices

`MTAR_DIRECTORY`: Directories

Other file types supported in tar archives, such as FIFOs or symlinks, are not supported and minitar will throw an error when encountering one of them.

### minitar_entry_metadata
`struct minitar_entry_metadata`

This structure represents an entry's metadata, with the following fields:

`name`: A string representing the full path of the entry within the archive. (`char[]`)

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

`ptr`: A pointer to the entry's contents, heap-allocated. (`char*`)

More details about this structure are available in the documentation for `minitar_read_entry()`.

## License

`minitar` is free and open-source software under the [BSD-2-Clause](LICENSE) license.
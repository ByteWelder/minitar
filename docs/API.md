# minitar API documentation

Functions/types suffixed with `_w` or that contain `write` in their names are part of the newer API for writing to archives. Other types/functions are part of the (older) API for reading archives.

## Functions
### minitar_open
`int minitar_open(const char* pathname, struct minitar* mp)`

Initializes the caller-provided `mp` [handle](API.md#minitar) by opening the archive pointed to by `pathname` for reading. Returns 0 on success, anything else is failure.

### minitar_open_w
`int minitar_open_w(const char* pathname, struct minitar_w* mp, enum minitar_write_mode mode)`

Initializes the caller-provided `mp` [handle](API.md#minitar_w) by opening the archive pointed to by `pathname` for writing (in case `pathname` already exists, mode selects if the existing file is overwritten or if new entries are appended to it). Returns 0 on success, anything else is failure.

### minitar_read_entry
`int minitar_read_entry(struct minitar* mp, struct minitar_entry* out)`

Reads the next entry from a `struct minitar` which should be initialized by a previous call to `minitar_open()` and stores the result in `out`.

The `minitar_entry` structure consists of the file metadata (in the `metadata` field), and other internally-used values.

To read the contents of an entry, you should allocate a buffer large enough to hold `metadata.size` bytes and pass it to `minitar_read_contents()`.

This function returns 0 on success and -1 on end-of-file (when all entries have been read).

### minitar_write_file_entry
`int minitar_write_file_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata, char* buf)`

Writes a regular file entry into a `struct minitar_w` which should be initialized by a previous call to `minitar_open_w()`.

This function writes both a header (generated from the metadata) and the file contents in `buf`, which should be `metadata.size` bytes long.

This function will only write entries for regular files (metadata.type == `MTAR_REGULAR`). It will ignore the `type` field and write the "regular file" type into the tar archive.

To write any other kind of entry (directories, special files), use `minitar_write_special_entry`.

This function returns 0 on success.

### minitar_write_special_entry
`int minitar_write_special_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata)`

Writes a special file entry (anything that does not have contents, so directories or special files) into a `struct minitar_w` which should be initialized by a previous call to `minitar_open_w()`.

This function only writes a header (generated from the metadata). The `size` field is written as 0, no matter what its original value was.

This function does not write entries for regular files (metadata.type == `MTAR_REGULAR`). Trying to do so will result in minitar panicking (see [error handling](../README.md#error-handling)). To write regular files, use `minitar_write_file_entry`.

This function returns 0 on success.

### minitar_rewind
`void minitar_rewind(struct minitar* mp)`

Rewinds the `struct minitar` back to the beginning of the archive file, which means that the next call to `minitar_read_entry()` will fetch the first entry instead of the entry after the last read entry.

### minitar_find_by_name
`int minitar_find_by_name(struct minitar* mp, const char* name, struct minitar_entry* out)`

Stores the first entry with a matching name in `out` and returns 0, or non-zero if none are found. If none are found, the state of `out` is unspecified and might have been changed by the function. (In this context, a "name" means the base name of a file, so `baz.txt` given the path `foo/bar/baz.txt`)

This function starts searching from the current archive position, which means that to find a matching entry in the entire archive `minitar_rewind()` should be called on it first.

The state of `mp` after `minitar_find_by_name()` returns is unspecified, but a successive call to `minitar_find_by_name()` will find the next matching entry, if there is one. (Calling `minitar_find_by_name()` in a loop until it returns non-zero will return all matching entries.)

In order to perform other minitar operations on the archive, `minitar_rewind()` should probably be called first, to get a known state.

### minitar_find_by_path
`int minitar_find_by_path(struct minitar* mp, const char* path, struct minitar_entry* out)`

Same as `minitar_find_by_name()`, but matches the full path inside the archive instead of the file name.


### minitar_find_any_of
`int minitar_find_any_of(struct minitar* mp, enum minitar_file_type type, struct minitar_entry* out)`

Same as `minitar_find_by_name()`, but matches the file type instead of the name. As with `minitar_find_by_name()`, this function starts searching from the current archive position and calling it in a loop until it returns -1 will find all matching entries.

### minitar_read_contents
`size_t minitar_read_contents(struct minitar* mp, const struct minitar_entry* entry, char* buf, size_t max)`

Reads up to `max` bytes of an entry's contents from the archive stream `mp` and stores them into `buf`.

This function can be called as many times as desired, and at any given point in time, provided both `mp` and `entry` are valid. (`mp` should be initialized by a previous call to `minitar_open()`, and `entry` initialized by a previous call to `minitar_read_entry()`, `minitar_find_by_name()`, `minitar_find_by_path()` or `minitar_find_any_of()`).

This function returns the number of bytes read, or 0 on error. 0 might also be a successful return value (if `max` is 0 or the entry's size is 0, for example), which means `errno` should be checked to see if 0 means error or simply 0 bytes read.

`minitar_read_contents()` only reads up to `metadata.size`, regardless of the value in `max`.

The contents are not null-terminated. If you want null-termination (keep in mind the contents might not be ASCII and might contain null bytes before the end), just do `buf[nread] = 0;`. In that case, the value of `max` should be one less than the size of the buffer, to make sure the zero byte is not written past the end of `buf` if `max` bytes are read.

### minitar_close
`int minitar_close(struct minitar* mp)`

Closes the tar archive file `mp` points to. The pointer passed to `minitar_close()` should be initialized by a previous call to `minitar_open()`.

Returns 0 on success, everything else is failure and you should check `errno`.

### minitar_close_w
`int minitar_close_w(struct minitar_w* mp)`

Closes the tar archive file `mp` points to. The pointer passed to `minitar_close_w()` should be initialized by a previous call to `minitar_open_w()`.

Returns 0 on success, everything else is failure and you should check `errno`.

## Types

### minitar_file_type
`enum minitar_file_type`

This enum lists all supported file types:

`MTAR_REGULAR`: Regular files

`MTAR_DIRECTORY`: Directories

`MTAR_SYMLINK`: Symbolic links

`MTAR_HARDLINK`: Hard links

`MTAR_FIFO`: FIFO special files

`MTAR_BLKDEV`: Block devices

`MTAR_CHRDEV`: Character devices

### minitar_write_mode
`enum minitar_write_mode`

This enum tells `minitar_open_w` what to do if the chosen archive path already exists:

`MTAR_OVERWRITE`: Overwrite the archive

`MTAR_APPEND`: Add new entries to the end of it

### minitar_entry_metadata
`struct minitar_entry_metadata`

This structure represents an entry's metadata, with the following fields:

`path`: A string representing the full path of the entry within the archive. (`char[]`)

`name`: A string representing the base name of the entry (the last component of its path). (`char[]`)

`link`: A string representing the file being linked to. (Only applies to symlinks/hard links) (`char[]`)

`mode`: An integer representing the permissions of the entry. (`mode_t`)

`uid`: An integer representing the user ID of the entry's owner. (`uid_t`)

`gid`: An integer representing the group ID of the entry's owner. (`gid_t`)

`size`: An integer representing the size of the entry's contents in bytes. (`size_t`)

`mtime`: A UNIX timestamp representing the last time the entry was modified. (`time_t`)

`type`: An enum representing the type of the entry. (`enum minitar_file_type`)

`uname`: A string representing the username of the entry's owner. (`char[]`)

`gname`: A string representing the group name of the entry's owner. (`char[]`)

`devmajor`: An integer representing the major number of a device. (`unsigned int`)

`devminor`: An integer representing the minor number of a device. (`unsigned int`)

### minitar_entry
`struct minitar_entry`

An entry in a tar archive. Fields:

`metadata`: The entry's metadata. (`struct minitar_entry_metadata`)

`_internal`: Reserved for internal use. (`struct minitar_entry_internal`)

### minitar
`struct minitar`

An archive handle for the "reading" API. To write to an archive, use `struct minitar_w` and `minitar_open_w()` instead.

### minitar_w
`struct minitar_w`

An archive handle for the "writing" API. To read from an archive, use `struct minitar` and `minitar_open()` instead.
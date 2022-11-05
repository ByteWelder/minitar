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
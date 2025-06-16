#include "file.h"

bool
file_read(const char* path, void* buffer, size_t size)
{
	SDL_IOStream* ioStream;

	memset(buffer, '\0', size);

	ioStream = SDL_IOFromFile(path, "r");

	if (!ioStream)
	{
		printf(STRING_ERROR_FILE_READ, path);
		return false;
	}

	if (!SDL_ReadIO(ioStream, buffer, size))
	{
		printf(STRING_ERROR_FILE_READ, path);
		return false;
	}
	
	SDL_CloseIO(ioStream);

	return true;
}


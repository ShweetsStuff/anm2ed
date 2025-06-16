#include "anm2.h"

bool
anm2_init(Anm2* self, const char* path)
{
	xml_document document;
	xml_parse_result result;

	memset(self, '\0', sizeof(Anm2));

	result = document.load_file(path);

	if (!result)
	{
		printf(STRING_ERROR_ANM2_INIT, path, result.description());
		return false;
	}

  	for (pugi::xml_node node : document.children()) 
		printf("%s\n", node.name());

	printf(STRING_INFO_ANM2_INIT, path);

	return true;
}

void
anm2_free(Anm2* self)
{


}

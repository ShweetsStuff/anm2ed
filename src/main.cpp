#include "main.h"

static bool _anm2_rescale(const std::string& file, f32 scale)
{
	Anm2 anm2;

	if (!anm2_deserialize(&anm2, nullptr, file)) return false;
	anm2_scale(&anm2, scale);
	return anm2_serialize(&anm2, file);
}

s32
main(s32 argc, char* argv[])
{
	State state;

	if (argc > 0 && argv[1])
	{
		if (std::string(argv[1]) == ARGUMENT_RESCALE)
		{
			if (argv[2] && argv[3])
			{
				if (_anm2_rescale(std::string(argv[2]), atof(argv[3])))
				{
					log_info(std::format(ARGUMENT_RESCALE_ANM2_INFO, argv[2], argv[3]));
					return EXIT_SUCCESS;
				}
				else
					log_error(ARGUMENT_RESCALE_ANM2_ERROR);
			}
			else
				log_error(ARGUMENT_RESCALE_ARGUMENT_ERROR);
			
			return EXIT_FAILURE;
		}
		else
			if (argv[1])
				state.argument = argv[1];
	}

	init(&state);

	while (state.isRunning)
		loop(&state);

	quit(&state);

	return EXIT_SUCCESS;
}
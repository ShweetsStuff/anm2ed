// Main function

#include "main.h"

s32
main(s32 argc, char* argv[])
{
	State state;

	if (argc > 0 && argv[1])
		state.argument = argv[1];

	init(&state);

	while (state.isRunning)
		loop(&state);

	quit(&state);

	return EXIT_SUCCESS;
}
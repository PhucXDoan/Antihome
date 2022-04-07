#include "unified.h"
#include "platform.h"

#define PRESSED(INPUT) (platform->inputs[+(INPUT)].curr && !platform->inputs[+(INPUT)].prev)
#define HELD(INPUT)    (platform->inputs[+(INPUT)].curr)

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
}

extern "C" PROTOTYPE_UPDATE(update)
{
	if (PRESSED(Input::q))
	{
		DEBUG_printf("q\n");
	}

	if (HELD(Input::e))
	{
		DEBUG_printf("e\n");
	}

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
}

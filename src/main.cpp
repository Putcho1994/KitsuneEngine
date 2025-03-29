

#include <kitsune_engine.hpp>

int main(int argc, char* argv[])
{
	KitsuneEngine engine;

	engine.init();

	engine.run();

	engine.cleanup();

	return 0;
}
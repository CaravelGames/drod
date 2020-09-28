#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_COLOUR_WINDOWS
#include "catch.hpp"

#include <iostream>
#include "CTestDb.h"
#include "Runner.h"
#undef main

int main(int argc, char* const argv[])
{
	CTestDb::Init(argc, argv);
	Runner::InitializeDatPath();

	int result = Catch::Session().run(argc, argv);

	CTestDb::Teardown();

	system("pause");

	return result;
}

#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_COLOUR_WINDOWS
#include "catch.hpp"

#include <iostream>
#include "CTestDb.h"
#include "Runner.h"
#undef main

// Used to extract test name to name the Level when saving the hold
struct MyListener : Catch::TestEventListenerBase {

	using TestEventListenerBase::TestEventListenerBase; // inherit constructor

	void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
		CTestDb::currentTestCaseName = &(testInfo.name);
	}
};
CATCH_REGISTER_LISTENER(MyListener)

int main(int argc, char* const argv[])
{
	CTestDb::Init(argc, argv);
	Runner::InitializeDatPath();

	int result = Catch::Session().run(argc, argv);

	CTestDb::Teardown();

	system("pause");

	return result;
}

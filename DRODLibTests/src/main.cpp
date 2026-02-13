#define CATCH_CONFIG_RUNNER

// Breaks linux builds
#if !defined(__linux__) &&  !defined(__linux) && !defined(linux)
#define CATCH_CONFIG_COLOUR_WINDOWS
#endif

#include "catch.hpp"

#include <iostream>
#include "CTestDb.h"
#include "Runner.h"
#undef main

// Used to extract test name to name the Level when saving the hold
struct MyListener : Catch::TestEventListenerBase {

	using TestEventListenerBase::TestEventListenerBase; // inherit constructor

	virtual void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
		// printf("Starting test %s:\n", testInfo.name.c_str());
		CTestDb::currentTestCaseName = &(testInfo.name);
	}
	virtual void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
		// printf("  - [SECTION START] %s\n", sectionInfo.name.c_str());
	}
	virtual void sectionEnded(Catch::SectionStats const& sectionStats) override {
		// printf("  - [SECTION END] %s\n", sectionStats.sectionInfo.name.c_str());
	}
};
CATCH_REGISTER_LISTENER(MyListener)

int main(int argc, char* const argv[])
{
	CTestDb::Init(argc, argv);
	Runner::InitializeDatPath();

	int result = Catch::Session().run(argc, argv);

	CTestDb::Teardown();

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
	system("pause");
#endif

	return result;
}

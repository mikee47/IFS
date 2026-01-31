/*
 * Errors.cpp
 *
 *  Created on: 26 Mar 2021
 *      Author: mikee47
 */

#include <FsTest.h>
#include <IFS/Host/Util.h>

class ErrorTest : public TestGroup
{
public:
	ErrorTest() : TestGroup(_F("Errors"))
	{
	}

	void execute() override
	{
#ifdef ARCH_HOST
		int err = IFS::Error::fromSystem(-EINVAL);
		String s = IFS::Host::getErrorString(err);
		Serial << _F("EINVAL: ") << s << endl;
		CHECK(s == F("Invalid argument"));
#endif
	}
};

void REGISTER_TEST(Errors)
{
	registerGroup<ErrorTest>();
}

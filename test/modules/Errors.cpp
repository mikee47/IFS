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
		int err = IFS::Error::fromSystem(-EINVAL);
		String s = IFS::Host::getErrorString(err);
		Serial.print("EINVAL: ");
		Serial.println(s);
		CHECK(s == F("Invalid argument"));
	}

	void execute() override
	{
	}
};

void REGISTER_TEST(Errors)
{
	registerGroup<ErrorTest>();
}

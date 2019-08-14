
#include "TimeStamp.h"
#include "strings/StringPrintf.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	// TimeStamp
	TimeStamp time = TimeStamp::now();
	cout << "now(utc):" << time << endl;

	time += TimeDelta::from_hours(10);
	cout << "+10:" << time << endl;

	cout << "midnight:" << time.utc_midnight() << endl;

	// Exploded
	time = TimeStamp::now();
	TimeStamp::Exploded local_exploded;
	time.to_local_explode(&local_exploded);
	cout << string_printf("%04d-%02d-%02d %02d:%02d:%02d",
					local_exploded.year,
					local_exploded.month,
					local_exploded.day_of_month,
					local_exploded.hour,
					local_exploded.minute,
					local_exploded.second)
	<< endl;
}

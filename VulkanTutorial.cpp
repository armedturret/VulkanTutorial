#include "MainApplication.hpp"

#include <stdexcept>
#include <iostream>

using namespace std;

int main()
{
	MainApplication mainApp;

	try
	{
		mainApp.run();
	}
	catch (const exception& e)
	{
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}

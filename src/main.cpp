#include "Application.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        Application application(&argc, argv);
        application.run();
    }
    catch (const vsg::Exception& exception)
    {
        std::cerr << exception.message << '\n';
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
    }

    return 0;
}

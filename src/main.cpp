#include "Engine/Core/Application.hpp"

int main(int, char**)
{
    ember::Application application;

    if (!application.Initialize())
    {
        return 1;
    }

    application.Run();
    return 0;
}

#include "Editor/EditorApplication.hpp"

int main(int, char**)
{
    ember::EditorApplication application;
    if (!application.Initialize())
    {
        return 1;
    }

    application.Run();
    return 0;
}

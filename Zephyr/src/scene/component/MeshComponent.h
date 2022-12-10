#pragma once
#include "Component.h"

namespace Zephyr
{
    class Mesh;

    COMPONENT(MeshComponent)
    {
        Mesh* mesh;
    };
}
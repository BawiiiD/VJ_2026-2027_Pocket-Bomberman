#include "Gameplay.h"

bool overlaps(const glm::vec4 &first, const glm::vec4 &second)
{
    const bool overlapX =
        first.x < second.x + second.z &&
        first.x + first.z > second.x;

    const bool overlapY =
        first.y < second.y + second.w &&
        first.y + first.w > second.y;

    return overlapX && overlapY;
}

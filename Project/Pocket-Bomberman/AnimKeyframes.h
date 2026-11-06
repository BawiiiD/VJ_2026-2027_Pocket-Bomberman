#ifndef _ANIMATION_KEYFRAMES
#define _ANIMATION_KEYFRAMES

#include <vector>
#include <glm/glm.hpp>

using namespace std;

// One animation of a Sprite: the list of frames and how long each one stays
// on screen
struct AnimKeyframes
{
    // How long each frame is shown, in milliseconds
    float millisecsPerKeyframe;
    // For each frame, the texture coordinates of its top-left corner
    vector<glm::vec2> keyframeDispl;
};

#endif

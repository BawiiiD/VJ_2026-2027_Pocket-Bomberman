#ifndef _ANIMATION_KEYFRAMES
#define _ANIMATION_KEYFRAMES

#include <vector>
#include <glm/glm.hpp>

using namespace std;

// A struct to hold the data for each animation
struct AnimKeyframes
{
    // The time in milliseconds between each keyframe
    float millisecsPerKeyframe;
    // The displacement of the sprite at each keyframe
    vector<glm::vec2> keyframeDispl;
};

#endif
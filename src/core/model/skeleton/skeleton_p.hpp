#pragma once
#include "skeleton.hpp"

#include <set>

class glaxnimate::model::Skeleton::Private
{
public:
    std::set<model::Bone*> bones;
    std::set<model::SkinSlot*> skin_slots;
    std::set<model::SkinAttachment*> attachments;
};

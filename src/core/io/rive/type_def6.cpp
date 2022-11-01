/**
 * NOTE: This file is generated automatically, do not edit manually
 * To generate this file run
 *       ./external/rive_typedef.py -t source -v6 >src/core/io/rive/type_def6.cpp
 */


#include "type_def.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<TypeId, ObjectDefinition> glaxnimate::io::rive::defined_objects6 = {
    {
        TypeId::Artboard, {
            "Artboard", TypeId::Artboard,
            TypeId::ContainerComponent, {
                {"width", 7, PropertyType::Float},
                {"height", 8, PropertyType::Float},
                {"x", 9, PropertyType::Float},
                {"y", 10, PropertyType::Float},
                {"originX", 11, PropertyType::Float},
                {"originY", 12, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Node, {
            "Node", TypeId::Node,
            TypeId::TransformComponent, {
                {"x", 13, PropertyType::Float},
                {"y", 14, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Shape, {
            "Shape", TypeId::Shape,
            TypeId::Drawable, {
            }
        }
    },
    {
        TypeId::Ellipse, {
            "Ellipse", TypeId::Ellipse,
            TypeId::ParametricPath, {
            }
        }
    },
    {
        TypeId::StraightVertex, {
            "StraightVertex", TypeId::StraightVertex,
            TypeId::PathVertex, {
                {"radius", 26, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicDetachedVertex, {
            "CubicDetachedVertex", TypeId::CubicDetachedVertex,
            TypeId::CubicVertex, {
                {"inRotation", 84, PropertyType::Float},
                {"inDistance", 85, PropertyType::Float},
                {"outRotation", 86, PropertyType::Float},
                {"outDistance", 87, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Rectangle, {
            "Rectangle", TypeId::Rectangle,
            TypeId::ParametricPath, {
                {"cornerRadius", 31, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Triangle, {
            "Triangle", TypeId::Triangle,
            TypeId::ParametricPath, {
            }
        }
    },
    {
        TypeId::PathComposer, {
            "PathComposer", TypeId::PathComposer,
            TypeId::Component, {
            }
        }
    },
    {
        TypeId::Component, {
            "Component", TypeId::Component,
            TypeId::NoType, {
                {"name", 4, PropertyType::String},
                {"parentId", 5, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ContainerComponent, {
            "ContainerComponent", TypeId::ContainerComponent,
            TypeId::Component, {
            }
        }
    },
    {
        TypeId::Path, {
            "Path", TypeId::Path,
            TypeId::Node, {
                {"pathFlags", 128, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Drawable, {
            "Drawable", TypeId::Drawable,
            TypeId::Node, {
                {"blendModeValue", 23, PropertyType::VarUint},
                {"drawableFlags", 129, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::PathVertex, {
            "PathVertex", TypeId::PathVertex,
            TypeId::ContainerComponent, {
                {"x", 24, PropertyType::Float},
                {"y", 25, PropertyType::Float},
            }
        }
    },
    {
        TypeId::ParametricPath, {
            "ParametricPath", TypeId::ParametricPath,
            TypeId::Path, {
                {"width", 20, PropertyType::Float},
                {"height", 21, PropertyType::Float},
                {"originX", 123, PropertyType::Float},
                {"originY", 124, PropertyType::Float},
            }
        }
    },
    {
        TypeId::PointsPath, {
            "PointsPath", TypeId::PointsPath,
            TypeId::Path, {
                {"isClosed", 32, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::RadialGradient, {
            "RadialGradient", TypeId::RadialGradient,
            TypeId::LinearGradient, {
            }
        }
    },
    {
        TypeId::SolidColor, {
            "SolidColor", TypeId::SolidColor,
            TypeId::Component, {
                {"colorValue", 37, PropertyType::Color},
            }
        }
    },
    {
        TypeId::GradientStop, {
            "GradientStop", TypeId::GradientStop,
            TypeId::Component, {
                {"colorValue", 38, PropertyType::Color},
                {"position", 39, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Fill, {
            "Fill", TypeId::Fill,
            TypeId::ShapePaint, {
                {"fillRule", 40, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ShapePaint, {
            "ShapePaint", TypeId::ShapePaint,
            TypeId::ContainerComponent, {
                {"isVisible", 41, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::LinearGradient, {
            "LinearGradient", TypeId::LinearGradient,
            TypeId::ContainerComponent, {
                {"startX", 42, PropertyType::Float},
                {"startY", 33, PropertyType::Float},
                {"endX", 34, PropertyType::Float},
                {"endY", 35, PropertyType::Float},
                {"opacity", 46, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Backboard, {
            "Backboard", TypeId::Backboard,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::Stroke, {
            "Stroke", TypeId::Stroke,
            TypeId::ShapePaint, {
                {"thickness", 47, PropertyType::Float},
                {"cap", 48, PropertyType::VarUint},
                {"join", 49, PropertyType::VarUint},
                {"transformAffectsStroke", 50, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::KeyedObject, {
            "KeyedObject", TypeId::KeyedObject,
            TypeId::NoType, {
                {"objectId", 51, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyedProperty, {
            "KeyedProperty", TypeId::KeyedProperty,
            TypeId::NoType, {
                {"propertyKey", 53, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Animation, {
            "Animation", TypeId::Animation,
            TypeId::NoType, {
                {"name", 55, PropertyType::String},
            }
        }
    },
    {
        TypeId::CubicInterpolator, {
            "CubicInterpolator", TypeId::CubicInterpolator,
            TypeId::NoType, {
                {"x1", 63, PropertyType::Float},
                {"y1", 64, PropertyType::Float},
                {"x2", 65, PropertyType::Float},
                {"y2", 66, PropertyType::Float},
            }
        }
    },
    {
        TypeId::KeyFrame, {
            "KeyFrame", TypeId::KeyFrame,
            TypeId::NoType, {
                {"frame", 67, PropertyType::VarUint},
                {"interpolationType", 68, PropertyType::VarUint},
                {"interpolatorId", 69, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyFrameDouble, {
            "KeyFrameDouble", TypeId::KeyFrameDouble,
            TypeId::KeyFrame, {
                {"value", 70, PropertyType::Float},
            }
        }
    },
    {
        TypeId::LinearAnimation, {
            "LinearAnimation", TypeId::LinearAnimation,
            TypeId::Animation, {
                {"fps", 56, PropertyType::VarUint},
                {"duration", 57, PropertyType::VarUint},
                {"speed", 58, PropertyType::Float},
                {"loopValue", 59, PropertyType::VarUint},
                {"workStart", 60, PropertyType::VarUint},
                {"workEnd", 61, PropertyType::VarUint},
                {"enableWorkArea", 62, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::CubicAsymmetricVertex, {
            "CubicAsymmetricVertex", TypeId::CubicAsymmetricVertex,
            TypeId::CubicVertex, {
                {"rotation", 79, PropertyType::Float},
                {"inDistance", 80, PropertyType::Float},
                {"outDistance", 81, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicMirroredVertex, {
            "CubicMirroredVertex", TypeId::CubicMirroredVertex,
            TypeId::CubicVertex, {
                {"rotation", 82, PropertyType::Float},
                {"distance", 83, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicVertex, {
            "CubicVertex", TypeId::CubicVertex,
            TypeId::PathVertex, {
            }
        }
    },
    {
        TypeId::KeyFrameColor, {
            "KeyFrameColor", TypeId::KeyFrameColor,
            TypeId::KeyFrame, {
                {"value", 88, PropertyType::Color},
            }
        }
    },
    {
        TypeId::TransformComponent, {
            "TransformComponent", TypeId::TransformComponent,
            TypeId::ContainerComponent, {
                {"rotation", 15, PropertyType::Float},
                {"scaleX", 16, PropertyType::Float},
                {"scaleY", 17, PropertyType::Float},
                {"opacity", 18, PropertyType::Float},
            }
        }
    },
    {
        TypeId::SkeletalComponent, {
            "SkeletalComponent", TypeId::SkeletalComponent,
            TypeId::TransformComponent, {
            }
        }
    },
    {
        TypeId::Bone, {
            "Bone", TypeId::Bone,
            TypeId::SkeletalComponent, {
                {"length", 89, PropertyType::Float},
            }
        }
    },
    {
        TypeId::RootBone, {
            "RootBone", TypeId::RootBone,
            TypeId::Bone, {
                {"x", 90, PropertyType::Float},
                {"y", 91, PropertyType::Float},
            }
        }
    },
    {
        TypeId::ClippingShape, {
            "ClippingShape", TypeId::ClippingShape,
            TypeId::Component, {
                {"sourceId", 92, PropertyType::VarUint},
                {"fillRule", 93, PropertyType::VarUint},
                {"isVisible", 94, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::Skin, {
            "Skin", TypeId::Skin,
            TypeId::ContainerComponent, {
                {"xx", 104, PropertyType::Float},
                {"yx", 105, PropertyType::Float},
                {"xy", 106, PropertyType::Float},
                {"yy", 107, PropertyType::Float},
                {"tx", 108, PropertyType::Float},
                {"ty", 109, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Tendon, {
            "Tendon", TypeId::Tendon,
            TypeId::Component, {
                {"boneId", 95, PropertyType::VarUint},
                {"xx", 96, PropertyType::Float},
                {"yx", 97, PropertyType::Float},
                {"xy", 98, PropertyType::Float},
                {"yy", 99, PropertyType::Float},
                {"tx", 100, PropertyType::Float},
                {"ty", 101, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Weight, {
            "Weight", TypeId::Weight,
            TypeId::Component, {
                {"values", 102, PropertyType::VarUint},
                {"indices", 103, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::CubicWeight, {
            "CubicWeight", TypeId::CubicWeight,
            TypeId::Weight, {
                {"inValues", 110, PropertyType::VarUint},
                {"inIndices", 111, PropertyType::VarUint},
                {"outValues", 112, PropertyType::VarUint},
                {"outIndices", 113, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TrimPath, {
            "TrimPath", TypeId::TrimPath,
            TypeId::Component, {
                {"start", 114, PropertyType::Float},
                {"end", 115, PropertyType::Float},
                {"offset", 116, PropertyType::Float},
                {"modeValue", 117, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawTarget, {
            "DrawTarget", TypeId::DrawTarget,
            TypeId::Component, {
                {"drawableId", 119, PropertyType::VarUint},
                {"placementValue", 120, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawRules, {
            "DrawRules", TypeId::DrawRules,
            TypeId::ContainerComponent, {
                {"drawTargetId", 121, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyFrameId, {
            "KeyFrameId", TypeId::KeyFrameId,
            TypeId::KeyFrame, {
                {"value", 122, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Polygon, {
            "Polygon", TypeId::Polygon,
            TypeId::ParametricPath, {
                {"points", 125, PropertyType::VarUint},
                {"cornerRadius", 126, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Star, {
            "Star", TypeId::Star,
            TypeId::Polygon, {
                {"innerRadius", 127, PropertyType::Float},
            }
        }
    },
    {
        TypeId::StateMachine, {
            "StateMachine", TypeId::StateMachine,
            TypeId::Animation, {
            }
        }
    },
    {
        TypeId::StateMachineComponent, {
            "StateMachineComponent", TypeId::StateMachineComponent,
            TypeId::NoType, {
                {"name", 138, PropertyType::String},
            }
        }
    },
    {
        TypeId::StateMachineInput, {
            "StateMachineInput", TypeId::StateMachineInput,
            TypeId::StateMachineComponent, {
            }
        }
    },
    {
        TypeId::StateMachineDouble, {
            "StateMachineDouble", TypeId::StateMachineDouble,
            TypeId::StateMachineInput, {
                {"value", 140, PropertyType::Float},
            }
        }
    },
    {
        TypeId::StateMachineLayer, {
            "StateMachineLayer", TypeId::StateMachineLayer,
            TypeId::StateMachineComponent, {
            }
        }
    },
    {
        TypeId::StateMachineTrigger, {
            "StateMachineTrigger", TypeId::StateMachineTrigger,
            TypeId::StateMachineInput, {
            }
        }
    },
    {
        TypeId::StateMachineBool, {
            "StateMachineBool", TypeId::StateMachineBool,
            TypeId::StateMachineInput, {
                {"value", 141, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::LayerState, {
            "LayerState", TypeId::LayerState,
            TypeId::StateMachineLayerComponent, {
            }
        }
    },
    {
        TypeId::AnimationState, {
            "AnimationState", TypeId::AnimationState,
            TypeId::LayerState, {
                {"animationId", 149, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::AnyState, {
            "AnyState", TypeId::AnyState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::EntryState, {
            "EntryState", TypeId::EntryState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::ExitState, {
            "ExitState", TypeId::ExitState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::StateTransition, {
            "StateTransition", TypeId::StateTransition,
            TypeId::StateMachineLayerComponent, {
                {"stateToId", 151, PropertyType::VarUint},
                {"flags", 152, PropertyType::VarUint},
                {"duration", 158, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::StateMachineLayerComponent, {
            "StateMachineLayerComponent", TypeId::StateMachineLayerComponent,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::TransitionCondition, {
            "TransitionCondition", TypeId::TransitionCondition,
            TypeId::NoType, {
                {"inputId", 155, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TransitionTriggerCondition, {
            "TransitionTriggerCondition", TypeId::TransitionTriggerCondition,
            TypeId::TransitionCondition, {
            }
        }
    },
    {
        TypeId::TransitionValueCondition, {
            "TransitionValueCondition", TypeId::TransitionValueCondition,
            TypeId::TransitionCondition, {
                {"opValue", 156, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TransitionDoubleCondition, {
            "TransitionDoubleCondition", TypeId::TransitionDoubleCondition,
            TypeId::TransitionValueCondition, {
                {"value", 157, PropertyType::Float},
            }
        }
    },
    {
        TypeId::TransitionBoolCondition, {
            "TransitionBoolCondition", TypeId::TransitionBoolCondition,
            TypeId::TransitionValueCondition, {
            }
        }
    },
};

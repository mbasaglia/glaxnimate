/**
 * NOTE: This file is generated automatically, do not edit manually
 * To generate this file run
 *       ./external/rive_typedef.py -t source >src/core/io/rive/type_def.cpp
 */


#include "type_def.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<TypeId, ObjectDefinition> glaxnimate::io::rive::defined_objects = {
    {
        TypeId::Artboard, {
            "Artboard", TypeId::Artboard,
            TypeId::WorldTransformComponent, {
                {196, {"clip", PropertyType::Bool}},
                {7, {"width", PropertyType::Float}},
                {8, {"height", PropertyType::Float}},
                {9, {"x", PropertyType::Float}},
                {10, {"y", PropertyType::Float}},
                {11, {"originX", PropertyType::Float}},
                {12, {"originY", PropertyType::Float}},
                {236, {"defaultStateMachineId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Node, {
            "Node", TypeId::Node,
            TypeId::TransformComponent, {
                {13, {"x", PropertyType::Float}},
                {14, {"y", PropertyType::Float}},
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
                {26, {"radius", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::CubicDetachedVertex, {
            "CubicDetachedVertex", TypeId::CubicDetachedVertex,
            TypeId::CubicVertex, {
                {84, {"inRotation", PropertyType::Float}},
                {85, {"inDistance", PropertyType::Float}},
                {86, {"outRotation", PropertyType::Float}},
                {87, {"outDistance", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Rectangle, {
            "Rectangle", TypeId::Rectangle,
            TypeId::ParametricPath, {
                {164, {"linkCornerRadius", PropertyType::Bool}},
                {31, {"cornerRadiusTL", PropertyType::Float}},
                {161, {"cornerRadiusTR", PropertyType::Float}},
                {162, {"cornerRadiusBL", PropertyType::Float}},
                {163, {"cornerRadiusBR", PropertyType::Float}},
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
        TypeId::Component, {
            "Component", TypeId::Component,
            TypeId::NoType, {
                {4, {"name", PropertyType::String}},
                {5, {"parentId", PropertyType::VarUint}},
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
                {128, {"pathFlags", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Drawable, {
            "Drawable", TypeId::Drawable,
            TypeId::Node, {
                {23, {"blendModeValue", PropertyType::VarUint}},
                {129, {"drawableFlags", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::PathVertex, {
            "PathVertex", TypeId::PathVertex,
            TypeId::Vertex, {
            }
        }
    },
    {
        TypeId::ParametricPath, {
            "ParametricPath", TypeId::ParametricPath,
            TypeId::Path, {
                {20, {"width", PropertyType::Float}},
                {21, {"height", PropertyType::Float}},
                {123, {"originX", PropertyType::Float}},
                {124, {"originY", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::PointsPath, {
            "PointsPath", TypeId::PointsPath,
            TypeId::Path, {
                {32, {"isClosed", PropertyType::Bool}},
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
                {37, {"colorValue", PropertyType::Color}},
            }
        }
    },
    {
        TypeId::GradientStop, {
            "GradientStop", TypeId::GradientStop,
            TypeId::Component, {
                {38, {"colorValue", PropertyType::Color}},
                {39, {"position", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Fill, {
            "Fill", TypeId::Fill,
            TypeId::ShapePaint, {
                {40, {"fillRule", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::ShapePaint, {
            "ShapePaint", TypeId::ShapePaint,
            TypeId::ContainerComponent, {
                {41, {"isVisible", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::LinearGradient, {
            "LinearGradient", TypeId::LinearGradient,
            TypeId::ContainerComponent, {
                {42, {"startX", PropertyType::Float}},
                {33, {"startY", PropertyType::Float}},
                {34, {"endX", PropertyType::Float}},
                {35, {"endY", PropertyType::Float}},
                {46, {"opacity", PropertyType::Float}},
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
                {47, {"thickness", PropertyType::Float}},
                {48, {"cap", PropertyType::VarUint}},
                {49, {"join", PropertyType::VarUint}},
                {50, {"transformAffectsStroke", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::KeyedObject, {
            "KeyedObject", TypeId::KeyedObject,
            TypeId::NoType, {
                {51, {"objectId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::KeyedProperty, {
            "KeyedProperty", TypeId::KeyedProperty,
            TypeId::NoType, {
                {53, {"propertyKey", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Animation, {
            "Animation", TypeId::Animation,
            TypeId::NoType, {
                {55, {"name", PropertyType::String}},
            }
        }
    },
    {
        TypeId::CubicInterpolator, {
            "CubicInterpolator", TypeId::CubicInterpolator,
            TypeId::NoType, {
                {63, {"x1", PropertyType::Float}},
                {64, {"y1", PropertyType::Float}},
                {65, {"x2", PropertyType::Float}},
                {66, {"y2", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::KeyFrame, {
            "KeyFrame", TypeId::KeyFrame,
            TypeId::NoType, {
                {67, {"frame", PropertyType::VarUint}},
                {68, {"interpolationType", PropertyType::VarUint}},
                {69, {"interpolatorId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::KeyFrameDouble, {
            "KeyFrameDouble", TypeId::KeyFrameDouble,
            TypeId::KeyFrame, {
                {70, {"value", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::LinearAnimation, {
            "LinearAnimation", TypeId::LinearAnimation,
            TypeId::Animation, {
                {56, {"fps", PropertyType::VarUint}},
                {57, {"duration", PropertyType::VarUint}},
                {58, {"speed", PropertyType::Float}},
                {59, {"loopValue", PropertyType::VarUint}},
                {60, {"workStart", PropertyType::VarUint}},
                {61, {"workEnd", PropertyType::VarUint}},
                {62, {"enableWorkArea", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::CubicAsymmetricVertex, {
            "CubicAsymmetricVertex", TypeId::CubicAsymmetricVertex,
            TypeId::CubicVertex, {
                {79, {"rotation", PropertyType::Float}},
                {80, {"inDistance", PropertyType::Float}},
                {81, {"outDistance", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::CubicMirroredVertex, {
            "CubicMirroredVertex", TypeId::CubicMirroredVertex,
            TypeId::CubicVertex, {
                {82, {"rotation", PropertyType::Float}},
                {83, {"distance", PropertyType::Float}},
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
                {88, {"value", PropertyType::Color}},
            }
        }
    },
    {
        TypeId::TransformComponent, {
            "TransformComponent", TypeId::TransformComponent,
            TypeId::WorldTransformComponent, {
                {15, {"rotation", PropertyType::Float}},
                {16, {"scaleX", PropertyType::Float}},
                {17, {"scaleY", PropertyType::Float}},
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
                {89, {"length", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::RootBone, {
            "RootBone", TypeId::RootBone,
            TypeId::Bone, {
                {90, {"x", PropertyType::Float}},
                {91, {"y", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::ClippingShape, {
            "ClippingShape", TypeId::ClippingShape,
            TypeId::Component, {
                {92, {"sourceId", PropertyType::VarUint}},
                {93, {"fillRule", PropertyType::VarUint}},
                {94, {"isVisible", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::Skin, {
            "Skin", TypeId::Skin,
            TypeId::ContainerComponent, {
                {104, {"xx", PropertyType::Float}},
                {105, {"yx", PropertyType::Float}},
                {106, {"xy", PropertyType::Float}},
                {107, {"yy", PropertyType::Float}},
                {108, {"tx", PropertyType::Float}},
                {109, {"ty", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Tendon, {
            "Tendon", TypeId::Tendon,
            TypeId::Component, {
                {95, {"boneId", PropertyType::VarUint}},
                {96, {"xx", PropertyType::Float}},
                {97, {"yx", PropertyType::Float}},
                {98, {"xy", PropertyType::Float}},
                {99, {"yy", PropertyType::Float}},
                {100, {"tx", PropertyType::Float}},
                {101, {"ty", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Weight, {
            "Weight", TypeId::Weight,
            TypeId::Component, {
                {102, {"values", PropertyType::VarUint}},
                {103, {"indices", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::CubicWeight, {
            "CubicWeight", TypeId::CubicWeight,
            TypeId::Weight, {
                {110, {"inValues", PropertyType::VarUint}},
                {111, {"inIndices", PropertyType::VarUint}},
                {112, {"outValues", PropertyType::VarUint}},
                {113, {"outIndices", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::TrimPath, {
            "TrimPath", TypeId::TrimPath,
            TypeId::Component, {
                {114, {"start", PropertyType::Float}},
                {115, {"end", PropertyType::Float}},
                {116, {"offset", PropertyType::Float}},
                {117, {"modeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::DrawTarget, {
            "DrawTarget", TypeId::DrawTarget,
            TypeId::Component, {
                {119, {"drawableId", PropertyType::VarUint}},
                {120, {"placementValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::DrawRules, {
            "DrawRules", TypeId::DrawRules,
            TypeId::ContainerComponent, {
                {121, {"drawTargetId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::KeyFrameId, {
            "KeyFrameId", TypeId::KeyFrameId,
            TypeId::KeyFrame, {
                {122, {"value", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Polygon, {
            "Polygon", TypeId::Polygon,
            TypeId::ParametricPath, {
                {125, {"points", PropertyType::VarUint}},
                {126, {"cornerRadius", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Star, {
            "Star", TypeId::Star,
            TypeId::Polygon, {
                {127, {"innerRadius", PropertyType::Float}},
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
                {138, {"name", PropertyType::String}},
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
        TypeId::StateMachineNumber, {
            "StateMachineNumber", TypeId::StateMachineNumber,
            TypeId::StateMachineInput, {
                {140, {"value", PropertyType::Float}},
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
                {141, {"value", PropertyType::Bool}},
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
                {149, {"animationId", PropertyType::VarUint}},
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
                {151, {"stateToId", PropertyType::VarUint}},
                {152, {"flags", PropertyType::VarUint}},
                {158, {"duration", PropertyType::VarUint}},
                {160, {"exitTime", PropertyType::VarUint}},
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
                {155, {"inputId", PropertyType::VarUint}},
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
                {156, {"opValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::TransitionNumberCondition, {
            "TransitionNumberCondition", TypeId::TransitionNumberCondition,
            TypeId::TransitionValueCondition, {
                {157, {"value", PropertyType::Float}},
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
    {
        TypeId::BlendState, {
            "BlendState", TypeId::BlendState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::BlendStateDirect, {
            "BlendStateDirect", TypeId::BlendStateDirect,
            TypeId::BlendState, {
            }
        }
    },
    {
        TypeId::BlendAnimation, {
            "BlendAnimation", TypeId::BlendAnimation,
            TypeId::NoType, {
                {165, {"animationId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::BlendAnimation1D, {
            "BlendAnimation1D", TypeId::BlendAnimation1D,
            TypeId::BlendAnimation, {
                {166, {"value", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::BlendState1D, {
            "BlendState1D", TypeId::BlendState1D,
            TypeId::BlendState, {
                {167, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::BlendAnimationDirect, {
            "BlendAnimationDirect", TypeId::BlendAnimationDirect,
            TypeId::BlendAnimation, {
                {168, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::BlendStateTransition, {
            "BlendStateTransition", TypeId::BlendStateTransition,
            TypeId::StateTransition, {
                {171, {"exitBlendAnimationId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Constraint, {
            "Constraint", TypeId::Constraint,
            TypeId::Component, {
                {172, {"strength", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::TargetedConstraint, {
            "TargetedConstraint", TypeId::TargetedConstraint,
            TypeId::Constraint, {
                {173, {"targetId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::IKConstraint, {
            "IKConstraint", TypeId::IKConstraint,
            TypeId::TargetedConstraint, {
                {174, {"invertDirection", PropertyType::Bool}},
                {175, {"parentBoneCount", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::DistanceConstraint, {
            "DistanceConstraint", TypeId::DistanceConstraint,
            TypeId::TargetedConstraint, {
                {177, {"distance", PropertyType::Float}},
                {178, {"modeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::TransformConstraint, {
            "TransformConstraint", TypeId::TransformConstraint,
            TypeId::TransformSpaceConstraint, {
            }
        }
    },
    {
        TypeId::KeyFrameBool, {
            "KeyFrameBool", TypeId::KeyFrameBool,
            TypeId::KeyFrame, {
                {181, {"value", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::TransformComponentConstraint, {
            "TransformComponentConstraint", TypeId::TransformComponentConstraint,
            TypeId::TransformSpaceConstraint, {
                {195, {"minMaxSpaceValue", PropertyType::VarUint}},
                {182, {"copyFactor", PropertyType::Float}},
                {183, {"minValue", PropertyType::Float}},
                {184, {"maxValue", PropertyType::Float}},
                {188, {"offset", PropertyType::Bool}},
                {189, {"doesCopy", PropertyType::Bool}},
                {190, {"min", PropertyType::Bool}},
                {191, {"max", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::TransformComponentConstraintY, {
            "TransformComponentConstraintY", TypeId::TransformComponentConstraintY,
            TypeId::TransformComponentConstraint, {
                {185, {"copyFactorY", PropertyType::Float}},
                {186, {"minValueY", PropertyType::Float}},
                {187, {"maxValueY", PropertyType::Float}},
                {192, {"doesCopyY", PropertyType::Bool}},
                {193, {"minY", PropertyType::Bool}},
                {194, {"maxY", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::TranslationConstraint, {
            "TranslationConstraint", TypeId::TranslationConstraint,
            TypeId::TransformComponentConstraintY, {
            }
        }
    },
    {
        TypeId::ScaleConstraint, {
            "ScaleConstraint", TypeId::ScaleConstraint,
            TypeId::TransformComponentConstraintY, {
            }
        }
    },
    {
        TypeId::RotationConstraint, {
            "RotationConstraint", TypeId::RotationConstraint,
            TypeId::TransformComponentConstraint, {
            }
        }
    },
    {
        TypeId::TransformSpaceConstraint, {
            "TransformSpaceConstraint", TypeId::TransformSpaceConstraint,
            TypeId::TargetedConstraint, {
                {179, {"sourceSpaceValue", PropertyType::VarUint}},
                {180, {"destSpaceValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::WorldTransformComponent, {
            "WorldTransformComponent", TypeId::WorldTransformComponent,
            TypeId::ContainerComponent, {
                {18, {"opacity", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::NestedArtboard, {
            "NestedArtboard", TypeId::NestedArtboard,
            TypeId::Drawable, {
                {197, {"artboardId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::NestedAnimation, {
            "NestedAnimation", TypeId::NestedAnimation,
            TypeId::ContainerComponent, {
                {198, {"animationId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::NestedStateMachine, {
            "NestedStateMachine", TypeId::NestedStateMachine,
            TypeId::NestedAnimation, {
            }
        }
    },
    {
        TypeId::NestedSimpleAnimation, {
            "NestedSimpleAnimation", TypeId::NestedSimpleAnimation,
            TypeId::NestedLinearAnimation, {
                {199, {"speed", PropertyType::Float}},
                {201, {"isPlaying", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::NestedLinearAnimation, {
            "NestedLinearAnimation", TypeId::NestedLinearAnimation,
            TypeId::NestedAnimation, {
                {200, {"mix", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::NestedRemapAnimation, {
            "NestedRemapAnimation", TypeId::NestedRemapAnimation,
            TypeId::NestedLinearAnimation, {
                {202, {"time", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Asset, {
            "Asset", TypeId::Asset,
            TypeId::NoType, {
                {203, {"name", PropertyType::String}},
            }
        }
    },
    {
        TypeId::Image, {
            "Image", TypeId::Image,
            TypeId::Drawable, {
                {206, {"assetId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::Folder, {
            "Folder", TypeId::Folder,
            TypeId::Asset, {
            }
        }
    },
    {
        TypeId::FileAsset, {
            "FileAsset", TypeId::FileAsset,
            TypeId::Asset, {
                {204, {"assetId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::DrawableAsset, {
            "DrawableAsset", TypeId::DrawableAsset,
            TypeId::FileAsset, {
                {207, {"height", PropertyType::Float}},
                {208, {"width", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::ImageAsset, {
            "ImageAsset", TypeId::ImageAsset,
            TypeId::DrawableAsset, {
            }
        }
    },
    {
        TypeId::FileAssetContents, {
            "FileAssetContents", TypeId::FileAssetContents,
            TypeId::NoType, {
                {212, {"bytes", PropertyType::Bytes}},
            }
        }
    },
    {
        TypeId::Vertex, {
            "Vertex", TypeId::Vertex,
            TypeId::ContainerComponent, {
                {24, {"x", PropertyType::Float}},
                {25, {"y", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::MeshVertex, {
            "MeshVertex", TypeId::MeshVertex,
            TypeId::Vertex, {
                {215, {"u", PropertyType::Float}},
                {216, {"v", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::Mesh, {
            "Mesh", TypeId::Mesh,
            TypeId::ContainerComponent, {
                {223, {"triangleIndexBytes", PropertyType::Bytes}},
            }
        }
    },
    {
        TypeId::Text, {
            "Text", TypeId::Text,
            TypeId::Node, {
                {218, {"value", PropertyType::String}},
            }
        }
    },
    {
        TypeId::ContourMeshVertex, {
            "ContourMeshVertex", TypeId::ContourMeshVertex,
            TypeId::MeshVertex, {
            }
        }
    },
    {
        TypeId::ForcedEdge, {
            "ForcedEdge", TypeId::ForcedEdge,
            TypeId::Component, {
                {219, {"fromId", PropertyType::VarUint}},
                {220, {"toId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::TextRun, {
            "TextRun", TypeId::TextRun,
            TypeId::Drawable, {
                {221, {"pointSize", PropertyType::Float}},
                {222, {"textLength", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::StateMachineListener, {
            "StateMachineListener", TypeId::StateMachineListener,
            TypeId::StateMachineComponent, {
                {224, {"targetId", PropertyType::VarUint}},
                {225, {"listenerTypeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::ListenerTriggerChange, {
            "ListenerTriggerChange", TypeId::ListenerTriggerChange,
            TypeId::ListenerInputChange, {
            }
        }
    },
    {
        TypeId::ListenerInputChange, {
            "ListenerInputChange", TypeId::ListenerInputChange,
            TypeId::ListenerAction, {
                {227, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::ListenerBoolChange, {
            "ListenerBoolChange", TypeId::ListenerBoolChange,
            TypeId::ListenerInputChange, {
                {228, {"value", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::ListenerNumberChange, {
            "ListenerNumberChange", TypeId::ListenerNumberChange,
            TypeId::ListenerInputChange, {
                {229, {"value", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::LayeredAsset, {
            "LayeredAsset", TypeId::LayeredAsset,
            TypeId::DrawableAsset, {
            }
        }
    },
    {
        TypeId::LayerImageAsset, {
            "LayerImageAsset", TypeId::LayerImageAsset,
            TypeId::ImageAsset, {
                {233, {"layer", PropertyType::VarUint}},
                {234, {"x", PropertyType::Float}},
                {235, {"y", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::NestedInput, {
            "NestedInput", TypeId::NestedInput,
            TypeId::Component, {
                {237, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        TypeId::NestedTrigger, {
            "NestedTrigger", TypeId::NestedTrigger,
            TypeId::NestedInput, {
            }
        }
    },
    {
        TypeId::NestedBool, {
            "NestedBool", TypeId::NestedBool,
            TypeId::NestedInput, {
                {238, {"nestedValue", PropertyType::Bool}},
            }
        }
    },
    {
        TypeId::NestedNumber, {
            "NestedNumber", TypeId::NestedNumber,
            TypeId::NestedInput, {
                {239, {"nestedValue", PropertyType::Float}},
            }
        }
    },
    {
        TypeId::ListenerAction, {
            "ListenerAction", TypeId::ListenerAction,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::ListenerAlignTarget, {
            "ListenerAlignTarget", TypeId::ListenerAlignTarget,
            TypeId::ListenerAction, {
                {240, {"targetId", PropertyType::VarUint}},
            }
        }
    },
};

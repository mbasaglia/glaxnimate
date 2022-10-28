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
                {"clip", 196, PropertyType::Bool},
                {"width", 7, PropertyType::Float},
                {"height", 8, PropertyType::Float},
                {"x", 9, PropertyType::Float},
                {"y", 10, PropertyType::Float},
                {"originX", 11, PropertyType::Float},
                {"originY", 12, PropertyType::Float},
                {"defaultStateMachineId", 236, PropertyType::VarUint},
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
                {"linkCornerRadius", 164, PropertyType::Bool},
                {"cornerRadiusTL", 31, PropertyType::Float},
                {"cornerRadiusTR", 161, PropertyType::Float},
                {"cornerRadiusBL", 162, PropertyType::Float},
                {"cornerRadiusBR", 163, PropertyType::Float},
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
            TypeId::Vertex, {
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
            TypeId::WorldTransformComponent, {
                {"rotation", 15, PropertyType::Float},
                {"scaleX", 16, PropertyType::Float},
                {"scaleY", 17, PropertyType::Float},
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
        TypeId::StateMachineNumber, {
            "StateMachineNumber", TypeId::StateMachineNumber,
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
                {"exitTime", 160, PropertyType::VarUint},
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
        TypeId::TransitionNumberCondition, {
            "TransitionNumberCondition", TypeId::TransitionNumberCondition,
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
                {"animationId", 165, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendAnimation1D, {
            "BlendAnimation1D", TypeId::BlendAnimation1D,
            TypeId::BlendAnimation, {
                {"value", 166, PropertyType::Float},
            }
        }
    },
    {
        TypeId::BlendState1D, {
            "BlendState1D", TypeId::BlendState1D,
            TypeId::BlendState, {
                {"inputId", 167, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendAnimationDirect, {
            "BlendAnimationDirect", TypeId::BlendAnimationDirect,
            TypeId::BlendAnimation, {
                {"inputId", 168, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendStateTransition, {
            "BlendStateTransition", TypeId::BlendStateTransition,
            TypeId::StateTransition, {
                {"exitBlendAnimationId", 171, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Constraint, {
            "Constraint", TypeId::Constraint,
            TypeId::Component, {
                {"strength", 172, PropertyType::Float},
            }
        }
    },
    {
        TypeId::TargetedConstraint, {
            "TargetedConstraint", TypeId::TargetedConstraint,
            TypeId::Constraint, {
                {"targetId", 173, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::IKConstraint, {
            "IKConstraint", TypeId::IKConstraint,
            TypeId::TargetedConstraint, {
                {"invertDirection", 174, PropertyType::Bool},
                {"parentBoneCount", 175, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DistanceConstraint, {
            "DistanceConstraint", TypeId::DistanceConstraint,
            TypeId::TargetedConstraint, {
                {"distance", 177, PropertyType::Float},
                {"modeValue", 178, PropertyType::VarUint},
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
                {"value", 181, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::TransformComponentConstraint, {
            "TransformComponentConstraint", TypeId::TransformComponentConstraint,
            TypeId::TransformSpaceConstraint, {
                {"minMaxSpaceValue", 195, PropertyType::VarUint},
                {"copyFactor", 182, PropertyType::Float},
                {"minValue", 183, PropertyType::Float},
                {"maxValue", 184, PropertyType::Float},
                {"offset", 188, PropertyType::Bool},
                {"doesCopy", 189, PropertyType::Bool},
                {"min", 190, PropertyType::Bool},
                {"max", 191, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::TransformComponentConstraintY, {
            "TransformComponentConstraintY", TypeId::TransformComponentConstraintY,
            TypeId::TransformComponentConstraint, {
                {"copyFactorY", 185, PropertyType::Float},
                {"minValueY", 186, PropertyType::Float},
                {"maxValueY", 187, PropertyType::Float},
                {"doesCopyY", 192, PropertyType::Bool},
                {"minY", 193, PropertyType::Bool},
                {"maxY", 194, PropertyType::Bool},
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
                {"sourceSpaceValue", 179, PropertyType::VarUint},
                {"destSpaceValue", 180, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::WorldTransformComponent, {
            "WorldTransformComponent", TypeId::WorldTransformComponent,
            TypeId::ContainerComponent, {
                {"opacity", 18, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedArtboard, {
            "NestedArtboard", TypeId::NestedArtboard,
            TypeId::Drawable, {
                {"artboardId", 197, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::NestedAnimation, {
            "NestedAnimation", TypeId::NestedAnimation,
            TypeId::ContainerComponent, {
                {"animationId", 198, PropertyType::VarUint},
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
                {"speed", 199, PropertyType::Float},
                {"isPlaying", 201, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::NestedLinearAnimation, {
            "NestedLinearAnimation", TypeId::NestedLinearAnimation,
            TypeId::NestedAnimation, {
                {"mix", 200, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedRemapAnimation, {
            "NestedRemapAnimation", TypeId::NestedRemapAnimation,
            TypeId::NestedLinearAnimation, {
                {"time", 202, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Asset, {
            "Asset", TypeId::Asset,
            TypeId::NoType, {
                {"name", 203, PropertyType::String},
            }
        }
    },
    {
        TypeId::Image, {
            "Image", TypeId::Image,
            TypeId::Drawable, {
                {"assetId", 206, PropertyType::VarUint},
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
                {"assetId", 204, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawableAsset, {
            "DrawableAsset", TypeId::DrawableAsset,
            TypeId::FileAsset, {
                {"height", 207, PropertyType::Float},
                {"width", 208, PropertyType::Float},
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
                {"bytes", 212, PropertyType::Bytes},
            }
        }
    },
    {
        TypeId::Vertex, {
            "Vertex", TypeId::Vertex,
            TypeId::ContainerComponent, {
                {"x", 24, PropertyType::Float},
                {"y", 25, PropertyType::Float},
            }
        }
    },
    {
        TypeId::MeshVertex, {
            "MeshVertex", TypeId::MeshVertex,
            TypeId::Vertex, {
                {"u", 215, PropertyType::Float},
                {"v", 216, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Mesh, {
            "Mesh", TypeId::Mesh,
            TypeId::ContainerComponent, {
                {"triangleIndexBytes", 223, PropertyType::Bytes},
            }
        }
    },
    {
        TypeId::Text, {
            "Text", TypeId::Text,
            TypeId::Node, {
                {"value", 218, PropertyType::String},
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
                {"fromId", 219, PropertyType::VarUint},
                {"toId", 220, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TextRun, {
            "TextRun", TypeId::TextRun,
            TypeId::Drawable, {
                {"pointSize", 221, PropertyType::Float},
                {"textLength", 222, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::StateMachineListener, {
            "StateMachineListener", TypeId::StateMachineListener,
            TypeId::StateMachineComponent, {
                {"targetId", 224, PropertyType::VarUint},
                {"listenerTypeValue", 225, PropertyType::VarUint},
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
                {"inputId", 227, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ListenerBoolChange, {
            "ListenerBoolChange", TypeId::ListenerBoolChange,
            TypeId::ListenerInputChange, {
                {"value", 228, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ListenerNumberChange, {
            "ListenerNumberChange", TypeId::ListenerNumberChange,
            TypeId::ListenerInputChange, {
                {"value", 229, PropertyType::Float},
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
                {"layer", 233, PropertyType::VarUint},
                {"x", 234, PropertyType::Float},
                {"y", 235, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedInput, {
            "NestedInput", TypeId::NestedInput,
            TypeId::Component, {
                {"inputId", 237, PropertyType::VarUint},
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
                {"nestedValue", 238, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::NestedNumber, {
            "NestedNumber", TypeId::NestedNumber,
            TypeId::NestedInput, {
                {"nestedValue", 239, PropertyType::Float},
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
                {"targetId", 240, PropertyType::VarUint},
            }
        }
    },
};

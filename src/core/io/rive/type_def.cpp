/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * NOTE: This file is generated automatically, do not edit manually
 * To generate this file run
 *       ./external/rive_typedef.py -t source >src/core/io/rive/type_def.cpp
 */


#include "type_def.hpp"
#include "app/utils/qstring_literal.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<TypeId, ObjectDefinition> glaxnimate::io::rive::defined_objects = {
    {
        TypeId::Artboard, {
            "Artboard"_qs, TypeId::Artboard,
            TypeId::WorldTransformComponent, {
                {"clip"_qs, 196, PropertyType::Bool},
                {"width"_qs, 7, PropertyType::Float},
                {"height"_qs, 8, PropertyType::Float},
                {"x"_qs, 9, PropertyType::Float},
                {"y"_qs, 10, PropertyType::Float},
                {"originX"_qs, 11, PropertyType::Float},
                {"originY"_qs, 12, PropertyType::Float},
                {"defaultStateMachineId"_qs, 236, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Node, {
            "Node"_qs, TypeId::Node,
            TypeId::TransformComponent, {
                {"x"_qs, 13, PropertyType::Float},
                {"y"_qs, 14, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Shape, {
            "Shape"_qs, TypeId::Shape,
            TypeId::Drawable, {
            }
        }
    },
    {
        TypeId::Ellipse, {
            "Ellipse"_qs, TypeId::Ellipse,
            TypeId::ParametricPath, {
            }
        }
    },
    {
        TypeId::StraightVertex, {
            "StraightVertex"_qs, TypeId::StraightVertex,
            TypeId::PathVertex, {
                {"radius"_qs, 26, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicDetachedVertex, {
            "CubicDetachedVertex"_qs, TypeId::CubicDetachedVertex,
            TypeId::CubicVertex, {
                {"inRotation"_qs, 84, PropertyType::Float},
                {"inDistance"_qs, 85, PropertyType::Float},
                {"outRotation"_qs, 86, PropertyType::Float},
                {"outDistance"_qs, 87, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Rectangle, {
            "Rectangle"_qs, TypeId::Rectangle,
            TypeId::ParametricPath, {
                {"linkCornerRadius"_qs, 164, PropertyType::Bool},
                {"cornerRadiusTL"_qs, 31, PropertyType::Float},
                {"cornerRadiusTR"_qs, 161, PropertyType::Float},
                {"cornerRadiusBL"_qs, 162, PropertyType::Float},
                {"cornerRadiusBR"_qs, 163, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Triangle, {
            "Triangle"_qs, TypeId::Triangle,
            TypeId::ParametricPath, {
            }
        }
    },
    {
        TypeId::Component, {
            "Component"_qs, TypeId::Component,
            TypeId::NoType, {
                {"name"_qs, 4, PropertyType::String},
                {"parentId"_qs, 5, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ContainerComponent, {
            "ContainerComponent"_qs, TypeId::ContainerComponent,
            TypeId::Component, {
            }
        }
    },
    {
        TypeId::Path, {
            "Path"_qs, TypeId::Path,
            TypeId::Node, {
                {"pathFlags"_qs, 128, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Drawable, {
            "Drawable"_qs, TypeId::Drawable,
            TypeId::Node, {
                {"blendModeValue"_qs, 23, PropertyType::VarUint},
                {"drawableFlags"_qs, 129, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::PathVertex, {
            "PathVertex"_qs, TypeId::PathVertex,
            TypeId::Vertex, {
            }
        }
    },
    {
        TypeId::ParametricPath, {
            "ParametricPath"_qs, TypeId::ParametricPath,
            TypeId::Path, {
                {"width"_qs, 20, PropertyType::Float},
                {"height"_qs, 21, PropertyType::Float},
                {"originX"_qs, 123, PropertyType::Float},
                {"originY"_qs, 124, PropertyType::Float},
            }
        }
    },
    {
        TypeId::PointsPath, {
            "PointsPath"_qs, TypeId::PointsPath,
            TypeId::Path, {
                {"isClosed"_qs, 32, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::RadialGradient, {
            "RadialGradient"_qs, TypeId::RadialGradient,
            TypeId::LinearGradient, {
            }
        }
    },
    {
        TypeId::SolidColor, {
            "SolidColor"_qs, TypeId::SolidColor,
            TypeId::Component, {
                {"colorValue"_qs, 37, PropertyType::Color},
            }
        }
    },
    {
        TypeId::GradientStop, {
            "GradientStop"_qs, TypeId::GradientStop,
            TypeId::Component, {
                {"colorValue"_qs, 38, PropertyType::Color},
                {"position"_qs, 39, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Fill, {
            "Fill"_qs, TypeId::Fill,
            TypeId::ShapePaint, {
                {"fillRule"_qs, 40, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ShapePaint, {
            "ShapePaint"_qs, TypeId::ShapePaint,
            TypeId::ContainerComponent, {
                {"isVisible"_qs, 41, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::LinearGradient, {
            "LinearGradient"_qs, TypeId::LinearGradient,
            TypeId::ContainerComponent, {
                {"startX"_qs, 42, PropertyType::Float},
                {"startY"_qs, 33, PropertyType::Float},
                {"endX"_qs, 34, PropertyType::Float},
                {"endY"_qs, 35, PropertyType::Float},
                {"opacity"_qs, 46, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Backboard, {
            "Backboard"_qs, TypeId::Backboard,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::Stroke, {
            "Stroke"_qs, TypeId::Stroke,
            TypeId::ShapePaint, {
                {"thickness"_qs, 47, PropertyType::Float},
                {"cap"_qs, 48, PropertyType::VarUint},
                {"join"_qs, 49, PropertyType::VarUint},
                {"transformAffectsStroke"_qs, 50, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::KeyedObject, {
            "KeyedObject"_qs, TypeId::KeyedObject,
            TypeId::NoType, {
                {"objectId"_qs, 51, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyedProperty, {
            "KeyedProperty"_qs, TypeId::KeyedProperty,
            TypeId::NoType, {
                {"propertyKey"_qs, 53, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Animation, {
            "Animation"_qs, TypeId::Animation,
            TypeId::NoType, {
                {"name"_qs, 55, PropertyType::String},
            }
        }
    },
    {
        TypeId::CubicInterpolator, {
            "CubicInterpolator"_qs, TypeId::CubicInterpolator,
            TypeId::NoType, {
                {"x1"_qs, 63, PropertyType::Float},
                {"y1"_qs, 64, PropertyType::Float},
                {"x2"_qs, 65, PropertyType::Float},
                {"y2"_qs, 66, PropertyType::Float},
            }
        }
    },
    {
        TypeId::KeyFrame, {
            "KeyFrame"_qs, TypeId::KeyFrame,
            TypeId::NoType, {
                {"frame"_qs, 67, PropertyType::VarUint},
                {"interpolationType"_qs, 68, PropertyType::VarUint},
                {"interpolatorId"_qs, 69, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyFrameDouble, {
            "KeyFrameDouble"_qs, TypeId::KeyFrameDouble,
            TypeId::KeyFrame, {
                {"value"_qs, 70, PropertyType::Float},
            }
        }
    },
    {
        TypeId::LinearAnimation, {
            "LinearAnimation"_qs, TypeId::LinearAnimation,
            TypeId::Animation, {
                {"fps"_qs, 56, PropertyType::VarUint},
                {"duration"_qs, 57, PropertyType::VarUint},
                {"speed"_qs, 58, PropertyType::Float},
                {"loopValue"_qs, 59, PropertyType::VarUint},
                {"workStart"_qs, 60, PropertyType::VarUint},
                {"workEnd"_qs, 61, PropertyType::VarUint},
                {"enableWorkArea"_qs, 62, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::CubicAsymmetricVertex, {
            "CubicAsymmetricVertex"_qs, TypeId::CubicAsymmetricVertex,
            TypeId::CubicVertex, {
                {"rotation"_qs, 79, PropertyType::Float},
                {"inDistance"_qs, 80, PropertyType::Float},
                {"outDistance"_qs, 81, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicMirroredVertex, {
            "CubicMirroredVertex"_qs, TypeId::CubicMirroredVertex,
            TypeId::CubicVertex, {
                {"rotation"_qs, 82, PropertyType::Float},
                {"distance"_qs, 83, PropertyType::Float},
            }
        }
    },
    {
        TypeId::CubicVertex, {
            "CubicVertex"_qs, TypeId::CubicVertex,
            TypeId::PathVertex, {
            }
        }
    },
    {
        TypeId::KeyFrameColor, {
            "KeyFrameColor"_qs, TypeId::KeyFrameColor,
            TypeId::KeyFrame, {
                {"value"_qs, 88, PropertyType::Color},
            }
        }
    },
    {
        TypeId::TransformComponent, {
            "TransformComponent"_qs, TypeId::TransformComponent,
            TypeId::WorldTransformComponent, {
                {"rotation"_qs, 15, PropertyType::Float},
                {"scaleX"_qs, 16, PropertyType::Float},
                {"scaleY"_qs, 17, PropertyType::Float},
            }
        }
    },
    {
        TypeId::SkeletalComponent, {
            "SkeletalComponent"_qs, TypeId::SkeletalComponent,
            TypeId::TransformComponent, {
            }
        }
    },
    {
        TypeId::Bone, {
            "Bone"_qs, TypeId::Bone,
            TypeId::SkeletalComponent, {
                {"length"_qs, 89, PropertyType::Float},
            }
        }
    },
    {
        TypeId::RootBone, {
            "RootBone"_qs, TypeId::RootBone,
            TypeId::Bone, {
                {"x"_qs, 90, PropertyType::Float},
                {"y"_qs, 91, PropertyType::Float},
            }
        }
    },
    {
        TypeId::ClippingShape, {
            "ClippingShape"_qs, TypeId::ClippingShape,
            TypeId::Component, {
                {"sourceId"_qs, 92, PropertyType::VarUint},
                {"fillRule"_qs, 93, PropertyType::VarUint},
                {"isVisible"_qs, 94, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::Skin, {
            "Skin"_qs, TypeId::Skin,
            TypeId::ContainerComponent, {
                {"xx"_qs, 104, PropertyType::Float},
                {"yx"_qs, 105, PropertyType::Float},
                {"xy"_qs, 106, PropertyType::Float},
                {"yy"_qs, 107, PropertyType::Float},
                {"tx"_qs, 108, PropertyType::Float},
                {"ty"_qs, 109, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Tendon, {
            "Tendon"_qs, TypeId::Tendon,
            TypeId::Component, {
                {"boneId"_qs, 95, PropertyType::VarUint},
                {"xx"_qs, 96, PropertyType::Float},
                {"yx"_qs, 97, PropertyType::Float},
                {"xy"_qs, 98, PropertyType::Float},
                {"yy"_qs, 99, PropertyType::Float},
                {"tx"_qs, 100, PropertyType::Float},
                {"ty"_qs, 101, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Weight, {
            "Weight"_qs, TypeId::Weight,
            TypeId::Component, {
                {"values"_qs, 102, PropertyType::VarUint},
                {"indices"_qs, 103, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::CubicWeight, {
            "CubicWeight"_qs, TypeId::CubicWeight,
            TypeId::Weight, {
                {"inValues"_qs, 110, PropertyType::VarUint},
                {"inIndices"_qs, 111, PropertyType::VarUint},
                {"outValues"_qs, 112, PropertyType::VarUint},
                {"outIndices"_qs, 113, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TrimPath, {
            "TrimPath"_qs, TypeId::TrimPath,
            TypeId::Component, {
                {"start"_qs, 114, PropertyType::Float},
                {"end"_qs, 115, PropertyType::Float},
                {"offset"_qs, 116, PropertyType::Float},
                {"modeValue"_qs, 117, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawTarget, {
            "DrawTarget"_qs, TypeId::DrawTarget,
            TypeId::Component, {
                {"drawableId"_qs, 119, PropertyType::VarUint},
                {"placementValue"_qs, 120, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawRules, {
            "DrawRules"_qs, TypeId::DrawRules,
            TypeId::ContainerComponent, {
                {"drawTargetId"_qs, 121, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::KeyFrameId, {
            "KeyFrameId"_qs, TypeId::KeyFrameId,
            TypeId::KeyFrame, {
                {"value"_qs, 122, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Polygon, {
            "Polygon"_qs, TypeId::Polygon,
            TypeId::ParametricPath, {
                {"points"_qs, 125, PropertyType::VarUint},
                {"cornerRadius"_qs, 126, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Star, {
            "Star"_qs, TypeId::Star,
            TypeId::Polygon, {
                {"innerRadius"_qs, 127, PropertyType::Float},
            }
        }
    },
    {
        TypeId::StateMachine, {
            "StateMachine"_qs, TypeId::StateMachine,
            TypeId::Animation, {
            }
        }
    },
    {
        TypeId::StateMachineComponent, {
            "StateMachineComponent"_qs, TypeId::StateMachineComponent,
            TypeId::NoType, {
                {"name"_qs, 138, PropertyType::String},
            }
        }
    },
    {
        TypeId::StateMachineInput, {
            "StateMachineInput"_qs, TypeId::StateMachineInput,
            TypeId::StateMachineComponent, {
            }
        }
    },
    {
        TypeId::StateMachineNumber, {
            "StateMachineNumber"_qs, TypeId::StateMachineNumber,
            TypeId::StateMachineInput, {
                {"value"_qs, 140, PropertyType::Float},
            }
        }
    },
    {
        TypeId::StateMachineLayer, {
            "StateMachineLayer"_qs, TypeId::StateMachineLayer,
            TypeId::StateMachineComponent, {
            }
        }
    },
    {
        TypeId::StateMachineTrigger, {
            "StateMachineTrigger"_qs, TypeId::StateMachineTrigger,
            TypeId::StateMachineInput, {
            }
        }
    },
    {
        TypeId::StateMachineBool, {
            "StateMachineBool"_qs, TypeId::StateMachineBool,
            TypeId::StateMachineInput, {
                {"value"_qs, 141, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::LayerState, {
            "LayerState"_qs, TypeId::LayerState,
            TypeId::StateMachineLayerComponent, {
            }
        }
    },
    {
        TypeId::AnimationState, {
            "AnimationState"_qs, TypeId::AnimationState,
            TypeId::LayerState, {
                {"animationId"_qs, 149, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::AnyState, {
            "AnyState"_qs, TypeId::AnyState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::EntryState, {
            "EntryState"_qs, TypeId::EntryState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::ExitState, {
            "ExitState"_qs, TypeId::ExitState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::StateTransition, {
            "StateTransition"_qs, TypeId::StateTransition,
            TypeId::StateMachineLayerComponent, {
                {"stateToId"_qs, 151, PropertyType::VarUint},
                {"flags"_qs, 152, PropertyType::VarUint},
                {"duration"_qs, 158, PropertyType::VarUint},
                {"exitTime"_qs, 160, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::StateMachineLayerComponent, {
            "StateMachineLayerComponent"_qs, TypeId::StateMachineLayerComponent,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::TransitionCondition, {
            "TransitionCondition"_qs, TypeId::TransitionCondition,
            TypeId::NoType, {
                {"inputId"_qs, 155, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TransitionTriggerCondition, {
            "TransitionTriggerCondition"_qs, TypeId::TransitionTriggerCondition,
            TypeId::TransitionCondition, {
            }
        }
    },
    {
        TypeId::TransitionValueCondition, {
            "TransitionValueCondition"_qs, TypeId::TransitionValueCondition,
            TypeId::TransitionCondition, {
                {"opValue"_qs, 156, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TransitionNumberCondition, {
            "TransitionNumberCondition"_qs, TypeId::TransitionNumberCondition,
            TypeId::TransitionValueCondition, {
                {"value"_qs, 157, PropertyType::Float},
            }
        }
    },
    {
        TypeId::TransitionBoolCondition, {
            "TransitionBoolCondition"_qs, TypeId::TransitionBoolCondition,
            TypeId::TransitionValueCondition, {
            }
        }
    },
    {
        TypeId::BlendState, {
            "BlendState"_qs, TypeId::BlendState,
            TypeId::LayerState, {
            }
        }
    },
    {
        TypeId::BlendStateDirect, {
            "BlendStateDirect"_qs, TypeId::BlendStateDirect,
            TypeId::BlendState, {
            }
        }
    },
    {
        TypeId::BlendAnimation, {
            "BlendAnimation"_qs, TypeId::BlendAnimation,
            TypeId::NoType, {
                {"animationId"_qs, 165, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendAnimation1D, {
            "BlendAnimation1D"_qs, TypeId::BlendAnimation1D,
            TypeId::BlendAnimation, {
                {"value"_qs, 166, PropertyType::Float},
            }
        }
    },
    {
        TypeId::BlendState1D, {
            "BlendState1D"_qs, TypeId::BlendState1D,
            TypeId::BlendState, {
                {"inputId"_qs, 167, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendAnimationDirect, {
            "BlendAnimationDirect"_qs, TypeId::BlendAnimationDirect,
            TypeId::BlendAnimation, {
                {"inputId"_qs, 168, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::BlendStateTransition, {
            "BlendStateTransition"_qs, TypeId::BlendStateTransition,
            TypeId::StateTransition, {
                {"exitBlendAnimationId"_qs, 171, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Constraint, {
            "Constraint"_qs, TypeId::Constraint,
            TypeId::Component, {
                {"strength"_qs, 172, PropertyType::Float},
            }
        }
    },
    {
        TypeId::TargetedConstraint, {
            "TargetedConstraint"_qs, TypeId::TargetedConstraint,
            TypeId::Constraint, {
                {"targetId"_qs, 173, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::IKConstraint, {
            "IKConstraint"_qs, TypeId::IKConstraint,
            TypeId::TargetedConstraint, {
                {"invertDirection"_qs, 174, PropertyType::Bool},
                {"parentBoneCount"_qs, 175, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DistanceConstraint, {
            "DistanceConstraint"_qs, TypeId::DistanceConstraint,
            TypeId::TargetedConstraint, {
                {"distance"_qs, 177, PropertyType::Float},
                {"modeValue"_qs, 178, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TransformConstraint, {
            "TransformConstraint"_qs, TypeId::TransformConstraint,
            TypeId::TransformSpaceConstraint, {
            }
        }
    },
    {
        TypeId::KeyFrameBool, {
            "KeyFrameBool"_qs, TypeId::KeyFrameBool,
            TypeId::KeyFrame, {
                {"value"_qs, 181, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::TransformComponentConstraint, {
            "TransformComponentConstraint"_qs, TypeId::TransformComponentConstraint,
            TypeId::TransformSpaceConstraint, {
                {"minMaxSpaceValue"_qs, 195, PropertyType::VarUint},
                {"copyFactor"_qs, 182, PropertyType::Float},
                {"minValue"_qs, 183, PropertyType::Float},
                {"maxValue"_qs, 184, PropertyType::Float},
                {"offset"_qs, 188, PropertyType::Bool},
                {"doesCopy"_qs, 189, PropertyType::Bool},
                {"min"_qs, 190, PropertyType::Bool},
                {"max"_qs, 191, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::TransformComponentConstraintY, {
            "TransformComponentConstraintY"_qs, TypeId::TransformComponentConstraintY,
            TypeId::TransformComponentConstraint, {
                {"copyFactorY"_qs, 185, PropertyType::Float},
                {"minValueY"_qs, 186, PropertyType::Float},
                {"maxValueY"_qs, 187, PropertyType::Float},
                {"doesCopyY"_qs, 192, PropertyType::Bool},
                {"minY"_qs, 193, PropertyType::Bool},
                {"maxY"_qs, 194, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::TranslationConstraint, {
            "TranslationConstraint"_qs, TypeId::TranslationConstraint,
            TypeId::TransformComponentConstraintY, {
            }
        }
    },
    {
        TypeId::ScaleConstraint, {
            "ScaleConstraint"_qs, TypeId::ScaleConstraint,
            TypeId::TransformComponentConstraintY, {
            }
        }
    },
    {
        TypeId::RotationConstraint, {
            "RotationConstraint"_qs, TypeId::RotationConstraint,
            TypeId::TransformComponentConstraint, {
            }
        }
    },
    {
        TypeId::TransformSpaceConstraint, {
            "TransformSpaceConstraint"_qs, TypeId::TransformSpaceConstraint,
            TypeId::TargetedConstraint, {
                {"sourceSpaceValue"_qs, 179, PropertyType::VarUint},
                {"destSpaceValue"_qs, 180, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::WorldTransformComponent, {
            "WorldTransformComponent"_qs, TypeId::WorldTransformComponent,
            TypeId::ContainerComponent, {
                {"opacity"_qs, 18, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedArtboard, {
            "NestedArtboard"_qs, TypeId::NestedArtboard,
            TypeId::Drawable, {
                {"artboardId"_qs, 197, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::NestedAnimation, {
            "NestedAnimation"_qs, TypeId::NestedAnimation,
            TypeId::ContainerComponent, {
                {"animationId"_qs, 198, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::NestedStateMachine, {
            "NestedStateMachine"_qs, TypeId::NestedStateMachine,
            TypeId::NestedAnimation, {
            }
        }
    },
    {
        TypeId::NestedSimpleAnimation, {
            "NestedSimpleAnimation"_qs, TypeId::NestedSimpleAnimation,
            TypeId::NestedLinearAnimation, {
                {"speed"_qs, 199, PropertyType::Float},
                {"isPlaying"_qs, 201, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::NestedLinearAnimation, {
            "NestedLinearAnimation"_qs, TypeId::NestedLinearAnimation,
            TypeId::NestedAnimation, {
                {"mix"_qs, 200, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedRemapAnimation, {
            "NestedRemapAnimation"_qs, TypeId::NestedRemapAnimation,
            TypeId::NestedLinearAnimation, {
                {"time"_qs, 202, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Asset, {
            "Asset"_qs, TypeId::Asset,
            TypeId::NoType, {
                {"name"_qs, 203, PropertyType::String},
            }
        }
    },
    {
        TypeId::Image, {
            "Image"_qs, TypeId::Image,
            TypeId::Drawable, {
                {"assetId"_qs, 206, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::Folder, {
            "Folder"_qs, TypeId::Folder,
            TypeId::Asset, {
            }
        }
    },
    {
        TypeId::FileAsset, {
            "FileAsset"_qs, TypeId::FileAsset,
            TypeId::Asset, {
                {"assetId"_qs, 204, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::DrawableAsset, {
            "DrawableAsset"_qs, TypeId::DrawableAsset,
            TypeId::FileAsset, {
                {"height"_qs, 207, PropertyType::Float},
                {"width"_qs, 208, PropertyType::Float},
            }
        }
    },
    {
        TypeId::ImageAsset, {
            "ImageAsset"_qs, TypeId::ImageAsset,
            TypeId::DrawableAsset, {
            }
        }
    },
    {
        TypeId::FileAssetContents, {
            "FileAssetContents"_qs, TypeId::FileAssetContents,
            TypeId::NoType, {
                {"bytes"_qs, 212, PropertyType::Bytes},
            }
        }
    },
    {
        TypeId::Vertex, {
            "Vertex"_qs, TypeId::Vertex,
            TypeId::ContainerComponent, {
                {"x"_qs, 24, PropertyType::Float},
                {"y"_qs, 25, PropertyType::Float},
            }
        }
    },
    {
        TypeId::MeshVertex, {
            "MeshVertex"_qs, TypeId::MeshVertex,
            TypeId::Vertex, {
                {"u"_qs, 215, PropertyType::Float},
                {"v"_qs, 216, PropertyType::Float},
            }
        }
    },
    {
        TypeId::Mesh, {
            "Mesh"_qs, TypeId::Mesh,
            TypeId::ContainerComponent, {
                {"triangleIndexBytes"_qs, 223, PropertyType::Bytes},
            }
        }
    },
    {
        TypeId::Text, {
            "Text"_qs, TypeId::Text,
            TypeId::Node, {
                {"value"_qs, 218, PropertyType::String},
            }
        }
    },
    {
        TypeId::ContourMeshVertex, {
            "ContourMeshVertex"_qs, TypeId::ContourMeshVertex,
            TypeId::MeshVertex, {
            }
        }
    },
    {
        TypeId::ForcedEdge, {
            "ForcedEdge"_qs, TypeId::ForcedEdge,
            TypeId::Component, {
                {"fromId"_qs, 219, PropertyType::VarUint},
                {"toId"_qs, 220, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::TextRun, {
            "TextRun"_qs, TypeId::TextRun,
            TypeId::Drawable, {
                {"pointSize"_qs, 221, PropertyType::Float},
                {"textLength"_qs, 222, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::StateMachineListener, {
            "StateMachineListener"_qs, TypeId::StateMachineListener,
            TypeId::StateMachineComponent, {
                {"targetId"_qs, 224, PropertyType::VarUint},
                {"listenerTypeValue"_qs, 225, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ListenerTriggerChange, {
            "ListenerTriggerChange"_qs, TypeId::ListenerTriggerChange,
            TypeId::ListenerInputChange, {
            }
        }
    },
    {
        TypeId::ListenerInputChange, {
            "ListenerInputChange"_qs, TypeId::ListenerInputChange,
            TypeId::ListenerAction, {
                {"inputId"_qs, 227, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ListenerBoolChange, {
            "ListenerBoolChange"_qs, TypeId::ListenerBoolChange,
            TypeId::ListenerInputChange, {
                {"value"_qs, 228, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::ListenerNumberChange, {
            "ListenerNumberChange"_qs, TypeId::ListenerNumberChange,
            TypeId::ListenerInputChange, {
                {"value"_qs, 229, PropertyType::Float},
            }
        }
    },
    {
        TypeId::LayeredAsset, {
            "LayeredAsset"_qs, TypeId::LayeredAsset,
            TypeId::DrawableAsset, {
            }
        }
    },
    {
        TypeId::LayerImageAsset, {
            "LayerImageAsset"_qs, TypeId::LayerImageAsset,
            TypeId::ImageAsset, {
                {"layer"_qs, 233, PropertyType::VarUint},
                {"x"_qs, 234, PropertyType::Float},
                {"y"_qs, 235, PropertyType::Float},
            }
        }
    },
    {
        TypeId::NestedInput, {
            "NestedInput"_qs, TypeId::NestedInput,
            TypeId::Component, {
                {"inputId"_qs, 237, PropertyType::VarUint},
            }
        }
    },
    {
        TypeId::NestedTrigger, {
            "NestedTrigger"_qs, TypeId::NestedTrigger,
            TypeId::NestedInput, {
            }
        }
    },
    {
        TypeId::NestedBool, {
            "NestedBool"_qs, TypeId::NestedBool,
            TypeId::NestedInput, {
                {"nestedValue"_qs, 238, PropertyType::Bool},
            }
        }
    },
    {
        TypeId::NestedNumber, {
            "NestedNumber"_qs, TypeId::NestedNumber,
            TypeId::NestedInput, {
                {"nestedValue"_qs, 239, PropertyType::Float},
            }
        }
    },
    {
        TypeId::ListenerAction, {
            "ListenerAction"_qs, TypeId::ListenerAction,
            TypeId::NoType, {
            }
        }
    },
    {
        TypeId::ListenerAlignTarget, {
            "ListenerAlignTarget"_qs, TypeId::ListenerAlignTarget,
            TypeId::ListenerAction, {
                {"targetId"_qs, 240, PropertyType::VarUint},
            }
        }
    },
};

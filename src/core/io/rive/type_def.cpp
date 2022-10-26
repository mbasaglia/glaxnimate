/**
 * NOTE: This file is generated automatically, do not edit manually
 */

#include "type_def.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<Identifier, ObjectDefinition> glaxnimate::io::rive::defined_objects = {
    {
        93, {
            "NestedAnimation",
            11, {
                {198, {"animationId", PropertyType::VarUint}},
            }
        }
    },
    {
        10, {
            "Component",
            0, {
                {4, {"name", PropertyType::String}},
                {5, {"parentId", PropertyType::VarUint}},
            }
        }
    },
    {
        102, {
            "Folder",
            99, {
            }
        }
    },
    {
        104, {
            "DrawableAsset",
            103, {
                {207, {"height", PropertyType::Float}},
                {208, {"width", PropertyType::Float}},
            }
        }
    },
    {
        103, {
            "FileAsset",
            99, {
                {204, {"assetId", PropertyType::VarUint}},
            }
        }
    },
    {
        105, {
            "ImageAsset",
            104, {
            }
        }
    },
    {
        120, {
            "LayerImageAsset",
            105, {
                {233, {"layer", PropertyType::VarUint}},
                {234, {"x", PropertyType::Float}},
                {235, {"y", PropertyType::Float}},
            }
        }
    },
    {
        119, {
            "LayeredAsset",
            104, {
            }
        }
    },
    {
        99, {
            "Asset",
            0, {
                {203, {"name", PropertyType::String}},
            }
        }
    },
    {
        106, {
            "FileAssetContents",
            0, {
                {212, {"bytes", PropertyType::Bytes}},
            }
        }
    },
    {
        23, {
            "Backboard",
            0, {
            }
        }
    },
    {
        46, {
            "CubicWeight",
            45, {
                {110, {"inValues", PropertyType::VarUint}},
                {111, {"inIndices", PropertyType::VarUint}},
                {112, {"outValues", PropertyType::VarUint}},
                {113, {"outIndices", PropertyType::VarUint}},
            }
        }
    },
    {
        45, {
            "Weight",
            10, {
                {102, {"values", PropertyType::VarUint}},
                {103, {"indices", PropertyType::VarUint}},
            }
        }
    },
    {
        43, {
            "Skin",
            11, {
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
        44, {
            "Tendon",
            10, {
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
        41, {
            "RootBone",
            40, {
                {90, {"x", PropertyType::Float}},
                {91, {"y", PropertyType::Float}},
            }
        }
    },
    {
        39, {
            "SkeletalComponent",
            38, {
            }
        }
    },
    {
        40, {
            "Bone",
            39, {
                {89, {"length", PropertyType::Float}},
            }
        }
    },
    {
        49, {
            "DrawRules",
            11, {
                {121, {"drawTargetId", PropertyType::VarUint}},
            }
        }
    },
    {
        13, {
            "Drawable",
            2, {
                {23, {"blendModeValue", PropertyType::VarUint}},
                {129, {"drawableFlags", PropertyType::VarUint}},
            }
        }
    },
    {
        48, {
            "DrawTarget",
            10, {
                {119, {"drawableId", PropertyType::VarUint}},
                {120, {"placementValue", PropertyType::VarUint}},
            }
        }
    },
    {
        87, {
            "TranslationConstraint",
            86, {
            }
        }
    },
    {
        88, {
            "ScaleConstraint",
            86, {
            }
        }
    },
    {
        90, {
            "TransformSpaceConstraint",
            80, {
                {179, {"sourceSpaceValue", PropertyType::VarUint}},
                {180, {"destSpaceValue", PropertyType::VarUint}},
            }
        }
    },
    {
        80, {
            "TargetedConstraint",
            79, {
                {173, {"targetId", PropertyType::VarUint}},
            }
        }
    },
    {
        85, {
            "TransformComponentConstraint",
            90, {
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
        79, {
            "Constraint",
            10, {
                {172, {"strength", PropertyType::Float}},
            }
        }
    },
    {
        82, {
            "DistanceConstraint",
            80, {
                {177, {"distance", PropertyType::Float}},
                {178, {"modeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        89, {
            "RotationConstraint",
            85, {
            }
        }
    },
    {
        86, {
            "TransformComponentConstraintY",
            85, {
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
        81, {
            "IKConstraint",
            80, {
                {174, {"invertDirection", PropertyType::Bool}},
                {175, {"parentBoneCount", PropertyType::VarUint}},
            }
        }
    },
    {
        83, {
            "TransformConstraint",
            90, {
            }
        }
    },
    {
        34, {
            "CubicAsymmetricVertex",
            36, {
                {79, {"rotation", PropertyType::Float}},
                {80, {"inDistance", PropertyType::Float}},
                {81, {"outDistance", PropertyType::Float}},
            }
        }
    },
    {
        111, {
            "ContourMeshVertex",
            108, {
            }
        }
    },
    {
        6, {
            "CubicDetachedVertex",
            36, {
                {84, {"inRotation", PropertyType::Float}},
                {85, {"inDistance", PropertyType::Float}},
                {86, {"outRotation", PropertyType::Float}},
                {87, {"outDistance", PropertyType::Float}},
            }
        }
    },
    {
        20, {
            "Fill",
            21, {
                {40, {"fillRule", PropertyType::VarUint}},
            }
        }
    },
    {
        21, {
            "ShapePaint",
            11, {
                {41, {"isVisible", PropertyType::Bool}},
            }
        }
    },
    {
        24, {
            "Stroke",
            21, {
                {47, {"thickness", PropertyType::Float}},
                {48, {"cap", PropertyType::VarUint}},
                {49, {"join", PropertyType::VarUint}},
                {50, {"transformAffectsStroke", PropertyType::Bool}},
            }
        }
    },
    {
        22, {
            "LinearGradient",
            11, {
                {42, {"startX", PropertyType::Float}},
                {33, {"startY", PropertyType::Float}},
                {34, {"endX", PropertyType::Float}},
                {35, {"endY", PropertyType::Float}},
                {46, {"opacity", PropertyType::Float}},
            }
        }
    },
    {
        47, {
            "TrimPath",
            10, {
                {114, {"start", PropertyType::Float}},
                {115, {"end", PropertyType::Float}},
                {116, {"offset", PropertyType::Float}},
                {117, {"modeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        17, {
            "RadialGradient",
            22, {
            }
        }
    },
    {
        19, {
            "GradientStop",
            10, {
                {38, {"colorValue", PropertyType::Color}},
                {39, {"position", PropertyType::Float}},
            }
        }
    },
    {
        18, {
            "SolidColor",
            10, {
                {37, {"colorValue", PropertyType::Color}},
            }
        }
    },
    {
        110, {
            "Text",
            2, {
                {218, {"value", PropertyType::String}},
            }
        }
    },
    {
        16, {
            "PointsPath",
            12, {
                {32, {"isClosed", PropertyType::Bool}},
            }
        }
    },
    {
        3, {
            "Shape",
            13, {
            }
        }
    },
    {
        35, {
            "CubicMirroredVertex",
            36, {
                {82, {"rotation", PropertyType::Float}},
                {83, {"distance", PropertyType::Float}},
            }
        }
    },
    {
        100, {
            "Image",
            13, {
                {206, {"assetId", PropertyType::VarUint}},
            }
        }
    },
    {
        52, {
            "Star",
            51, {
                {127, {"innerRadius", PropertyType::Float}},
            }
        }
    },
    {
        107, {
            "Vertex",
            11, {
                {24, {"x", PropertyType::Float}},
                {25, {"y", PropertyType::Float}},
            }
        }
    },
    {
        14, {
            "PathVertex",
            107, {
            }
        }
    },
    {
        4, {
            "Ellipse",
            15, {
            }
        }
    },
    {
        5, {
            "StraightVertex",
            14, {
                {26, {"radius", PropertyType::Float}},
            }
        }
    },
    {
        36, {
            "CubicVertex",
            14, {
            }
        }
    },
    {
        108, {
            "MeshVertex",
            107, {
                {215, {"u", PropertyType::Float}},
                {216, {"v", PropertyType::Float}},
            }
        }
    },
    {
        15, {
            "ParametricPath",
            12, {
                {20, {"width", PropertyType::Float}},
                {21, {"height", PropertyType::Float}},
                {123, {"originX", PropertyType::Float}},
                {124, {"originY", PropertyType::Float}},
            }
        }
    },
    {
        112, {
            "ForcedEdge",
            10, {
                {219, {"fromId", PropertyType::VarUint}},
                {220, {"toId", PropertyType::VarUint}},
            }
        }
    },
    {
        8, {
            "Triangle",
            15, {
            }
        }
    },
    {
        42, {
            "ClippingShape",
            10, {
                {92, {"sourceId", PropertyType::VarUint}},
                {93, {"fillRule", PropertyType::VarUint}},
                {94, {"isVisible", PropertyType::Bool}},
            }
        }
    },
    {
        12, {
            "Path",
            2, {
                {128, {"pathFlags", PropertyType::VarUint}},
            }
        }
    },
    {
        109, {
            "Mesh",
            11, {
                {223, {"triangleIndexBytes", PropertyType::Bytes}},
            }
        }
    },
    {
        7, {
            "Rectangle",
            15, {
                {164, {"linkCornerRadius", PropertyType::Bool}},
                {31, {"cornerRadiusTL", PropertyType::Float}},
                {161, {"cornerRadiusTR", PropertyType::Float}},
                {162, {"cornerRadiusBL", PropertyType::Float}},
                {163, {"cornerRadiusBR", PropertyType::Float}},
            }
        }
    },
    {
        113, {
            "TextRun",
            13, {
                {221, {"pointSize", PropertyType::Float}},
                {222, {"textLength", PropertyType::VarUint}},
            }
        }
    },
    {
        51, {
            "Polygon",
            15, {
                {125, {"points", PropertyType::VarUint}},
                {126, {"cornerRadius", PropertyType::Float}},
            }
        }
    },
    {
        11, {
            "ContainerComponent",
            10, {
            }
        }
    },
    {
        1, {
            "Artboard",
            91, {
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
        92, {
            "NestedArtboard",
            13, {
                {197, {"artboardId", PropertyType::VarUint}},
            }
        }
    },
    {
        56, {
            "StateMachineNumber",
            55, {
                {140, {"value", PropertyType::Float}},
            }
        }
    },
    {
        118, {
            "ListenerNumberChange",
            116, {
                {229, {"value", PropertyType::Float}},
            }
        }
    },
    {
        70, {
            "TransitionNumberCondition",
            69, {
                {157, {"value", PropertyType::Float}},
            }
        }
    },
    {
        31, {
            "LinearAnimation",
            27, {
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
        78, {
            "BlendStateTransition",
            65, {
                {171, {"exitBlendAnimationId", PropertyType::VarUint}},
            }
        }
    },
    {
        124, {
            "NestedNumber",
            121, {
                {239, {"nestedValue", PropertyType::Float}},
            }
        }
    },
    {
        116, {
            "ListenerInputChange",
            125, {
                {227, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        37, {
            "KeyFrameColor",
            29, {
                {88, {"value", PropertyType::Color}},
            }
        }
    },
    {
        28, {
            "CubicInterpolator",
            0, {
                {63, {"x1", PropertyType::Float}},
                {64, {"y1", PropertyType::Float}},
                {65, {"x2", PropertyType::Float}},
                {66, {"y2", PropertyType::Float}},
            }
        }
    },
    {
        30, {
            "KeyFrameDouble",
            29, {
                {70, {"value", PropertyType::Float}},
            }
        }
    },
    {
        121, {
            "NestedInput",
            10, {
                {237, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        67, {
            "TransitionCondition",
            0, {
                {155, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        59, {
            "StateMachineBool",
            55, {
                {141, {"value", PropertyType::Bool}},
            }
        }
    },
    {
        65, {
            "StateTransition",
            66, {
                {151, {"stateToId", PropertyType::VarUint}},
                {152, {"flags", PropertyType::VarUint}},
                {158, {"duration", PropertyType::VarUint}},
                {160, {"exitTime", PropertyType::VarUint}},
            }
        }
    },
    {
        115, {
            "ListenerTriggerChange",
            116, {
            }
        }
    },
    {
        57, {
            "StateMachineLayer",
            54, {
            }
        }
    },
    {
        98, {
            "NestedRemapAnimation",
            97, {
                {202, {"time", PropertyType::Float}},
            }
        }
    },
    {
        54, {
            "StateMachineComponent",
            0, {
                {138, {"name", PropertyType::String}},
            }
        }
    },
    {
        123, {
            "NestedBool",
            121, {
                {238, {"nestedValue", PropertyType::Bool}},
            }
        }
    },
    {
        95, {
            "NestedStateMachine",
            93, {
            }
        }
    },
    {
        61, {
            "AnimationState",
            60, {
                {149, {"animationId", PropertyType::VarUint}},
            }
        }
    },
    {
        71, {
            "TransitionBoolCondition",
            69, {
            }
        }
    },
    {
        63, {
            "EntryState",
            60, {
            }
        }
    },
    {
        73, {
            "BlendStateDirect",
            72, {
            }
        }
    },
    {
        125, {
            "ListenerAction",
            0, {
            }
        }
    },
    {
        62, {
            "AnyState",
            60, {
            }
        }
    },
    {
        114, {
            "StateMachineListener",
            54, {
                {224, {"targetId", PropertyType::VarUint}},
                {225, {"listenerTypeValue", PropertyType::VarUint}},
            }
        }
    },
    {
        29, {
            "KeyFrame",
            0, {
                {67, {"frame", PropertyType::VarUint}},
                {68, {"interpolationType", PropertyType::VarUint}},
                {69, {"interpolatorId", PropertyType::VarUint}},
            }
        }
    },
    {
        96, {
            "NestedSimpleAnimation",
            97, {
                {199, {"speed", PropertyType::Float}},
                {201, {"isPlaying", PropertyType::Bool}},
            }
        }
    },
    {
        122, {
            "NestedTrigger",
            121, {
            }
        }
    },
    {
        50, {
            "KeyFrameId",
            29, {
                {122, {"value", PropertyType::VarUint}},
            }
        }
    },
    {
        75, {
            "BlendAnimation1D",
            74, {
                {166, {"value", PropertyType::Float}},
            }
        }
    },
    {
        97, {
            "NestedLinearAnimation",
            93, {
                {200, {"mix", PropertyType::Float}},
            }
        }
    },
    {
        77, {
            "BlendAnimationDirect",
            74, {
                {168, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        74, {
            "BlendAnimation",
            0, {
                {165, {"animationId", PropertyType::VarUint}},
            }
        }
    },
    {
        55, {
            "StateMachineInput",
            54, {
            }
        }
    },
    {
        76, {
            "BlendState1D",
            72, {
                {167, {"inputId", PropertyType::VarUint}},
            }
        }
    },
    {
        53, {
            "StateMachine",
            27, {
            }
        }
    },
    {
        64, {
            "ExitState",
            60, {
            }
        }
    },
    {
        84, {
            "KeyFrameBool",
            29, {
                {181, {"value", PropertyType::Bool}},
            }
        }
    },
    {
        25, {
            "KeyedObject",
            0, {
                {51, {"objectId", PropertyType::VarUint}},
            }
        }
    },
    {
        58, {
            "StateMachineTrigger",
            55, {
            }
        }
    },
    {
        68, {
            "TransitionTriggerCondition",
            67, {
            }
        }
    },
    {
        26, {
            "KeyedProperty",
            0, {
                {53, {"propertyKey", PropertyType::VarUint}},
            }
        }
    },
    {
        27, {
            "Animation",
            0, {
                {55, {"name", PropertyType::String}},
            }
        }
    },
    {
        117, {
            "ListenerBoolChange",
            116, {
                {228, {"value", PropertyType::VarUint}},
            }
        }
    },
    {
        66, {
            "StateMachineLayerComponent",
            0, {
            }
        }
    },
    {
        60, {
            "LayerState",
            66, {
            }
        }
    },
    {
        72, {
            "BlendState",
            60, {
            }
        }
    },
    {
        126, {
            "ListenerAlignTarget",
            125, {
                {240, {"targetId", PropertyType::VarUint}},
            }
        }
    },
    {
        69, {
            "TransitionValueCondition",
            67, {
                {156, {"opValue", PropertyType::VarUint}},
            }
        }
    },
    {
        2, {
            "Node",
            38, {
                {13, {"x", PropertyType::Float}},
                {14, {"y", PropertyType::Float}},
            }
        }
    },
    {
        91, {
            "WorldTransformComponent",
            11, {
                {18, {"opacity", PropertyType::Float}},
            }
        }
    },
    {
        38, {
            "TransformComponent",
            91, {
                {15, {"rotation", PropertyType::Float}},
                {16, {"scaleX", PropertyType::Float}},
                {17, {"scaleY", PropertyType::Float}},
            }
        }
    },
};

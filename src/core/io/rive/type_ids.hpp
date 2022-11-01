#pragma once
/**
 * NOTE: This file is generated automatically, do not edit manually
 * To generate this file run
 *       ./external/rive_typedef.py -t ids >src/core/io/rive/type_ids.hpp
 */

namespace glaxnimate::io::rive {
enum class TypeId {
    NoType = 0,
    Artboard = 1,
    Node = 2,
    Shape = 3,
    Ellipse = 4,
    StraightVertex = 5,
    CubicDetachedVertex = 6,
    Rectangle = 7,
    Triangle = 8,
    Component = 10,
    ContainerComponent = 11,
    Path = 12,
    Drawable = 13,
    PathVertex = 14,
    ParametricPath = 15,
    PointsPath = 16,
    RadialGradient = 17,
    SolidColor = 18,
    GradientStop = 19,
    Fill = 20,
    ShapePaint = 21,
    LinearGradient = 22,
    Backboard = 23,
    Stroke = 24,
    KeyedObject = 25,
    KeyedProperty = 26,
    Animation = 27,
    CubicInterpolator = 28,
    KeyFrame = 29,
    KeyFrameDouble = 30,
    LinearAnimation = 31,
    CubicAsymmetricVertex = 34,
    CubicMirroredVertex = 35,
    CubicVertex = 36,
    KeyFrameColor = 37,
    TransformComponent = 38,
    SkeletalComponent = 39,
    Bone = 40,
    RootBone = 41,
    ClippingShape = 42,
    Skin = 43,
    Tendon = 44,
    Weight = 45,
    CubicWeight = 46,
    TrimPath = 47,
    DrawTarget = 48,
    DrawRules = 49,
    KeyFrameId = 50,
    Polygon = 51,
    Star = 52,
    StateMachine = 53,
    StateMachineComponent = 54,
    StateMachineInput = 55,
    StateMachineNumber = 56,
    StateMachineLayer = 57,
    StateMachineTrigger = 58,
    StateMachineBool = 59,
    LayerState = 60,
    AnimationState = 61,
    AnyState = 62,
    EntryState = 63,
    ExitState = 64,
    StateTransition = 65,
    StateMachineLayerComponent = 66,
    TransitionCondition = 67,
    TransitionTriggerCondition = 68,
    TransitionValueCondition = 69,
    TransitionNumberCondition = 70,
    TransitionBoolCondition = 71,
    BlendState = 72,
    BlendStateDirect = 73,
    BlendAnimation = 74,
    BlendAnimation1D = 75,
    BlendState1D = 76,
    BlendAnimationDirect = 77,
    BlendStateTransition = 78,
    Constraint = 79,
    TargetedConstraint = 80,
    IKConstraint = 81,
    DistanceConstraint = 82,
    TransformConstraint = 83,
    KeyFrameBool = 84,
    TransformComponentConstraint = 85,
    TransformComponentConstraintY = 86,
    TranslationConstraint = 87,
    ScaleConstraint = 88,
    RotationConstraint = 89,
    TransformSpaceConstraint = 90,
    WorldTransformComponent = 91,
    NestedArtboard = 92,
    NestedAnimation = 93,
    NestedStateMachine = 95,
    NestedSimpleAnimation = 96,
    NestedLinearAnimation = 97,
    NestedRemapAnimation = 98,
    Asset = 99,
    Image = 100,
    Folder = 102,
    FileAsset = 103,
    DrawableAsset = 104,
    ImageAsset = 105,
    FileAssetContents = 106,
    Vertex = 107,
    MeshVertex = 108,
    Mesh = 109,
    Text = 110,
    ContourMeshVertex = 111,
    ForcedEdge = 112,
    TextRun = 113,
    StateMachineListener = 114,
    ListenerTriggerChange = 115,
    ListenerInputChange = 116,
    ListenerBoolChange = 117,
    ListenerNumberChange = 118,
    LayeredAsset = 119,
    LayerImageAsset = 120,
    NestedInput = 121,
    NestedTrigger = 122,
    NestedBool = 123,
    NestedNumber = 124,
    ListenerAction = 125,
    ListenerAlignTarget = 126,
    // v6
    PathComposer = 9,
    StateMachineDouble = 56,
    TransitionDoubleCondition = 70,
};
} // namespace glaxnimate::io::rive

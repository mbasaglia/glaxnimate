# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later
import glaxnimate


class HueShiftVisitor(glaxnimate.model.Visitor):
    def __init__(self, amount, times, easing):
        super().__init__()
        self.amount = amount
        self.gradients = set()
        self.named_colors = set()
        self.times = times
        self.easing = easing

    def _color(self, color, amount):
        return glaxnimate.utils.Color.from_hsv(
            (color.hue + amount) % 360,
            color.saturation,
            color.value,
            color.alpha
        )

    def _prop_color(self, value, amount):
        if isinstance(value, list):
            return [
                (offset, self._color(color, amount))
                for offset, color in value
            ]
        return self._color(value, amount)

    def set_time(self, time):
        self.gradients = set()
        self.named_colors = set()
        self.time = time

    def _set_value(self, prop):
        value = prop.value
        if not self.times:
            prop.value = self._prop_color(value, self.amount)
        else:
            amount = 0
            for time in self.times:
                kf = prop.set_keyframe(time, self._prop_color(value, amount))
                kft = kf.transition
                kft.before_descriptive = self.easing
                kft.after_descriptive = self.easing
                kf.transition = kft
                amount += self.amount

    def on_visit_node(self, node):
        if isinstance(node, glaxnimate.model.shapes.Styler):
            self._set_value(node.color)
            if node.use:
                if isinstance(node.use, glaxnimate.model.assets.NamedColor):
                    if node.use not in self.named_colors:
                        self._set_value(node.use.color)
                        self.named_colors.add(node.use)
                elif isinstance(node.use, glaxnimate.model.assets.Gradient) and node.use.colors:
                    if node.use.colors not in self.gradients:
                        self._set_value(node.use.colors.colors)
                        self.gradients.add(node.use.colors)


def hue_shift(window, document, settings):
    with document.macro("Hue Shift"):
        visitor = HueShiftVisitor(settings["amount"], None, None)
        for shape in window.cleaned_selection:
            visitor.visit(shape, True)


def hue_shift_cycle(window, document, settings):
    with document.macro("Hue Shift Cycle"):
        easing_str = settings["easing"]
        easing = glaxnimate.model.KeyframeTransition.Descriptive.Linear
        if easing_str == "Ease":
            easing = glaxnimate.model.KeyframeTransition.Descriptive.Ease
        elif easing_str == "Hold":
            easing = glaxnimate.model.KeyframeTransition.Descriptive.Hold

        amount = settings["amount"]
        duration = settings["duration"]
        times = [
            document.current_time + step * duration
            for step in range(settings["step_count"])
        ]

        visitor = HueShiftVisitor(amount, times, easing)

        for shape in window.cleaned_selection:
            visitor.visit(shape, True)






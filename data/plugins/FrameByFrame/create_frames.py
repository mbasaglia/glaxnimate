# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later
def main(window, document, settings):
    dialog = window.create_dialog("create_frames.ui")

    if not dialog:
        return

    dialog.set_value("spin_first", "minimum", document.main.animation.first_frame)
    dialog.set_value("spin_first", "maximum", document.main.animation.last_frame)

    dialog.set_value("spin_last", "minimum", document.main.animation.first_frame)
    dialog.set_value("spin_last", "maximum", document.main.animation.last_frame)

    dialog.set_value("spin_first", "value", document.main.animation.first_frame)
    dialog.set_value("spin_last", "value", document.main.animation.last_frame)

    initial = (document.main.animation.last_frame - document.main.animation.first_frame) / document.main.fps * 24
    dialog.set_value("spin_count", "value", int(initial))

    if dialog.exec():
        first = dialog.get_value("spin_first", "value")
        last = dialog.get_value("spin_last", "value")
        count = dialog.get_value("spin_count", "value")

        if first == last:
            return

        if first > last:
            (last, first) = (first, last)

        duration = (last - first) / count
        with document.macro("Create frame layers"):
            for i in range(count):
                layer = window.current_composition.add_shape("Layer")
                layer.name = "Frame %s" % i
                layer.animation.first_frame = duration * i
                layer.animation.last_frame = duration * (i+1)

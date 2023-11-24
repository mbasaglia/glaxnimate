# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later
import os
from PIL import Image
from PIL import features


def _png_gif_prepare(image):
    if image.mode not in ["RGBA", "RGBa"]:
        image = image.convert("RGBA")
    alpha = image.getchannel("A")
    image = image.convert("RGB").convert('P', palette=Image.ADAPTIVE, colors=255)
    mask = Image.eval(alpha, lambda a: 255 if a <= 128 else 0)
    image.paste(255, mask=mask)
    return image


def save_gif(window, document, comp, file, name, import_export, settings):
    skip_frames = 1
    start = int(comp.animation.first_frame)
    end = int(comp.animation.last_frame)

    import_export.progress_max_changed(end-start)

    frames = []
    for i in range(start, end+1, skip_frames):
        import_export.progress(i-start)
        frames.append(_png_gif_prepare(comp.render_image(i)))

    duration = int(round(1000 / comp.fps * skip_frames / 10)) * 10
    frames[0].save(
        file,
        format='GIF',
        append_images=frames[1:],
        save_all=True,
        duration=duration,
        loop=0,
        transparency=255,
        disposal=2,
    )


def save_webp(window, document, comp, file, name, import_export, settings):
    if not features.check("webp_anim"):
        window.warning("WebP animations not supported in this system")
        return

    skip_frames = settings["skip_frames"]
    dpi = 96

    start = int(comp.animation.first_frame)
    end = int(comp.animation.last_frame)
    import_export.progress_max_changed(end-start)

    frames = []
    for i in range(start, end+1, skip_frames):
        import_export.progress(i-start)
        frames.append(comp.render_image(i))

    duration = int(round(1000 / comp.fps * skip_frames))
    frames[0].save(
        file,
        format='WebP',
        append_images=frames[1:],
        save_all=True,
        duration=duration,
        loop=0,
        background=(0, 0, 0, 0),
        lossless=settings["lossless"],
        quality=settings["quality"],
        method=settings["method"]
    )


def open_image(window, document, file, name, import_export, settings):
    raster = Image.open(file)
    comp = document.assets.compositions.values[0]
    comp.width = raster.width
    comp.height = raster.height

    if not hasattr(raster, "is_animated"):
        raster.n_frames = 1
        raster.seek = lambda x: None

    parent_layer = comp.add_shape("Layer")
    parent_layer.name = comp.name = os.path.basename(name)
    time = 0

    for frame in range(raster.n_frames):
        layer = parent_layer.add_shape("Layer")
        layer.name = "Frame %s Layer" % frame
        layer.animation.first_frame = time

        raster.seek(frame)

        if "duration" in raster.info:
            time += raster.info.get("duration") / 1000 * comp.fps
        else:
            time += 1
        layer.animation.last_frame = time

        asset = document.assets.add_image(raster)

        shape = layer.add_shape("Image")
        shape.image = asset
        shape.name = "Frame %s" % frame

    comp.animation.last_frame = parent_layer.animation.last_frame = time

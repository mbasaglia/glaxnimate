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


def save_gif(window, document, file, name, import_export, settings):
    skip_frames = 1
    start = int(document.main.animation.first_frame)
    end = int(document.main.animation.last_frame)

    import_export.progress_max_changed(end-start)

    frames = []
    for i in range(start, end+1, skip_frames):
        import_export.progress(i-start)
        frames.append(_png_gif_prepare(document.render_image(i)))

    duration = int(round(1000 / document.main.fps * skip_frames / 10)) * 10
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


def save_webp(window, document, file, name, import_export, settings):
    if not features.check("webp_anim"):
        window.warning("WebP animations not supported in this system")
        return

    skip_frames = settings["skip_frames"]
    dpi = 96

    start = int(document.main.animation.first_frame)
    end = int(document.main.animation.last_frame)
    import_export.progress_max_changed(end-start)

    frames = []
    for i in range(start, end+1, skip_frames):
        import_export.progress(i-start)
        frames.append(document.render_image(i))

    duration = int(round(1000 / document.main.fps * skip_frames))
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

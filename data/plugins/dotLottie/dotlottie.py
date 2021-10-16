import io
import re
import os
import json
import string
import base64
import zipfile
from PIL import Image

import glaxnimate


def asset_data(asset):
    if asset["e"]:
        m = re.match("data:[^/]+/([^;,]+);base64,(.*)", asset["p"])
        if not m:
            return None, None
        return m.group(1), base64.b64decode(m.group(2))

    path = os.path.join(asset["u"], asset["p"])
    if not os.path.isfile(path):
        return None, None

    with open(path, "rb") as imgfile:
        return os.path.splitext(path)[1][1:], imgfile.read()


def save_dotlottie(window, document, file, fname, import_export, settings):
    files = {}

    if settings["append"]:
        with zipfile.ZipFile(fname, "r") as zf:
            with zf.open("manifest.json") as manifest:
                meta = json.load(manifest)
                meta["custom"].update(document.metadata)

            for name in zf.namelist():
                if name != "manifest.json":
                    files[name] = zf.read(name)
    else:
        meta = {
            "generator": "Glaxnimate " + glaxnimate.__version__,
            "version": 1.0,
            "revision": 1,
            "author": str(document.metadata.get("author", "")),
            "animations": [],
            "custom": document.metadata,
            "author": document.info.author,
        }

    try:
        meta["revision"] = int(document.metadata["revision"])
    except (KeyError, ValueError):
        pass

    id = settings["id"]
    if not id:
        if document.main.name:
            idok = string.ascii_letters + string.digits + "_-"
            id = "".join(filter(lambda x: x in idok, document.main.name.replace(" ", "_")))
        if not id:
            id = "animation_%s" % len(meta["animations"])

    meta["animations"].append({
        "id": id,
        "speed": settings["speed"],
        "themeColor": settings["theme_color"],
        "loop": settings["loop"],
    })

    lottie_str = glaxnimate.io.registry["lottie"].save(document)
    lottie_dict = json.loads(lottie_str)

    if settings["pack_assets"]:
        image_no = 0
        for asset in lottie_dict.get("assets", []):
            if "e" not in asset or "p" not in asset:
                continue

            ext, data = asset_data(asset)
            if not ext:
                continue

            pathname = "images/"
            while True:
                basename = "image_%s.%s" % (image_no, ext)
                image_no += 1
                if pathname+basename not in files:
                    break
            files[pathname+basename] = data
            asset["u"] = pathname
            asset["p"] = basename
            asset["e"] = 0

    files["manifest.json"] = json.dumps(meta)
    files["animations/%s.json" % id] = json.dumps(lottie_dict)

    with open(fname, "wb") as fd:
        with zipfile.ZipFile(fd, "w") as zf:
            for name, data in files.items():
                zf.writestr(name, data)


def load_asset(asset, file):
    image = Image.open(file)
    asset["u"] = ""
    asset["p"] = ""
    format = (image.format or "png").lower()
    asset["w"], asset["h"] = image.size
    output = io.BytesIO()
    image.save(output, format=format)
    asset["p"] = "data:image/%s;base64,%s" % (
        format,
        base64.b64encode(output.getvalue()).decode("ascii")
    )
    asset["e"] = 1


def open_dotlottie(window, document, file, fname, import_export, settings):
    file.open("rb")
    with zipfile.ZipFile(file, "r") as zf:
        with zf.open("manifest.json") as manifest:
            meta = json.load(manifest)

        # TODO dialog showing available animations
        id = meta["animations"][0]["id"]

        info = zf.getinfo("animations/%s.json" % id)
        with zf.open(info) as animfile:
            lottie = json.load(animfile)

        for asset in lottie.get("assets", []):
            if "e" not in asset or "p" not in asset and not asset["e"]:
                continue
            asset_fname = os.path.join(asset["u"], asset["p"])
            if asset_fname in zf.namelist():
                with zf.open(asset_fname) as asset_file:
                    load_asset(asset, asset_file)

        glaxnimate.io.registry["lottie"].load(document, json.dumps(lottie).encode("utf-8"))

        meta["custom"]["author"] = meta["author"]
        meta["custom"]["revision"] = meta["revision"]
        document.metadata = meta["custom"]

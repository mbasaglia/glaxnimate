#ifndef ANDROID_MIME_HPP
#define ANDROID_MIME_HPP

#include "io/glaxnimate/glaxnimate_mime.hpp"

namespace glaxnimate::android {

// QClipboard seems to only retain text/plain on android...
class AndroidMime : public io::glaxnimate::GlaxnimateMime
{
    QString slug() const override { return "android_json"; }
    QStringList mime_types() const override { return {"text/plain"}; }
};


} // glaxnimate::android

#endif // ANDROID_MIME_HPP

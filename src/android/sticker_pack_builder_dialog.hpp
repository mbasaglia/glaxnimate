#ifndef STICKER_PACK_BUILDER_DIALOG_HPP
#define STICKER_PACK_BUILDER_DIALOG_HPP

#include <memory>

#include "base_dialog.hpp"
#include "model/document.hpp"

namespace glaxnimate::android {

class StickerPackBuilderDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit StickerPackBuilderDialog(QWidget *parent = nullptr);
    ~StickerPackBuilderDialog();
    void set_current_file(model::Document* current);

protected:
    void changeEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android
#endif // STICKER_PACK_BUILDER_DIALOG_HPP

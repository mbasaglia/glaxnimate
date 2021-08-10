#ifndef SKELETONTOOLWIDGET_H
#define SKELETONTOOLWIDGET_H

#include <memory>
#include <QWidget>

#include "model/skeleton/skeleton.hpp"


namespace glaxnimate::gui {

class SkeletonToolWidget : public QWidget
{
    Q_OBJECT

public:
    SkeletonToolWidget(QWidget* parent = nullptr);
    ~SkeletonToolWidget();

    void clear();
    void set_skeleton(model::Skeleton* skeleton);
    void set_bone(model::Bone* bone);
    void set_slot(model::SkinSlot* slot);
    void set_skin(model::Skin* skin);
    void set_skin_item(model::SkinItem* skin_item);

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // glaxnimate::gui

#endif // SKELETONTOOLWIDGET_H

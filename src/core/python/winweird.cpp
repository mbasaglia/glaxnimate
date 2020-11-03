#include "command/object_list_commands.hpp"
#include "model/shapes/shape.hpp"
#include "model/defs/defs.hpp"

#define WINWEIRD_DECL(ItemT)                                                        \
    command::AddObject<ItemT, model::ObjectListProperty<ItemT>>*                    \
    winweird(model::ObjectListProperty<ItemT>* propptr, ItemT* object, int index) { \
        return new command::AddObject<ItemT, model::ObjectListProperty<ItemT>>(     \
            propptr, std::unique_ptr<ItemT>(object), index                          \
    ); }                                                                            \
    // end of macro

WINWEIRD_DECL(model::ShapeElement)

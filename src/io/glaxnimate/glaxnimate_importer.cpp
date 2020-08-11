#include "glaxnimate_format.hpp"

#include <QUuid>
#include <QJsonArray>


class io::glaxnimate::GlaxnimateFormat::ImportState
{
public:
    GlaxnimateFormat* fmt;
    model::Document* document = nullptr;
    model::Composition* compostion = nullptr;
    QMap<QString, model::DocumentNode*> references;
    QMap<model::BaseProperty*, QUuid> unresolved_references;
    QMap<model::Object*, QJsonObject> deferred_loads;
    std::vector<model::Object*> unwanted;

    ImportState(GlaxnimateFormat* fmt) : fmt(fmt) {}

    ~ImportState() {}

    void resolve()
    {
        for ( auto it = unresolved_references.begin(); it != unresolved_references.end(); ++it )
        {
            model::BaseProperty* prop = it.key();
            model::DocumentNode* node = document->node_by_uuid(*it);
            if ( !node )
            {
                emit fmt->error(tr("Property %1 of %2 refers to unexisting object %3")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                    .arg(it->toString())
                );
            }
            else
            {
                if ( !prop->set_value(QVariant::fromValue(node)) )
                    emit fmt->error(tr("Could not load %1 for %2: uuid refers to an unacceptable object")
                        .arg(prop->name())
                        .arg(prop->object()->object_name())
                    );
            }
        }

        for ( model::Object* obj : unwanted )
        {
            if ( obj )
            {
                emit fmt->error(tr("Object %1 is invalid").arg(obj->object_name()));
                delete obj;
            }
        }
    }

    void load_object ( model::Object* target, const QJsonObject& object )
    {
        QString type = object["__type__"].toString();
        if ( type != target->type_name() )
            emit fmt->error(tr("Wrong object type: expected '%1' but got '%2'").arg(target->type_name()).arg(type));

        for ( model::BaseProperty* prop : target->properties() )
        {
            if ( !load_prop(prop, object[prop->name()]) )
                emit fmt->error(tr("Could not load %1 for %2")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                );
        }

        for ( auto it = object.begin(); it != object.end(); ++it )
        {
            if ( !target->has(it.key()) && it.key() != "__type__" )
            {
                target->set(it.key(), it->toVariant(), true);
            }
        }
    }

    bool load_prop ( model::BaseProperty* target, const QJsonValue& val )
    {
        if ( target->traits().flags & model::PropertyTraits::List )
        {
            if ( !val.isArray() )
                return false;

            QVariantList list;
            for ( const QJsonValue& item : val.toArray() )
                list.push_back(load_prop_value(target, item, false));

            if ( target->traits().type == model::PropertyTraits::Object )
            {
                int index = 0;
                for ( const QVariant& item : list )
                {
                    auto ptr = item.value<model::Object*>();
                    model::ObjectListPropertyBase* prop = static_cast<model::ObjectListPropertyBase*>(target);
                    if ( !ptr )
                    {
                        emit fmt->error(
                            tr("Item %1 for %2 in %3 isn't an object")
                            .arg(index)
                            .arg(target->name())
                            .arg(target->object()->object_name())
                        );
                    }
                    else
                    {
                        auto inserted = prop->insert_clone(ptr);
                        if ( !inserted )
                        {
                            emit fmt->error(
                                tr("Item %1 for %2 in %3 is not acceptable")
                                .arg(index)
                                .arg(target->name())
                                .arg(target->object()->object_name())
                            );
                        }
                        else
                        {
                            load_object(inserted, deferred_loads[ptr]);
                        }
                        deferred_loads.remove(ptr);
                    }
                    index++;
                }

                return true;
            }
            else
            {
                return target->set_value(list);
            }
        }


        if ( target->traits().type == model::PropertyTraits::ObjectReference )
        {
            QUuid uuid(val.toString());
            if ( !uuid.isNull() )
                unresolved_references[target] = uuid;
            return true;
        }
        else if ( target->traits().type == model::PropertyTraits::Uuid )
        {
            QUuid uuid(val.toString());
            if ( uuid.isNull() )
                return false;
            return target->set_value(uuid);
        }

        QVariant loaded_val = load_prop_value(target, val, true);
        if ( !target->set_value(loaded_val) )
        {
            if ( target->traits().type == model::PropertyTraits::Object )
                unwanted.push_back(loaded_val.value<model::Object*>());
            return false;
        }
        return true;
    }


    QVariant load_prop_value ( model::BaseProperty* target, const QJsonValue& val, bool load_objects )
    {
        switch ( target->traits().type )
        {
            case model::PropertyTraits::Object:
            {
                if ( !val.isObject() )
                    return {};
                QJsonObject jobj = val.toObject();
                model::Object* object = create_object(jobj["__type__"].toString());
                if ( !object )
                    return {};
                if ( load_objects )
                    load_object(object, jobj);
                else
                    deferred_loads.insert(object, jobj);
                return  QVariant::fromValue(object);
            }
            case model::PropertyTraits::ObjectReference:
            case model::PropertyTraits::Uuid:
                // handled above
                return {};
            case model::PropertyTraits::Color:
            {
                QString name = val.toString();
                // We want #rrggbbaa, qt does #aarrggbb
                if ( name.startsWith("#") && name.size() == 9 )
                {
                    int alpha = name.right(2).toInt(nullptr, 16);
                    QColor col(name);
                    col.setAlpha(alpha);
                    return col;
                }
                return QColor(name);

            }
            case model::PropertyTraits::Point:
            {
                QJsonObject obj = val.toObject();
                if ( obj.empty() )
                    return {};
                return QPointF(obj["x"].toDouble(), obj["y"].toDouble());
            }
            case model::PropertyTraits::Size:
            {
                QJsonObject obj = val.toObject();
                if ( obj.empty() )
                    return QVariant{};
                return QSizeF(obj["width"].toDouble(), obj["height"].toDouble());
            }
            case model::PropertyTraits::Scale:
            {
                QJsonObject obj = val.toObject();
                if ( obj.empty() )
                    return QVariant{};
                return QVector2D(obj["x"].toDouble(), obj["y"].toDouble());
            }
            default:
                return val.toVariant();
        }
    }

    /// @todo Find a way of automating this
    model::Object* create_object(const QString& type)
    {
        if ( type == "Animation" )
        {
            emit fmt->error(tr("Objects of type 'Animation' can only be at the top level of the document"));
            return nullptr;
        }

        if ( type == "EmptyLayer" )
            return new model::EmptyLayer(document, compostion);

        if ( type == "ShapeLayer" )
            return new model::ShapeLayer(document, compostion);

        if ( type == "SolidColorLayer" )
            return new model::SolidColorLayer(document, compostion);

        if ( type == "Transform" )
            return new model::Transform(document);

        emit fmt->error(tr("Unknow object of type '%1'").arg(type));
        return new model::Object(document);
    }

};

bool io::glaxnimate::GlaxnimateFormat::on_open ( QIODevice& file, const QString&, model::Document* document, const QVariantMap& )
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(file.readAll());
    } catch ( const QJsonParseError& err ) {
        emit error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        emit error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    /// @todo check / handle format version
    // int document_format_version = top_level["format"].toObject()["format_version"].toInt(0);

    document->metadata() = top_level["metadata"].toObject().toVariantMap();

    if ( !top_level["animation"].isObject() )
    {
        emit error(tr("Missing animation object"));
        return false;
    }

    ImportState state(this);
    state.document = document;
    state.compostion = document->animation();
    state.load_object(document->animation(), top_level["animation"].toObject());
    state.resolve();

    return true;
}

#include "glaxnimate_format.hpp"

#include <QUuid>
#include <QJsonArray>

class io::glaxnimate::GlaxnimateFormat::ImportState
{
public:
    model::Document* document = nullptr;
    model::Composition* compostion = nullptr;
    QMap<QString, model::DocumentNode*> references;
    QMap<model::BaseProperty*, QString> unresolved_references;
    std::vector<model::Object*> unwanted;

    ~ImportState()
    {
        for ( model::Object* obj : unwanted )
            if ( obj )
                delete obj;
    }

    bool resolve()
    {
        return unresolved_references.empty();
    }
};

bool io::glaxnimate::GlaxnimateFormat::open ( QIODevice& file, const QString&, model::Document* document, const QVariantMap& )
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

    ImportState state;
    state.document = document;
    state.compostion = &document->animation();
    load_object(&document->animation(), top_level["animation"].toObject(), state);

    if ( !state.resolve() )
        emit error(tr("Could not resolve some references"));

    return true;
}

void io::glaxnimate::GlaxnimateFormat::load_object ( model::Object* target, const QJsonObject& object, ImportState& state )
{
    QString type = object["__type__"].toString();
    if ( type != target->type_name() )
        emit error(tr("Wrong object type: expected '%1' but got '%2'").arg(target->type_name()).arg(type));

    for ( model::BaseProperty* prop : target->properties() )
    {
        if ( !load_prop(prop, object[prop->name()], state) )
            emit error(tr("Could not load %1").arg(prop->name()));
    }

    for ( auto it = object.begin(); it != object.end(); ++it )
    {
        if ( !target->has(it.key()) && it.key() != "__type__" )
        {
            target->set(it.key(), it->toVariant(), true);
        }
    }
}

bool io::glaxnimate::GlaxnimateFormat::load_prop ( model::BaseProperty* target, const QJsonValue& val, ImportState& state )
{
    if ( target->traits().list )
    {
        if ( !val.isArray() )
            return false;

        QVariantList list;
        for ( const QJsonValue& item : val.toArray() )
            list.push_back(load_prop_value(target, item, state));

        bool ok = target->set_value(list);
        if ( target->traits().type == model::PropertyTraits::Object )
        {
            if ( !ok )
            {
                for ( const QVariant& item : list )
                    state.unwanted.push_back(item.value<model::Object*>());
            }
            else
            {
                QVariantList actual_value = target->value().toList();
                QSet<model::Object*> made_it;
                for ( const QVariant& item : actual_value )
                    if ( auto ptr = item.value<model::Object*>() )
                        made_it.insert(ptr);

                for ( const QVariant& item : list )
                {
                    auto ptr = item.value<model::Object*>();
                    if ( !made_it.contains(ptr) )
                        state.unwanted.push_back(ptr);
                }
            }
        }
        return ok;
    }


    if ( target->traits().type == model::PropertyTraits::ObjectReference )
    {
        state.unresolved_references[target] = val.toString();
        return true;
    }
    else if ( target->traits().type == model::PropertyTraits::Uuid )
    {
        QUuid uuid = QUuid::fromString(val.toString());
        if ( uuid.isNull() )
            return false;
        return target->set_value(uuid);
    }

    QVariant loaded_val = load_prop_value(target, val, state);
    if ( !target->set_value(loaded_val) )
    {
        if ( target->traits().type == model::PropertyTraits::Object )
            state.unwanted.push_back(loaded_val.value<model::Object*>());
        return false;
    }
    return true;
}


QVariant io::glaxnimate::GlaxnimateFormat::load_prop_value ( model::BaseProperty* target, const QJsonValue& val, ImportState& state )
{
    switch ( target->traits().type )
    {
        case model::PropertyTraits::Object:
        {
            if ( !val.isObject() )
                return {};
            QJsonObject jobj = val.toObject();
            model::Object* object = create_object(jobj["__type__"].toString(), state);
            if ( !object )
                return {};
            load_object(object, jobj, state);
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
            if ( !obj.empty() )
                return {};
            return QPointF(obj["x"].toDouble(), obj["y"].toDouble());
        }
        case model::PropertyTraits::Size:
        {
            QJsonObject obj = val.toObject();
            if ( !obj.empty() )
                return QVariant{};
            return QSizeF(obj["width"].toDouble(), obj["height"].toDouble());
        }
        default:
            return val.toVariant();
    }
}

model::Object* io::glaxnimate::GlaxnimateFormat::create_object(const QString& type, ImportState& state)
{
    if ( type == "Animation" )
    {
        emit error(tr("Objects of type 'Animation' can only be at the top level of the document"));
        return nullptr;
    }

    if ( type == "EmptyLayer" )
        return new model::EmptyLayer(state.document, state.compostion);

    if ( type == "ShapeLayer" )
        return new model::ShapeLayer(state.document, state.compostion);

    emit error(tr("Unknow object of type '%1'").arg(type));
    return new model::Object();
}

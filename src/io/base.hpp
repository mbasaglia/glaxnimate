#pragma once

#include "io/options.hpp"
#include "model/document.hpp"

#include <QFileInfo>
#include <QDebug>
#include <QObject>

namespace io {

class ImportExport : QObject
{
    Q_OBJECT

public:
    virtual ~ImportExport() = default;

    bool can_handle_extension(const QString& extension) const
    {
        return extension.contains(extension);
    }

    bool can_handle_filename(const QString& filename) const
    {
        return can_handle_extension(QFileInfo(filename).completeSuffix());
    }

    virtual bool process(const QString& filename, model::Document* document) const = 0;

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QStringList extensions() const = 0;
    virtual OptionList options() const = 0;

signals:
    void error(const QString& message);
    void progress_max_changed(int max);
    void progress(int value);
};


template<class Derived>
class ImportExportFactory
{
public:
    void register_object(std::unique_ptr<Derived> ie)
    {
        object_list.push_back(std::move(ie));
    }

    const std::vector<std::unique_ptr<Derived>>& registered()
    {
        return object_list;
    }

    Derived* from_extension(const QString& extension)
    {
        for ( const auto& p : object_list )
            if ( p->can_handle_extension(extension) )
                return p.get();

        return nullptr;
    }

    Derived* from_filename(const QString& filename)
    {
        for ( const auto& p : object_list )
            if ( p->can_handle_filename(filename) )
                return p.get();

        return nullptr;
    }

    bool process(const QString& filename, model::Document* document)
    {
        for ( const auto& p : object_list )
            if ( p->can_handle_filename(filename) )
                return p->process(filename, document);
        return false;
    }

protected:
    std::vector<std::unique_ptr<Derived>> object_list;
};


template<class Derived, class Base>
class ImportExportAutoreg
{
public:
    template<class... Args>
    ImportExportAutoreg(Args&&... args)
    {
        Base::factory().register_object(std::make_unique<Derived>(std::forward<Args>(args)...));
    }
};

template<class Derived, class Base>
class ImportExportConcrete : public Base
{
protected:
    using Autoreg = ImportExportAutoreg<Derived, Base>;
};




} // namespace io


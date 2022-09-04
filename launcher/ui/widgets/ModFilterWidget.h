#pragma once

#include <QTabWidget>
#include <QButtonGroup>

#include "Version.h"

#include "meta/Index.h"
#include "meta/VersionList.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

class MinecraftInstance;

namespace Ui {
class ModFilterWidget;
}

class ModFilterWidget : public QTabWidget
{
    Q_OBJECT
public:
    enum VersionButtonID {
        Strict = 0,
        Major = 1,
        All = 2,
        Between = 3
    };

    enum LoaderButtonID {
        Fabric = 0,
        Forge = 1,
        Quilt = 2
    };

    struct Filter {
        std::list<Version> versions;
        ModAPI::ModLoaderTypes mod_loaders;

        bool operator==(const Filter& other) const { return versions == other.versions && mod_loaders == other.mod_loaders; }
        bool operator!=(const Filter& other) const { return !(*this == other); }
    };

    std::shared_ptr<Filter> m_filter;

public:
    struct Config {
        Version default_version;

        bool allow_multiple_loaders = true;
        ModAPI::ModLoaderTypes default_loaders;
    };
    static unique_qobject_ptr<ModFilterWidget> create(Config&& conf, QWidget* parent = nullptr);
    ~ModFilterWidget();

    void setInstance(MinecraftInstance* instance);

    /// By default all buttons are enabled
    void disableVersionButton(VersionButtonID, QString reason = {});

    auto getFilter() -> std::shared_ptr<Filter>;
    auto changed() const -> bool;

    Meta::VersionListPtr versionList() { return m_version_list; }

private:
    ModFilterWidget(Config&& conf, QWidget* parent = nullptr);

    inline auto mcVersionStr() const -> QString { return m_instance ? m_instance->getPackProfile()->getComponentVersion("net.minecraft") : ""; }
    inline auto mcVersion() const -> Version { return { mcVersionStr() }; }

private slots:
    void onVersionFilterChanged(int id);
    void onLoaderFilterChanged(int id);

public: signals:
    void filterChanged();
    void filterUnchanged();

private:
    Ui::ModFilterWidget* ui;

    MinecraftInstance* m_instance = nullptr;

/* Loader stuff */
    QButtonGroup m_loader_buttons;

    /* Used to tell if the filter was changed since the last getFilter() call */
    ModAPI::ModLoaderTypes m_last_loader_state;

/* Version stuff */
    QButtonGroup m_mcVersion_buttons;

    Meta::VersionListPtr m_version_list;

    /* Used to tell if the filter was changed since the last getFilter() call */
    VersionButtonID m_last_version_id = VersionButtonID::Strict;
    VersionButtonID m_version_id = VersionButtonID::Strict;
};

#include "ModFilterWidget.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"

#include <QCheckBox>

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(Config&& conf, QWidget* parent)
{
    auto filter_widget = new ModFilterWidget(std::move(conf), parent);

    if (!filter_widget->versionList()->isLoaded()) {
        QEventLoop load_version_list_loop;

        QTimer time_limit_for_list_load;
        time_limit_for_list_load.setTimerType(Qt::TimerType::CoarseTimer);
        time_limit_for_list_load.setSingleShot(true);
        time_limit_for_list_load.callOnTimeout(&load_version_list_loop, &QEventLoop::quit);
        time_limit_for_list_load.start(4000);

        auto task = filter_widget->versionList()->getLoadTask();

        connect(task.get(), &Task::failed, [filter_widget]{
            filter_widget->disableVersionButton(VersionButtonID::Major, tr("failed to get version index"));
        });
        connect(task.get(), &Task::finished, &load_version_list_loop, &QEventLoop::quit);

        if (!task->isRunning())
            task->start();

        load_version_list_loop.exec();
        if (time_limit_for_list_load.isActive())
            time_limit_for_list_load.stop();
    }

    return unique_qobject_ptr<ModFilterWidget>(filter_widget);
}

ModFilterWidget::ModFilterWidget(Config&& conf, QWidget* parent)
    : QTabWidget(parent), m_filter(new Filter()),  ui(new Ui::ModFilterWidget)
{
    // Check that there's only one default loader if the 'allow_multiple_loaders' flag is false
    Q_ASSERT(conf.allow_multiple_loaders || ((conf.default_loaders - 1) & conf.default_loaders) == 0);

    ui->setupUi(this);

    {
        m_mcVersion_buttons.addButton(ui->strictVersionButton,   VersionButtonID::Strict);
        ui->strictVersionButton->click();
        m_mcVersion_buttons.addButton(ui->majorVersionButton,    VersionButtonID::Major);
        m_mcVersion_buttons.addButton(ui->allVersionsButton,     VersionButtonID::All);
        //m_mcVersion_buttons.addButton(ui->betweenVersionsButton, VersionButtonID::Between);

        connect(&m_mcVersion_buttons, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), this, &ModFilterWidget::onVersionFilterChanged);
    }

    {
        QAbstractButton *fabric_btn = nullptr, *forge_btn = nullptr, *quilt_btn = nullptr;
        if (conf.allow_multiple_loaders) {
            fabric_btn = new QCheckBox(tr("Fabric"), ui->LoaderPage);
            forge_btn = new QCheckBox(tr("Forge"), ui->LoaderPage);
            quilt_btn = new QCheckBox(tr("Quilt"), ui->LoaderPage);
        } else {
            fabric_btn = new QRadioButton(tr("Fabric"), ui->LoaderPage);
            forge_btn = new QRadioButton(tr("Forge"), ui->LoaderPage);
            quilt_btn = new QRadioButton(tr("Quilt"), ui->LoaderPage);
        }

        m_loader_buttons.addButton(fabric_btn, LoaderButtonID::Fabric);
        m_loader_buttons.addButton(forge_btn, LoaderButtonID::Forge);
        m_loader_buttons.addButton(quilt_btn, LoaderButtonID::Quilt);

        m_loader_buttons.setExclusive(!conf.allow_multiple_loaders);

        ui->loaderPageLayout->setWidget(0, QFormLayout::LabelRole, fabric_btn);
        ui->loaderPageLayout->setWidget(1, QFormLayout::LabelRole, forge_btn);
        ui->loaderPageLayout->setWidget(2, QFormLayout::LabelRole, quilt_btn);

        if (conf.default_loaders.testFlag(ModAPI::Fabric))
            m_loader_buttons.button(LoaderButtonID::Fabric)->setChecked(true);
        if (conf.default_loaders.testFlag(ModAPI::Forge))
            m_loader_buttons.button(LoaderButtonID::Forge)->setChecked(true);
        if (conf.default_loaders.testFlag(ModAPI::Quilt))
            m_loader_buttons.button(LoaderButtonID::Quilt)->setChecked(true);

        connect(&m_loader_buttons, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), this, &ModFilterWidget::onLoaderFilterChanged);
    }

    m_filter->versions.push_front(conf.default_version);
    m_filter->mod_loaders = conf.default_loaders;

    m_version_list = APPLICATION->metadataIndex()->get("net.minecraft");
    setHidden(true);
}

void ModFilterWidget::setInstance(MinecraftInstance* instance)
{
    m_instance = instance;

    {
        ui->strictVersionButton->setText(
            tr("Strict match (= %1)").arg(mcVersionStr()));

        // we can't do this for snapshots sadly
        if(mcVersionStr().contains('.'))
        {
            auto mcVersionSplit = mcVersionStr().split(".");
            ui->majorVersionButton->setText(
                tr("Major version match (= %1.%2.x)").arg(mcVersionSplit[0], mcVersionSplit[1]));
        }
        else
        {
            ui->majorVersionButton->setText(tr("Major version match (unsupported)"));
            disableVersionButton(Major);
        }
        ui->allVersionsButton->setText(
            tr("Any version"));
        //ui->betweenVersionsButton->setText(
        //    tr("Between two versions"));
    }
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_last_version_id = m_version_id;
    m_last_loader_state = m_filter->mod_loaders;

    emit filterUnchanged();

    return m_filter;
}

bool ModFilterWidget::changed() const
{
    if (m_last_version_id != m_version_id)
        return true;
    if (m_last_loader_state != m_filter->mod_loaders)
        return true;
    return false;
}

void ModFilterWidget::disableVersionButton(VersionButtonID id, QString reason)
{
    QAbstractButton* btn = nullptr;

    switch(id){
    case(VersionButtonID::Strict):
        btn = ui->strictVersionButton;
        break;
    case(VersionButtonID::Major):
        btn = ui->majorVersionButton;
        break;
    case(VersionButtonID::All):
        btn = ui->allVersionsButton;
        break;
    case(VersionButtonID::Between):
    default:
        break;
    }

    if (btn) {
        btn->setEnabled(false);
        if (!reason.isEmpty())
            btn->setText(btn->text() + QString(" (%1)").arg(reason));
    }
}

void ModFilterWidget::onVersionFilterChanged(QAbstractButton* btn)
{
    //ui->lowerVersionComboBox->setEnabled(id == VersionButtonID::Between);
    //ui->upperVersionComboBox->setEnabled(id == VersionButtonID::Between);

    int index = 1;

    auto cast_id = (VersionButtonID) m_mcVersion_buttons.id(btn);
    if (cast_id != m_version_id) {
        m_version_id = cast_id;
    } else {
        return;
    }

    m_filter->versions.clear();

    switch(cast_id){
    case(VersionButtonID::Strict):
        m_filter->versions.push_front(mcVersion());
        break;
    case(VersionButtonID::Major): {
        auto versionSplit = mcVersionStr().split(".");

        auto major_version = QString("%1.%2").arg(versionSplit[0], versionSplit[1]);
        QString version_str = major_version;

        while (m_version_list->hasVersion(version_str)) {
            m_filter->versions.emplace_back(version_str);
            version_str = QString("%1.%2").arg(major_version, QString::number(index++));
        }

        break;
    }
    case(VersionButtonID::All):
        // Empty list to avoid enumerating all versions :P
        break;
    case(VersionButtonID::Between):
        // TODO
        break;
    }

    if(changed())
        emit filterChanged();
    else
        emit filterUnchanged();
}

void ModFilterWidget::onLoaderFilterChanged(QAbstractButton* btn)
{
    auto id = m_loader_buttons.id(btn);

    if (m_loader_buttons.exclusive()) {
        m_filter->mod_loaders &= 0;
        switch(id) {
            case(LoaderButtonID::Fabric):
                m_filter->mod_loaders |= ModAPI::Fabric;
                break;
            case(LoaderButtonID::Forge):
                m_filter->mod_loaders |= ModAPI::Forge;
                break;
            case(LoaderButtonID::Quilt):
                m_filter->mod_loaders |= ModAPI::Quilt;
                break;
            default:
                qWarning() << "Invalid ID in onLoaderFilterChanged";
        }
    } else {
        // XOR to toggle.
        switch(id) {
            case(LoaderButtonID::Fabric):
                m_filter->mod_loaders ^= ModAPI::Fabric;
                break;
            case(LoaderButtonID::Forge):
                m_filter->mod_loaders ^= ModAPI::Forge;
                break;
            case(LoaderButtonID::Quilt):
                m_filter->mod_loaders ^= ModAPI::Quilt;
                break;
            default:
                qWarning() << "Invalid ID in onLoaderFilterChanged";
        }
    }

    if (changed())
        emit filterChanged();
    else
        emit filterUnchanged();

}

ModFilterWidget::~ModFilterWidget()
{
    delete ui;
}

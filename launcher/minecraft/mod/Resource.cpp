#include "Resource.h"

#include "FileSystem.h"

Resource::Resource(QObject* parent) : QObject(parent) {}

Resource::Resource(QFileInfo file_info) : QObject()
{
    setFile(file_info);
}

void Resource::setFile(QFileInfo file_info)
{
    m_file_info = file_info;
    parseFile();
}

void Resource::parseFile()
{
    QString file_name{ m_file_info.fileName() };

    m_type = ResourceType::UNKNOWN;

    m_internal_id = file_name;

    if (m_file_info.isDir()) {
        m_type = ResourceType::FOLDER;
        m_name = file_name;
    } else if (m_file_info.isFile()) {
        if (file_name.endsWith(".disabled"))
            file_name.chop(9);

        if (file_name.endsWith(".zip") || file_name.endsWith(".jar")) {
            m_type = ResourceType::ZIPFILE;
            file_name.chop(4);
        } else if (file_name.endsWith(".litemod")) {
            m_type = ResourceType::LITEMOD;
            file_name.chop(8);
        } else {
            m_type = ResourceType::SINGLEFILE;
        }

        m_name = file_name;
    }

    m_changed_date_time = m_file_info.lastModified();
}

bool Resource::destroy()
{
    m_type = ResourceType::UNKNOWN;
    return FS::deletePath(m_file_info.filePath());
}
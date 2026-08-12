/**
 * @file    PackageTableModel.cpp
 * @brief   Implementation of the QAbstractTableModel adapter for domain Package objects.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 * 
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Added support for name field in metadata
 */

#include "PackageTableModel.h"

#include <QColor>
#include <QString>

namespace wms::gui {

    namespace
    {
        QString categoryLabel(wms::domain::Category category)
        {
            switch (category)
            {
            case wms::domain::Category::Standard:   return QStringLiteral("Standard");
            case wms::domain::Category::Fragile:    return QStringLiteral("Fragile");
            case wms::domain::Category::Perishable: return QStringLiteral("Perishable");
            case wms::domain::Category::Hazmat:     return QStringLiteral("Hazmat");
            case wms::domain::Category::Oversized:  return QStringLiteral("Oversized");
            case wms::domain::Category::Liquid:     return QStringLiteral("Liquid");
            }
            return QStringLiteral("Unknown");
        }

        QString formatDate(const wms::domain::Date& date)
        {
            return QStringLiteral("%1-%2-%3")
                .arg(static_cast<int>(date.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.day()), 2, 10, QChar('0'));
        }
    }

    PackageTableModel::PackageTableModel(QObject* parent)
        : QAbstractTableModel(parent)
    {}

    int PackageTableModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return static_cast<int>(m_packages.size());
    }

    int PackageTableModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return 9;
    }

    QVariant PackageTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        const int row = index.row();
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};

        const wms::domain::Package& pkg = m_packages[static_cast<std::size_t>(row)];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case 0: return QString::fromStdString(pkg.id());
            case 1: return QString::fromStdString(pkg.metadata().name);
            case 2: return QString::fromStdString(pkg.metadata().description);
            case 3: return categoryLabel(pkg.metadata().category);
            case 4: return pkg.metadata().weight;
            case 5: return QString::fromStdString(pkg.location().zone);
            case 6: return QString::fromUtf8(
                pkg.currentState().getStateLabel().data(),
                static_cast<int>(pkg.currentState().getStateLabel().size()));
            case 7: return formatDate(pkg.logistics().importDate);
            case 8: return formatDate(pkg.logistics().expectedExportDate);
            default: return {};
            }
        }

        if (role == Qt::ForegroundRole && index.column() == 6)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::Overdue:
                return QColor(235, 87, 87);
            case wms::domain::PackageStateId::Missing:
                return QColor(229, 62, 62);
            case wms::domain::PackageStateId::Dispatched:
                return QColor(47, 128, 237);
            case wms::domain::PackageStateId::InStorage:
                return QColor(39, 174, 96);
            default:
                break;
            }
        }

        return {};
    }

    QVariant PackageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};

        switch (section)
        {
        case 0: return QStringLiteral("ID");
        case 1: return QStringLiteral("Name");
        case 2: return QStringLiteral("Description");
        case 3: return QStringLiteral("Category");
        case 4: return QStringLiteral("Weight (kg)");
        case 5: return QStringLiteral("Zone");
        case 6: return QStringLiteral("Status");
        case 7: return QStringLiteral("Import Date");
        case 8: return QStringLiteral("Export Date");
        default: return {};
        }
    }

    void PackageTableModel::refresh(std::vector<wms::domain::Package> newPackages)
    {
        beginResetModel();
        m_packages = std::move(newPackages);
        endResetModel();
    }

    QString PackageTableModel::packageIdAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};
        return QString::fromStdString(m_packages[static_cast<std::size_t>(row)].id());
    }

    const wms::domain::Package* PackageTableModel::packageAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return nullptr;
        return &m_packages[static_cast<std::size_t>(row)];
    }

    PackageSmallTableModel::PackageSmallTableModel(QObject* parent)
        : PackageTableModel(parent)
    {}

    int PackageSmallTableModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return 4;
    }

    QVariant PackageSmallTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        const int row = index.row();
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};

        const wms::domain::Package& pkg = m_packages[static_cast<std::size_t>(row)];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case 0: return QString::fromStdString(pkg.metadata().name);
            case 1: return categoryLabel(pkg.metadata().category);
            case 2: return QString::fromStdString(pkg.location().zone);
            case 3: return QString::fromUtf8(
                pkg.currentState().getStateLabel().data(),
                static_cast<int>(pkg.currentState().getStateLabel().size()));
            default: return {};
            }
        }

        if (role == Qt::ForegroundRole && index.column() == 3)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::Overdue:
                return QColor(235, 87, 87);
            case wms::domain::PackageStateId::Missing:
                return QColor(229, 62, 62);
            case wms::domain::PackageStateId::Dispatched:
                return QColor(47, 128, 237);
            case wms::domain::PackageStateId::InStorage:
                return QColor(39, 174, 96);
            default:
                break;
            }
        }

        return {};
    }

    QVariant PackageSmallTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};

        switch (section)
        {
        case 0: return QStringLiteral("Name");
        case 1: return QStringLiteral("Category");
        case 2: return QStringLiteral("Zone");
        case 3: return QStringLiteral("Status");
        default: return {};
        }
    }

    PackageCompactTableModel::PackageCompactTableModel(QObject* parent)
        : PackageTableModel(parent)
    {}

    int PackageCompactTableModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return 7;
    }

    QVariant PackageCompactTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        const int row = index.row();
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};

        const wms::domain::Package& pkg = m_packages[static_cast<std::size_t>(row)];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case 0: return QString::fromStdString(pkg.id());
            case 1: return QString::fromStdString(pkg.metadata().name);
            case 2: return categoryLabel(pkg.metadata().category);
            case 3: return QString::fromStdString(pkg.location().zone);
            case 4: return QString::fromUtf8(
                pkg.currentState().getStateLabel().data(),
                static_cast<int>(pkg.currentState().getStateLabel().size()));
            case 5: return formatDate(pkg.logistics().importDate);
            case 6: return formatDate(pkg.logistics().expectedExportDate);
            default: return {};
            }
        }

        if (role == Qt::ForegroundRole && index.column() == 4)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::Overdue:
                return QColor(235, 87, 87);
            case wms::domain::PackageStateId::Missing:
                return QColor(229, 62, 62);
            case wms::domain::PackageStateId::Dispatched:
                return QColor(47, 128, 237);
            case wms::domain::PackageStateId::InStorage:
                return QColor(39, 174, 96);
            default:
                break;
            }
        }

        return {};
    }

    QVariant PackageCompactTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};

        switch (section)
        {
        case 0: return QStringLiteral("ID");
        case 1: return QStringLiteral("Name");
        case 2: return QStringLiteral("Category");
        case 3: return QStringLiteral("Zone");
        case 4: return QStringLiteral("Status");
        case 5: return QStringLiteral("Import Date");
        case 6: return QStringLiteral("Export Date");
        default: return {};
        }
    }

} // namespace wms::gui

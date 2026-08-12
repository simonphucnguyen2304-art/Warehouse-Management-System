/**
 * @file    PackageTableModel.h
 * @brief   Table model mirroring domain Package data into QTableView.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 */

#pragma once

#include <QAbstractTableModel>
#include <vector>

#include "domain/entities/Package.h"

namespace wms::gui {

    class PackageTableModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit PackageTableModel(QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        void refresh(std::vector<wms::domain::Package> newPackages);
        QString packageIdAt(int row) const;
        const wms::domain::Package* packageAt(int row) const;

    protected:
        std::vector<wms::domain::Package> m_packages;
    };

    class PackageSmallTableModel : public PackageTableModel
    {
        Q_OBJECT

    public:
        explicit PackageSmallTableModel(QObject* parent = nullptr);

        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    };

    /**
     * @brief  7-column variant of PackageTableModel: ID, Name, Category, Zone,
     *         Status, Import Date, Export Date. Drops Description and Weight,
     *         which aren't essential for at-a-glance viewing, to leave more
     *         room for the other columns in narrower table areas (State
     *         Operations, Reports) that were getting cramped with all 9.
     */
    class PackageCompactTableModel : public PackageTableModel
    {
        Q_OBJECT

    public:
        explicit PackageCompactTableModel(QObject* parent = nullptr);

        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    };

} // namespace wms::gui
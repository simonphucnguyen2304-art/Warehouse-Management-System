/**
 * @file    AddPackageDialog.h
 * @brief   Dialog for collecting full package fields and building a domain Package.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 * * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 * - Added support for name field in metadata
 * * @update
 * @author  Duong Anh Hao
 * @date    2026-07-26
 * @changelog
 * - Wrapped existing form fields into QTabWidget for better UX.
 *
 * @update
 * @author  Do Minh Khang
 * @date    2026-08-08
 * @changelog
 * - Added Status combo box to Metadata tab so staff can create packages
 *   in any initial state (OnRoute, InStorage, etc.).
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QTabWidget> // Added for Tab UI

#include "domain/entities/Package.h"

namespace wms::gui::dialogs {

    class AddPackageDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit AddPackageDialog(QWidget* parent = nullptr);

        /**
         * @brief  Builds and returns a new Package from the current form values.
         *
         * Reads all input widgets, constructs the required value objects
         * (PackageMetadata, Address, LogisticsInfo, StorageLocation) and
         * delegates to Package::create() to produce a fully initialised
         * package with a generated UUID in OnRoute state.
         *
         * @return A new Package instance ready to be handed to WarehouseManager.
         * @throws std::invalid_argument if the export date is before the import date.
         */
        wms::domain::Package packageData() const;

    private:
        QTabWidget* m_tabWidget{ nullptr }; // Added Tab Widget

        QLineEdit* m_nameEdit{ nullptr };
        QLineEdit* m_descriptionEdit{ nullptr };
        QComboBox* m_categoryCombo{ nullptr };
        QComboBox* m_statusCombo{ nullptr };
        QDoubleSpinBox* m_weightSpin{ nullptr };
        QLineEdit* m_zoneEdit{ nullptr };
        QLineEdit* m_aisleEdit{ nullptr };
        QSpinBox* m_shelfSpin{ nullptr };
        QSpinBox* m_slotSpin{ nullptr };
        QLineEdit* m_sourceCityEdit{ nullptr };
        QLineEdit* m_destinationCityEdit{ nullptr };
        QDateEdit* m_importDateEdit{ nullptr };
        QDateEdit* m_exportDateEdit{ nullptr };
        QDialogButtonBox* m_buttonBox{ nullptr };
    };

} // namespace wms::gui::dialogs

/**
 * @file    EditPackageDialog.h
 * @brief   Dialog for editing an existing Package's mutable fields.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 * - Completely rewrote EditPackageDialog declaration
 * - Added pre-populated input fields for all mutable Package fields
 * (metadata, logistics dates, vehicle info, container ID, location)
 * * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 * - Added support for name field in metadata
 *
 * @update
 * @author  Duong Anh Hao
 * @date    2026-07-26
 * @changelog
 * - Integrated QTabWidget to organize input fields into separate tabs
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QTabWidget>

#include "domain/entities/Package.h"

namespace wms::gui::dialogs {

    class EditPackageDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit EditPackageDialog(const wms::domain::Package& package, QWidget* parent = nullptr);
        wms::domain::Package updatedPackage() const;

    private:
        wms::domain::Package m_original;

        QTabWidget* m_tabWidget{ nullptr };

        QLineEdit* m_nameEdit{ nullptr };
        QLineEdit* m_descriptionEdit{ nullptr };
        QComboBox* m_categoryCombo{ nullptr };
        QDoubleSpinBox* m_weightSpin{ nullptr };
        QLineEdit* m_zoneEdit{ nullptr };
        QLineEdit* m_aisleEdit{ nullptr };
        QSpinBox* m_shelfSpin{ nullptr };
        QSpinBox* m_slotSpin{ nullptr };
        QDateEdit* m_importDateEdit{ nullptr };
        QDateEdit* m_exportDateEdit{ nullptr };
        QDialogButtonBox* m_buttonBox{ nullptr };
    };

} // namespace wms::gui::dialogs

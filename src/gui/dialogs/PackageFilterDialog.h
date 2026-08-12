/**
 * @file    PackageFilterDialog.h
 * @brief   Dialog for building a PackageQueryCriteria object based on user input.
 * @author  Duong Anh Hao
 * @date    2026-07-27
 * 
 * Update
 * @author Duong Anh Hao
 * @date    2026-07-29
 * @changelog
 * - Added more fields (Name, Description Keyword, Zone, Container ID).
 * 
 * @update
 * @author Duong Anh Hao
 * @date   2026-08-02
 * @changelog
 *   - Add pointer members for date filtering UI components 
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QDateEdit>

#include "domain/queries/PackageQueryCriteria.h"

namespace wms::gui::dialogs {

    class PackageFilterDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit PackageFilterDialog(QWidget* parent = nullptr);

        /**
         * @brief Collects all input fields and generates the query criteria.
         * Fields left empty or set to "Any" will remain std::nullopt.
         * * @return wms::domain::PackageQueryCriteria containing the user's active filters.
         */
        wms::domain::PackageQueryCriteria getCriteria() const;

        /**
         * @brief Resets all UI fields to their default (empty/unset) state.
         */
        void resetFilters();

    private:

        QLineEdit* m_nameEdit{ nullptr };
        QLineEdit* m_zoneEdit{ nullptr };
        QLineEdit* m_descriptionKeywordEdit{ nullptr };

        // Classification & Status Fields
        QComboBox* m_stateCombo{ nullptr };
        QComboBox* m_categoryCombo{ nullptr };
        QDoubleSpinBox* m_minWeightSpin{ nullptr };
        QDoubleSpinBox* m_maxWeightSpin{ nullptr };

        // Quick Toggle Fields (Booleans)
        QCheckBox* m_overdueCheck{ nullptr };
        QCheckBox* m_missingCheck{ nullptr };
        QCheckBox* m_importedTodayCheck{ nullptr };
        QCheckBox* m_exportDueTodayCheck{ nullptr };

        // Date 
        QCheckBox* m_filterImportCheck{ nullptr };
        QDateEdit* m_importDateEdit{ nullptr };

        QCheckBox* m_filterExportCheck{ nullptr };
        QDateEdit* m_exportDateEdit{ nullptr };

        QDialogButtonBox* m_buttonBox{ nullptr };

        // Helper setup functions
        void setupTextFiltersGroup(QVBoxLayout* mainLayout);
        void setupClassificationGroup(QVBoxLayout* mainLayout);
        void setupQuickTogglesGroup(QVBoxLayout* mainLayout);
    };

} // namespace wms::gui::dialogs

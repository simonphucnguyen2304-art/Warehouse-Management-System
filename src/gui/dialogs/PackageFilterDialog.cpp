/**
 * @file    PackageFilterDialog.cpp
 * @brief   Implementation of the filter dialog mapping UI to PackageQueryCriteria.
 * @author  Duong Anh Hao
 * @date    2026-07-27
 * 
 * @update
 * @author Duong Anh Hao
 * @date    2026-07-27
 * @changelog
 * - Added more fields (Name, Keyword, Zone, and Container ID).
 * 
 * @update
 * @author Duong Anh Hao
 * @date   2026-08-02
 * @changelog
 *   - Update setupQuickTogglesGroup() to include QCheckBox and QDateEdit 
 *     for custom import/export date filtering.
 *   - Update getCriteria() to parse QDate to wms::domain::Date and map to criteria.
 *   - Implement backward compatibility logic: auto-enable importedToday/exportDueToday 
 *     if the selected date matches the current date.
 */

#include "PackageFilterDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDateEdit>
#include <QToolButton>
#include <QCalendarWidget>

 // Enums from domain
#include "domain/entities/Category.h"
#include "domain/states/PackageStateId.h"

namespace wms::gui::dialogs {

    namespace
    {
        /**
         * @brief  Gives a QDateEdit's calendar-popup prev/next month buttons
         *         a visible custom icon - see AddPackageDialog.cpp for the
         *         full rationale.
         */
        void polishCalendarPopup(QDateEdit* dateEdit)
        {
            auto* cal = dateEdit->calendarWidget();
            if (!cal)
                return;

            if (auto* prevBtn = cal->findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth")))
                prevBtn->setIcon(QIcon(QStringLiteral(":/icons/chevron_left.svg")));
            if (auto* nextBtn = cal->findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth")))
                nextBtn->setIcon(QIcon(QStringLiteral(":/icons/chevron_right.svg")));
        }
    }

    PackageFilterDialog::PackageFilterDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Filter Packages");
        setModal(true);
        setMinimumSize(400, 480);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(16);

        // Build the UI groups
        setupTextFiltersGroup(mainLayout); 
        setupClassificationGroup(mainLayout);
        setupQuickTogglesGroup(mainLayout);

        // Buttons: Apply, Reset, Cancel
        m_buttonBox = new QDialogButtonBox(this);
        auto* btnApply = m_buttonBox->addButton("Apply Filter", QDialogButtonBox::AcceptRole);
        auto* btnReset = m_buttonBox->addButton("Reset", QDialogButtonBox::ResetRole);
        auto* btnCancel = m_buttonBox->addButton(QDialogButtonBox::Cancel);

        mainLayout->addWidget(m_buttonBox);

        // Connect signals
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(btnReset, &QPushButton::clicked, this, &PackageFilterDialog::resetFilters);

        setMinimumWidth(480);
        adjustSize();
        resize(qMax(width(), 500), qMax(height(), 680));
    }

    

    void PackageFilterDialog::setupClassificationGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Classification && Constraints", this);
        auto* layout = new QFormLayout(group);

        // State Combo
        m_stateCombo = new QComboBox(this);
        m_stateCombo->addItem("Any State", -1); // Sentinel value for std::nullopt
        m_stateCombo->addItem("On Route", static_cast<int>(wms::domain::PackageStateId::OnRoute));
        m_stateCombo->addItem("In Storage", static_cast<int>(wms::domain::PackageStateId::InStorage));
        m_stateCombo->addItem("Dispatched", static_cast<int>(wms::domain::PackageStateId::Dispatched));
        m_stateCombo->addItem("Missing", static_cast<int>(wms::domain::PackageStateId::Missing));
        m_stateCombo->addItem("Overdue", static_cast<int>(wms::domain::PackageStateId::Overdue));

        // Category Combo
        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("Any Category", -1); // Sentinel value for std::nullopt
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile", static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat", static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid", static_cast<int>(wms::domain::Category::Liquid));

        // Weight SpinBoxes (-1.0 means no limit)
        m_minWeightSpin = new QDoubleSpinBox(this);
        m_minWeightSpin->setRange(-1.0, 10000.0);
        m_minWeightSpin->setValue(-1.0);
        m_minWeightSpin->setSpecialValueText("No Minimum"); // Displays this when value is at min (-1.0)

        m_maxWeightSpin = new QDoubleSpinBox(this);
        m_maxWeightSpin->setRange(-1.0, 10000.0);
        m_maxWeightSpin->setValue(-1.0);
        m_maxWeightSpin->setSpecialValueText("No Maximum");

        layout->addRow("State:", m_stateCombo);
        layout->addRow("Category:", m_categoryCombo);
        layout->addRow("Min Weight (kg):", m_minWeightSpin);
        layout->addRow("Max Weight (kg):", m_maxWeightSpin);

        mainLayout->addWidget(group);
    }

    void PackageFilterDialog::setupQuickTogglesGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Date && Status Filters", this);
        auto* layout = new QVBoxLayout(group);

        // 1. Import Filter
        auto* importLayout = new QHBoxLayout();
        m_filterImportCheck = new QCheckBox("Filter by Import Date:", this);
        m_importDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_importDateEdit->setCalendarPopup(true);
        m_importDateEdit->setEnabled(false);
        polishCalendarPopup(m_importDateEdit);

        connect(m_filterImportCheck, &QCheckBox::toggled, m_importDateEdit, &QWidget::setEnabled);

        importLayout->addWidget(m_filterImportCheck);
        importLayout->addWidget(m_importDateEdit);
        importLayout->addStretch();
        layout->addLayout(importLayout);

        // 2. Export Filter
        auto* exportLayout = new QHBoxLayout();
        m_filterExportCheck = new QCheckBox("Filter by Export Date:", this);
        m_exportDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_exportDateEdit->setCalendarPopup(true);
        m_exportDateEdit->setEnabled(false);
        polishCalendarPopup(m_exportDateEdit);

        connect(m_filterExportCheck, &QCheckBox::toggled, m_exportDateEdit, &QWidget::setEnabled);

        exportLayout->addWidget(m_filterExportCheck);
        exportLayout->addWidget(m_exportDateEdit);
        exportLayout->addStretch();
        layout->addLayout(exportLayout);

   
        mainLayout->addWidget(group);
    }
    void PackageFilterDialog::setupTextFiltersGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Text Search Filters", this);
        auto* layout = new QFormLayout(group);

        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText("e.g: Electronics, Macbook...");
        layout->addRow("Name:", m_nameEdit);

        m_descriptionKeywordEdit = new QLineEdit(this);
        m_descriptionKeywordEdit->setPlaceholderText("e.g: fragile, urgent...");
        layout->addRow("Keyword:", m_descriptionKeywordEdit);

        m_zoneEdit = new QLineEdit(this);
        m_zoneEdit->setPlaceholderText("e.g: A, B, Cold Storage...");
        layout->addRow("Zone:", m_zoneEdit);

        mainLayout->addWidget(group);
    }

    void PackageFilterDialog::resetFilters()
    {
        if (m_nameEdit) m_nameEdit->clear();
        if (m_descriptionKeywordEdit) m_descriptionKeywordEdit->clear();
        if (m_zoneEdit) m_zoneEdit->clear();

        m_stateCombo->setCurrentIndex(0);
        m_categoryCombo->setCurrentIndex(0);

        m_minWeightSpin->setValue(-1.0);
        m_maxWeightSpin->setValue(-1.0);

        if (m_filterImportCheck) m_filterImportCheck->setChecked(false);
        if (m_filterExportCheck) m_filterExportCheck->setChecked(false);
        if (m_importDateEdit) m_importDateEdit->setDate(QDate::currentDate());
        if (m_exportDateEdit) m_exportDateEdit->setDate(QDate::currentDate());
    }

    wms::domain::PackageQueryCriteria PackageFilterDialog::getCriteria() const
    {
        wms::domain::PackageQueryCriteria criteria;
        //1.  Get Text data (name, Keyword, zone, containerID
        if (m_nameEdit && !m_nameEdit->text().trimmed().isEmpty())
            criteria.name = m_nameEdit->text().trimmed().toStdString();

        if (m_descriptionKeywordEdit && !m_descriptionKeywordEdit->text().trimmed().isEmpty())
            criteria.descriptionKeyword = m_descriptionKeywordEdit->text().trimmed().toStdString();

        if (m_zoneEdit && !m_zoneEdit->text().trimmed().isEmpty())
            criteria.zone = m_zoneEdit->text().trimmed().toStdString();

        // 2. Combo Boxes (check for sentinel value -1)
        if (m_stateCombo->currentData().toInt() != -1)
            criteria.state = static_cast<wms::domain::PackageStateId>(m_stateCombo->currentData().toInt());

        if (m_categoryCombo->currentData().toInt() != -1)
            criteria.category = static_cast<wms::domain::Category>(m_categoryCombo->currentData().toInt());

        // 3. Weight Limits (check for sentinel value -1.0)
        if (m_minWeightSpin->value() >= 0.0)
            criteria.minWeight = m_minWeightSpin->value();

        if (m_maxWeightSpin->value() >= 0.0)
            criteria.maxWeight = m_maxWeightSpin->value();

        if (m_overdueCheck) criteria.overdueOnly = m_overdueCheck->isChecked();
        if (m_missingCheck) criteria.lateOnly = m_missingCheck->isChecked();

        // 4. Date
        auto convertDate = [](const QDate& qdate) {
            return wms::domain::Date{
                std::chrono::year{ qdate.year() },
                std::chrono::month{ static_cast<unsigned>(qdate.month()) },
                std::chrono::day{ static_cast<unsigned>(qdate.day()) }
            };
            };

        if (m_filterImportCheck && m_filterImportCheck->isChecked()) {
            QDate selectedDate = m_importDateEdit->date();

            criteria.importDate.emplace(convertDate(selectedDate));

            if (selectedDate == QDate::currentDate()) {
                criteria.importedToday = true; 
            }
        }

        if (m_filterExportCheck && m_filterExportCheck->isChecked()) {
            QDate selectedDate = m_exportDateEdit->date();

            criteria.exportDate.emplace(convertDate(selectedDate));

            if (selectedDate == QDate::currentDate()) {
                criteria.exportDueToday = true; 
            }
        }

        return criteria;
    }

} // namespace wms::gui::dialogs

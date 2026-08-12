/**
 * @file    AddPackageDialog.cpp
 * @brief   Implementation of AddPackageDialog mapping UI fields to domain Package attributes.
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
 * - Grouped existing input fields into QTabWidget categories (Metadata, Location, Address, Logistics).
 */

#include "AddPackageDialog.h"
#include "domain/entities/Category.h"
#include "domain/states/PackageStateId.h"
#include <chrono>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QToolButton>
#include <QCalendarWidget>

namespace wms::gui::dialogs {

    namespace
    {
        wms::domain::Category categoryFromIndex(int data)
        {
            return static_cast<wms::domain::Category>(data);
        }

        wms::domain::Date dateFromQDate(const QDate& date)
        {
            return wms::domain::Date{
                std::chrono::year{ date.year() },
                std::chrono::month{ static_cast<unsigned>(date.month()) },
                std::chrono::day{ static_cast<unsigned>(date.day()) }
            };
        }

        /**
         * @brief  Gives a QDateEdit's calendar-popup prev/next month buttons
         *         a visible custom icon - the native arrow glyph wasn't
         *         picking up our QSS color override (icons aren't colored
         *         by the "color" property).
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

    AddPackageDialog::AddPackageDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Add New Package");
        setModal(true);
        setMinimumSize(480, 420);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(16);

        // Initialize Tab Widget
        m_tabWidget = new QTabWidget(this);

        //1. Metadata Tab
        auto* metadataTab = new QWidget();
        auto* metadataLayout = new QFormLayout(metadataTab);
        metadataLayout->setContentsMargins(16, 16, 16, 16);
        metadataLayout->setSpacing(10);

        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText("Package Name");

        m_descriptionEdit = new QLineEdit(this);
        m_descriptionEdit->setPlaceholderText("Package Description");

        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile", static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat", static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid", static_cast<int>(wms::domain::Category::Liquid));

        m_statusCombo = new QComboBox(this);
        m_statusCombo->addItem("On Route",   static_cast<int>(wms::domain::PackageStateId::OnRoute));
        m_statusCombo->addItem("In Storage", static_cast<int>(wms::domain::PackageStateId::InStorage));
        m_statusCombo->addItem("Dispatched", static_cast<int>(wms::domain::PackageStateId::Dispatched));
        m_statusCombo->addItem("Missing",    static_cast<int>(wms::domain::PackageStateId::Missing));
        m_statusCombo->addItem("Overdue",    static_cast<int>(wms::domain::PackageStateId::Overdue));

        m_weightSpin = new QDoubleSpinBox(this);
        m_weightSpin->setRange(0.1, 10000.0);
        m_weightSpin->setValue(10.0);
        m_weightSpin->setSuffix(" kg");

        metadataLayout->addRow("Name:", m_nameEdit);
        metadataLayout->addRow("Description:", m_descriptionEdit);
        metadataLayout->addRow("Category:", m_categoryCombo);
        metadataLayout->addRow("Status:", m_statusCombo);
        metadataLayout->addRow("Weight:", m_weightSpin);
        m_tabWidget->addTab(metadataTab, "Metadata");

        //2. Location Tab 
        auto* locationTab = new QWidget();
        auto* locationLayout = new QFormLayout(locationTab);
        locationLayout->setContentsMargins(16, 16, 16, 16);
        locationLayout->setSpacing(10);

        m_zoneEdit = new QLineEdit("ZoneA", this);
        m_aisleEdit = new QLineEdit("Aisle1", this);
        m_shelfSpin = new QSpinBox(this);
        m_shelfSpin->setRange(1, 99);
        m_slotSpin = new QSpinBox(this);
        m_slotSpin->setRange(1, 99);

        locationLayout->addRow("Zone:", m_zoneEdit);
        locationLayout->addRow("Aisle:", m_aisleEdit);
        locationLayout->addRow("Shelf:", m_shelfSpin);
        locationLayout->addRow("Slot:", m_slotSpin);
        m_tabWidget->addTab(locationTab, "Location");

        //3. Address Tab
        auto* addressTab = new QWidget();
        auto* addressLayout = new QFormLayout(addressTab);
        addressLayout->setContentsMargins(16, 16, 16, 16);
        addressLayout->setSpacing(10);

        m_sourceCityEdit = new QLineEdit("Hanoi", this);
        m_destinationCityEdit = new QLineEdit("Ho Chi Minh City", this);

        addressLayout->addRow("Source City:", m_sourceCityEdit);
        addressLayout->addRow("Destination City:", m_destinationCityEdit);
        m_tabWidget->addTab(addressTab, "Address");

        //4. Logistics Tab
        auto* logisticsTab = new QWidget();
        auto* logisticsLayout = new QFormLayout(logisticsTab);
        logisticsLayout->setContentsMargins(16, 16, 16, 16);
        logisticsLayout->setSpacing(10);

        m_importDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_importDateEdit->setCalendarPopup(true);
        polishCalendarPopup(m_importDateEdit);
        m_exportDateEdit = new QDateEdit(QDate::currentDate().addDays(5), this);
        m_exportDateEdit->setCalendarPopup(true);
        polishCalendarPopup(m_exportDateEdit);

        // GitHub #17: an invalid import/export date order corrupted the
        // database and crashed the app on tab switch. Constrain each field
        // against the other's current value so an invalid combination can
        // never be picked from the calendar/spinner in the first place.
        m_exportDateEdit->setMinimumDate(m_importDateEdit->date());
        m_importDateEdit->setMaximumDate(m_exportDateEdit->date());
        connect(m_importDateEdit, &QDateEdit::dateChanged, m_exportDateEdit, &QDateEdit::setMinimumDate);
        connect(m_exportDateEdit, &QDateEdit::dateChanged, m_importDateEdit, &QDateEdit::setMaximumDate);

        logisticsLayout->addRow("Import Date:", m_importDateEdit);
        logisticsLayout->addRow("Export Date:", m_exportDateEdit);
        m_tabWidget->addTab(logisticsTab, "Logistics");

        // Add TabWidget to main layout
        mainLayout->addWidget(m_tabWidget);

        //Button Box
        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(m_buttonBox);

        connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
            // Safety net in case the live constraint above is ever bypassed.
            if (m_importDateEdit->date() > m_exportDateEdit->date())
            {
                QMessageBox::warning(this, "Invalid Dates",
                    "Import date must be on or before the export date.\n\n"
                    "Please correct the Logistics tab before saving.");
                m_tabWidget->setCurrentIndex(3); // Logistics tab
                return;
            }
            accept();
        });
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }


    wms::domain::Package AddPackageDialog::packageData() const
    {
        const QString description = m_descriptionEdit->text().trimmed().isEmpty()
            ? QStringLiteral("New Package")
            : m_descriptionEdit->text().trimmed();

        wms::domain::PackageMetadata metadata{
            m_nameEdit->text().toStdString(),
            categoryFromIndex(m_categoryCombo->currentData().toInt()),
            m_weightSpin->value(),
            wms::domain::Dimension(10.0, 10.0, 10.0),
            150.0,
            description.toStdString()
        };

        wms::domain::Address source{
            "Origin Address",
            m_sourceCityEdit->text().toStdString(),
            "Vietnam",
            "100000"
        };

        wms::domain::Address destination{
            "Destination Address",
            m_destinationCityEdit->text().toStdString(),
            "Vietnam",
            "700000"
        };

        wms::domain::LogisticsInfo logistics{
            dateFromQDate(m_importDateEdit->date()),
            dateFromQDate(m_exportDateEdit->date()),
            "Truck-01",
            "Truck-02",
            "CONT-100"
        };

        wms::domain::StorageLocation location{
            m_zoneEdit->text().toStdString(),
            m_aisleEdit->text().toStdString(),
            m_shelfSpin->value(),
            m_slotSpin->value()
        };

        const auto selectedStateId = static_cast<wms::domain::PackageStateId>(
            m_statusCombo->currentData().toInt());

        // Package::create() always starts OnRoute. For any other initial state
        // we use createWithState() which generates a fresh UUID but allows
        // specifying the initial lifecycle state (e.g. InStorage on arrival).
        if (selectedStateId == wms::domain::PackageStateId::OnRoute)
            return wms::domain::Package::create(metadata, source, destination, logistics, location);

        return wms::domain::Package::createWithState(
            metadata, source, destination, logistics, location, selectedStateId);
    }

} // namespace wms::gui::dialogs

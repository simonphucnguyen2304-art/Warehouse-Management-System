/**
 * @file    MainWindow.cpp
 * @brief   Implementation of the main application window for the Warehouse Management System.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Refactored to multi-page layout with sidebar navigation
 *   - Added Dashboard, Inventory, State Operations and Reports pages
 *   - Integrated periodic overdue-check timer
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-12
 * @changelog
 *   - Updated text color for various strings for contrast
 *   - Replaced Dashboard statistics with pie chart
 *   - Fixed unsaved changes prompt appearing when no changes were made
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Commented out Save and Load buttons, and dirty workspace check due
 *     to redundancy with SQLite database
 *
 * @update
 * @author  Do Minh Khang
 * @date    2026-07-23
 * @changelog
 *   - Replaced WarehouseManager* with WarehouseGateway* throughout (see
 *     MainWindow.h for the full rationale). persistAndRefresh() is gone -
 *     renamed to onPackagesChanged() and now triggered by
 *     WarehouseGateway::packagesChanged() via a single connect() in the
 *     constructor, instead of being called explicitly after every mutation.
 *   - refreshTable() removed: it had no callers anywhere in this file (a
 *     leftover from before the multi-page redesign) and its entire body
 *     was just the now-removed persistAndRefresh(true).
 *
 * @note onSave()/onLoad() are still present and still compile correctly
 *       against the gateway, but are effectively unreachable following the
 *       2026-07-26 change above (closeEvent()'s dirty check is hardcoded
 *       `if (false)`, and no button is connected to onLoad() anymore).
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-25
 * @changelog
 *   - Added Export/Import buttons (CSV & JSON) to the Inventory page toolbar.
 *   - Implemented onExportCsv / onImportCsv / onExportJson / onImportJson
 *     slots: each opens a QFileDialog, calls the appropriate WarehouseGateway
 *     method, and shows a QMessageBox for success or failure.
 *   - Import slots rely on the existing packagesChanged() → onPackagesChanged()
 *     Observer chain for view refresh - no extra wiring needed.
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Updated dashboard UI by making the progress bar vertical and moving
 *     it to the left side
 *   - Fixed pie slices flickering and tooltips disappearing early
 *   - Implemented To-Do List
 * 
 * @update 
 * @author  Duong Anh Hao
 * @date    2026-07-29
 * @changelog
 * - Relocated the "Filter Packages" button to a dedicated top header layout on the Inventory page for better UX.
 * - Wired the filter button to capture PackageQueryCriteria and execute queryPackages() via the Gateway.
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 * @changelog
 * - Added click-to-sort on all table column headers across Inventory, Operations,
 *   Dashboard and Reports pages (QSortFilterProxyModel stacked on each table).
 * - Implemented Excel-style per-column dropdown filter (▼ button on every header):
 *   clicking the button shows a popup with checkboxes for each unique value in that
 *   column; active filters are highlighted orange on the header.
 * - Extracted reusable components: MultiColumnFilterProxy (per-column value filter
 *   proxy) and FilterableHeaderView (custom QHeaderView that paints and handles the
 *   ▼ filter button).
 * - Added showColumnFilterPopup() helper method to avoid duplicating popup code
 *   across all table pages.
 * - Fixed proxy-chain index mapping in selectedPackageId(),
 *   selectedOpsPackageId(), and updateOpsButtonStates() so that Edit / Remove /
 *   state-transition operations always target the correct source-model row after
 *   sorting or filtering.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Implement onCheckLate() and the button connection
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-08-08
 * @changelog
 *   - onReceivePackage() and onOpsReceivePackage(): when the package's scheduled
 *     importDate is in the future, a QMessageBox::question warns the user and
 *     lets them cancel the early receipt instead of hard-blocking the action.
 *   - Removed the special "not due for arrival" catch branch in onOpsReceivePackage()
 *     because WarehouseManager::receivePackage() no longer throws that error.
 *   - Added #include <chrono> for date comparison.
 */

#define WAREHOUSE_MAX 1000

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "dialogs/AddPackageDialog.h"
#include "dialogs/EditPackageDialog.h"
#include "dialogs/PackageFilterDialog.h"
#include "domain/states/PackageStateId.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QItemSelectionModel>
#include <QPieSeries>
#include <QPieSlice>
#include <QToolTip>
#include <QCursor>
#include <QChartView>
#include <QPieLegendMarker>
#include <QListWidget>
#include <QColor>
#include <QComboBox>
#include <QApplication>
#include <QFile>
#include <QScrollArea>
#include <chrono>

namespace wms::gui {

    namespace
    {
        QString buttonStyle(const char* bg, const char* fg = "white")
        {
            // Hover/pressed shades are derived from the button's own base
            // color rather than hand-picked per button, so every colored
            // action button (Add/Export/Filter/Dispatch/...) gets consistent
            // feedback without touching any of the 14 call sites.
            const QColor base(bg);
            const QString hoverColor = base.darker(112).name();
            const QString pressedColor = base.darker(128).name();

            return QStringLiteral(
                "QPushButton { background-color: %1; color: %2; padding: 8px 16px; "
                "border: 2px solid transparent; border-radius: 6px; font-weight: 600; }"
                "QPushButton:hover { background-color: %3; }"
                "QPushButton:pressed { background-color: %4; }"
                "QPushButton:focus { border: 2px solid #201F1E; }"
                "QPushButton:disabled { background-color: #CBD5E0; color: #718096; }")
                .arg(QString::fromUtf8(bg), QString::fromUtf8(fg), hoverColor, pressedColor);
        }

        /**
         * @brief  Shared stylesheet for every QTableView in the app.
         *         Consolidates 6 previously-duplicated inline blocks (5
         *         identical, 1 with a stray 1px padding difference) into a
         *         single source of truth. Also adds an ::item:hover state -
         *         previously only :selected was styled, so hovering a row
         *         gave no visual feedback at all before clicking it.
         */
        QString tableStyle()
        {
            return QStringLiteral(
                "QTableView { outline: none; background-color: white; color: #2D3748; "
                "  gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
                "QTableView::item { padding: 6px 10px; }"
                "QTableView::item:focus { outline: none; border: none; }"
                "QTableView::item:hover:!selected { background-color: #F5FAFF; }"
                "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none; }"
                "QHeaderView::section { background-color: #F7FAFC; padding: 9px 26px 9px 10px; color: #4A5568;"
                "  font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; border-right: 1px solid #E2E8F0; }"
                "QHeaderView::section:hover { background-color: #EBF4FF; color: #2B6CB0; }"
                "QHeaderView::section:pressed { background-color: #BEE3F8; }"
                "QHeaderView::up-arrow   { image: url(:/icons/sort_asc.svg);  width: 8px; height: 8px; margin-right: 8px; subcontrol-position: right; }"
                "QHeaderView::down-arrow { image: url(:/icons/sort_desc.svg); width: 8px; height: 8px; margin-right: 8px; subcontrol-position: right; }"
            );
        }

        /**
         * @brief  Loads and applies the centralized Fluent-style theme
         *         (resources/theme.qss, embedded via CMake's qt_add_resources)
         *         at the application level.
         *
         *         Applied once, app-wide, rather than per-widget: this gives
         *         every currently-unstyled widget (dialogs, plain buttons,
         *         inputs) a consistent modern default. It does NOT override
         *         any of MainWindow's existing inline setStyleSheet() calls -
         *         a widget-level stylesheet always takes precedence over the
         *         application-level one for that widget, so today's sidebar/
         *         page/table styling is unaffected until it's deliberately
         *         migrated in a later phase.
         */
        void applyCentralTheme()
        {
            QFile themeFile(":/theme.qss");
            if (themeFile.open(QFile::ReadOnly | QFile::Text))
            {
                qApp->setStyleSheet(QString::fromUtf8(themeFile.readAll()));
                themeFile.close();
            }
        }

        /**
         * @brief  QHeaderView that ignores clicks on one designated column,
         *         so it can never be sorted-by. Used for the ID column:
         *         package IDs are auto-generated UUIDs, so alphabetically
         *         sorting by them carries no meaningful order. Swallowing
         *         the click at the header level (rather than just no-op'ing
         *         the model's sort()) also prevents a sort-arrow indicator
         *         from appearing and implying something happened when it didn't.
         */
        class NoSortHeaderView : public QHeaderView
        {
        public:
            NoSortHeaderView(Qt::Orientation orientation, int noSortColumn, QWidget* parent = nullptr)
                : QHeaderView(orientation, parent), m_noSortColumn(noSortColumn)
            {}

        protected:
            void mousePressEvent(QMouseEvent* event) override
            {
                if (logicalIndexAt(event->pos()) == m_noSortColumn)
                    return;
                QHeaderView::mousePressEvent(event);
            }

            void mouseReleaseEvent(QMouseEvent* event) override
            {
                if (logicalIndexAt(event->pos()) == m_noSortColumn)
                    return;
                QHeaderView::mouseReleaseEvent(event);
            }

        private:
            int m_noSortColumn;
        };
    }

    MainWindow::MainWindow(WarehouseGateway* gateway, QWidget* parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow)
        , m_gateway(gateway)
    {
        applyCentralTheme();

        ui->setupUi(this);
        resize(1280, 800);
        setMinimumSize(1024, 700);
        setWindowTitle("Warehouse Management System");

        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        auto* mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        m_sidebarMenu = new QListWidget(this);
        m_sidebarMenu->setObjectName("sidebarMenu");
        m_sidebarMenu->setFixedWidth(240);
        m_sidebarMenu->addItems({
            "Dashboard",
            "Package Inventory",
            "State Operations",
            "Reports"
            });
        // Styling for #sidebarMenu now lives in resources/theme.qss (Phase 2 -
        // Main Window & Primary Shell), rather than as an inline literal here.
        mainLayout->addWidget(m_sidebarMenu);

        m_stackedWidget = new QStackedWidget(this);
        mainLayout->addWidget(m_stackedWidget);

        auto* dashboardPage = new QWidget(this);
        auto* inventoryPage = new QWidget(this);
        auto* operationsPage = new QWidget(this);
        auto* reportsPage = new QWidget(this);

        setupDashboardPage(dashboardPage);
        setupInventoryPage(inventoryPage);
        setupOperationsPage(operationsPage);
        setupReportsPage(reportsPage);

        m_stackedWidget->addWidget(dashboardPage);
        m_stackedWidget->addWidget(inventoryPage);
        m_stackedWidget->addWidget(operationsPage);
        m_stackedWidget->addWidget(reportsPage);

        connect(m_sidebarMenu, &QListWidget::currentRowChanged, this, &MainWindow::onSidebarCurrentRowChanged);

        // The Observer connection: every WarehouseGateway mutation ends by
        // emitting packagesChanged(), and this is the one place that gets
        // wired to react to it - no other call site needs to know a signal
        // exists at all.
        connect(m_gateway, &WarehouseGateway::packagesChanged, this, &MainWindow::onPackagesChanged);

        m_overdueTimer = new QTimer(this);
        m_overdueTimer->setInterval(60 * 60 * 1000);
        connect(m_overdueTimer, &QTimer::timeout, this, &MainWindow::onTimerExec);
        m_overdueTimer->start();

        // Startup overdue scan, through the gateway like any other mutation.
        // Note this does NOT replace the explicit onPackagesChanged() call
        // below: WarehouseGateway::checkOverduePackages() only emits
        // packagesChanged() when it actually finds something (count > 0),
        // so relying on that signal alone would leave every page empty on
        // a normal startup where nothing happens to be overdue yet. The
        // views' initial population is a separate concern from "did this
        // particular action produce a change worth notifying about", so it
        // gets its own direct call here.
        m_gateway->checkOverduePackages();
        onPackagesChanged();
        m_dirty = false;   // an automatic startup scan is never an unsaved
        // user edit, even though onPackagesChanged()
        // above unconditionally sets m_dirty = true -
        // this matches the original behaviour, where
        // persistAndRefresh(true) always reset m_dirty
        // to false here regardless of overdueCount.
        m_sidebarMenu->setCurrentRow(0);
    }

    MainWindow::~MainWindow()
    {
        delete ui;
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (false) // if (m_dirty)
        {
            const auto reply = QMessageBox::question(
                this,
                "Unsaved Changes",
                "Save changes before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

            if (reply == QMessageBox::Cancel)
            {
                event->ignore();
                return;
            }
            if (reply == QMessageBox::Save)
                onSave();
        }
        event->accept();
    }

    // Page 0: Dashboard Page Setup
    void MainWindow::setupDashboardPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("Warehouse Performance Dashboard", page);
        title->setProperty("class", "pageTitle");
        layout->addWidget(title);

        auto* dashboardTopLayout = new QHBoxLayout();
        dashboardTopLayout->setSpacing(12);

        auto* capacityLayout = new QVBoxLayout();
        auto* capacityFrame = new QFrame(page);
        capacityFrame->setStyleSheet(
            "background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 12px;");
        auto* capLayout = new QVBoxLayout(capacityFrame);
        capLayout->setAlignment(Qt::AlignCenter);

        m_dbCapacityLabel = new QLabel(QString("Occupancy<br>0 / %1").arg(WAREHOUSE_MAX), capacityFrame);
        m_dbCapacityLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2D3748; text-align: center; border: none;");
        m_dbCapacityLabel->setAlignment(Qt::AlignHCenter);
        capLayout->addWidget(m_dbCapacityLabel);

        m_dbCapacityProgress = new QProgressBar(capacityFrame);
        m_dbCapacityProgress->setOrientation(Qt::Vertical);
        m_dbCapacityProgress->setRange(0, WAREHOUSE_MAX);
        m_dbCapacityProgress->setValue(0);
        m_dbCapacityProgress->setTextVisible(true);
        m_dbCapacityProgress->setStyleSheet(
            "QProgressBar { background-color: #EDF2F7; color: #2D3748; border-radius: 6px; text-align: center; width: 22px; font-weight: bold; border: none; }"
            "QProgressBar::chunk { background-color: #48BB78; color: #2D3748; border-radius: 6px; }");

        m_dbCapacityProgress->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        capLayout->addWidget(m_dbCapacityProgress, 1, Qt::AlignHCenter);
        capacityLayout->addWidget(capacityFrame);
        dashboardTopLayout->addLayout(capacityLayout);

        auto* pieLayout = new QVBoxLayout();

        auto* series = new QPieSeries();

        m_dbPlaceholderSlice = new QPieSlice("Placeholder", 0);
        m_dbPlaceholderSlice->setProperty("isPlaceholder", true);
        m_dbPlaceholderSlice->setBrush(QBrush(QColor("#E0E0E0")));
        m_dbPlaceholderSlice->setPen(QPen(QColor("#9E9E9E"), 2, Qt::DashLine));
        m_dbPlaceholderSlice->setLabelVisible(false);
        series->append(m_dbPlaceholderSlice);

        m_dbStorageSlice = new QPieSlice(QString("<b>In Storage</b>"), 0);
        series->append(m_dbStorageSlice);

        m_dbOnRouteSlice = new QPieSlice(QString("<b>On Route</b>"), 0);
        series->append(m_dbOnRouteSlice);

        m_dbDispatchedSlice = new QPieSlice(QString("<b>Dispatched</b>"), 0);
        series->append(m_dbDispatchedSlice);

        m_dbOverdueSlice = new QPieSlice(QString("<b>Overdue</b>"), 0);
        series->append(m_dbOverdueSlice);

        m_dbMissingSlice = new QPieSlice(QString("<b>Missing</b>"), 0);
        series->append(m_dbMissingSlice);

        auto* chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(QString("<b>Statistics</b>"));
        chart->setTheme(QChart::ChartThemeLight);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->titleFont().setBold(true);
        chart->legend()->setAlignment(Qt::AlignBottom);

        const QList<QLegendMarker*> markers = chart->legend()->markers(series);
        for (QLegendMarker* marker : markers) {
            QPieLegendMarker* pieMarker = qobject_cast<QPieLegendMarker*>(marker);
            if (pieMarker && pieMarker->slice()) {
                if (pieMarker->slice()->property("isPlaceholder").toBool()) pieMarker->setVisible(false);
            }
        }

        QChartView* chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 15px;");

        QLabel* chartTooltip = new QLabel(chartView);
        chartTooltip->setStyleSheet(
            "QLabel { background-color: #2D3748; color: #FFFFFF; padding: 6px 12px; font-size: 12px; }"
        );
        chartTooltip->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        chartTooltip->hide();

        for (QPieSlice* slice : series->slices()) {
            if (!slice->property("isPlaceholder").toBool())
                slice->setExplodeDistanceFactor(0.06);

            QObject::connect(slice, &QPieSlice::hovered, [slice, chartView, chartTooltip](bool isHovered) {
                if (isHovered && !slice->property("isPlaceholder").toBool() && slice->percentage() != 0) {
                    double currentPct = slice->percentage() * 100;

                    QString info = QString("<b>%1</b><br/>"
                        "Count: %2<br/>"
                        "Percentage: %3%")
                        .arg(slice->label())
                        .arg(slice->value())
                        .arg(QString::number(currentPct, 'f', 1));

                    chartTooltip->setText(info);
                    chartTooltip->adjustSize();

                    QPoint globalPos = QCursor::pos();
                    QPoint localPos = chartView->mapFromGlobal(globalPos);
                    chartTooltip->move(localPos + QPoint(10, 10));
                    chartTooltip->show();
                    chartTooltip->raise();

                    slice->setExploded(true);
                }
                else {
                    chartTooltip->hide();
                    slice->setExploded(false);
                }
                });
        }

        pieLayout->addWidget(chartView);
        dashboardTopLayout->addLayout(pieLayout);

        auto* todoLayout = new QVBoxLayout();
        auto* todoTitle = new QLabel("Today's To-Do List", page);
        todoTitle->setProperty("class", "sectionTitle");

        m_dbTodoTableView = new QTableView(page);
        m_dbTodoModel = new PackageSmallTableModel(page);

        auto* dbTodoSortProxy = new QSortFilterProxyModel(page);
        dbTodoSortProxy->setSourceModel(m_dbTodoModel);
        dbTodoSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
        auto* dbTodoHeader = new QHeaderView(Qt::Horizontal, m_dbTodoTableView);
        dbTodoHeader->setSectionResizeMode(QHeaderView::Stretch);
        dbTodoHeader->setSectionsClickable(true);
        dbTodoHeader->setSortIndicatorShown(true);
        m_dbTodoTableView->setHorizontalHeader(dbTodoHeader);
        m_dbTodoTableView->setModel(dbTodoSortProxy);
        m_dbTodoTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_dbTodoTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_dbTodoTableView->setSortingEnabled(true);
        m_dbTodoTableView->verticalHeader()->setVisible(false);
        m_dbTodoTableView->setAlternatingRowColors(true);
        m_dbTodoTableView->setStyleSheet(tableStyle());

        todoLayout->addWidget(todoTitle);
        todoLayout->addWidget(m_dbTodoTableView);
        dashboardTopLayout->addLayout(todoLayout);


        layout->addLayout(dashboardTopLayout);

        auto* recentHeader = new QHBoxLayout();
        auto* recentTitle = new QLabel("All Packages Activity Tracker", page);
        recentTitle->setProperty("class", "sectionTitle");
        recentHeader->addWidget(recentTitle);
        recentHeader->addStretch();

        m_lateBtn = new QPushButton("Check Late Packages", page);
        m_lateBtn->setStyleSheet(buttonStyle("#e3a425"));
        recentHeader->addWidget(m_lateBtn);
        layout->addLayout(recentHeader);

        m_overdueBtn = new QPushButton("Check Overdue Packages", page);
        m_overdueBtn->setStyleSheet(buttonStyle("#E53E3E"));
        recentHeader->addWidget(m_overdueBtn);
        layout->addLayout(recentHeader);



        auto* recentLayout = new QHBoxLayout();

        m_dbRecentTableView = new QTableView(page);
        m_dbRecentModel = new PackageTableModel(page);

        auto* dbRecentSortProxy = new QSortFilterProxyModel(page);
        dbRecentSortProxy->setSourceModel(m_dbRecentModel);
        dbRecentSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
        auto* dbRecentHeader = new NoSortHeaderView(Qt::Horizontal, 0, m_dbRecentTableView);
        dbRecentHeader->setSectionResizeMode(QHeaderView::Stretch);
        dbRecentHeader->setSectionsClickable(true);
        dbRecentHeader->setSortIndicatorShown(true);
        m_dbRecentTableView->setHorizontalHeader(dbRecentHeader);
        m_dbRecentTableView->setModel(dbRecentSortProxy);
        m_dbRecentTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_dbRecentTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_dbRecentTableView->setSortingEnabled(true);
        m_dbRecentTableView->verticalHeader()->setVisible(false);
        m_dbRecentTableView->setAlternatingRowColors(true);
        m_dbRecentTableView->setStyleSheet(tableStyle());
        layout->addWidget(m_dbRecentTableView);

        connect(m_overdueBtn, &QPushButton::clicked, this, &MainWindow::onCheckOverdue);

        connect(m_lateBtn, &QPushButton::clicked, this, &MainWindow::onCheckLate); // Missing setup and implement
    }

    // Page 1: Inventory Explorer Setup
    void MainWindow::setupInventoryPage(QWidget* page)
    {
        
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* topLayout = new QHBoxLayout();
        auto* title = new QLabel("Package Inventory Explorer", page);
        title->setProperty("class", "pageTitle");
        topLayout->addWidget(title);
        topLayout->addStretch();
        
        auto* filterBtn = new QPushButton("Filter Packages", page);
        filterBtn->setStyleSheet(buttonStyle("#D69E2E"));
        topLayout->addWidget(filterBtn);
        
        layout->addLayout(topLayout);


        m_packageTableView = new QTableView(page);
        m_tableModel       = new PackageTableModel(page);

        // ── Sort proxy ────────────────────────────────────────────────────
        m_invSortProxy = new QSortFilterProxyModel(page);
        m_invSortProxy->setSourceModel(m_tableModel);
        m_invSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);

        // ── Standard header ──────────────────────────────────────────────
        auto* invHeader = new NoSortHeaderView(Qt::Horizontal, 0, m_packageTableView);

        // Global mode (no index) is safe before setModel.
        // Per-column setSectionResizeMode(col, mode) and resizeSection() call
        // visualIndex() internally and MUST come after setModel().
        invHeader->setSectionResizeMode(QHeaderView::ResizeToContents); // default all cols
        invHeader->setMinimumSectionSize(80);
        invHeader->setSectionsClickable(true);
        invHeader->setSortIndicatorShown(true);
        m_packageTableView->setHorizontalHeader(invHeader);
        m_packageTableView->setModel(m_invSortProxy);

        // ↓ Per-column settings – MUST be after setModel() ↓
        // Col 0 – ID          : fixed narrow
        // Col 1 – Name        : interactive, capped
        // Col 2 – Description : interactive, capped
        // Col 3–7             : ResizeToContents (category, weight, zone, status, import date)
        // Col 8 – Export Date : ResizeToContents – never Stretch, cannot shrink to 0
        invHeader->setSectionResizeMode(0, QHeaderView::Interactive);
        invHeader->setSectionResizeMode(1, QHeaderView::Interactive);
        invHeader->setSectionResizeMode(2, QHeaderView::Interactive);
        invHeader->setSectionResizeMode(8, QHeaderView::ResizeToContents);
        invHeader->resizeSection(0, 90);    // ID
        invHeader->resizeSection(1, 180);   // Name
        invHeader->resizeSection(2, 220);   // Description

        m_packageTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_packageTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);  // multi-select
        m_packageTableView->setSortingEnabled(true);
        m_packageTableView->verticalHeader()->setVisible(false);
        m_packageTableView->setAlternatingRowColors(true);
        m_packageTableView->setStyleSheet(tableStyle());
        layout->addWidget(m_packageTableView);

        // Apply default sort: Name A→Z
        m_invSortProxy->sort(1, Qt::AscendingOrder);


        m_summaryLabel = new QLabel(page);
        m_summaryLabel->setStyleSheet(
            "background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; "
            "padding: 12px; color: #2D3748; font-weight: bold;");
        layout->addWidget(m_summaryLabel);

        auto* toolbar = new QHBoxLayout();
        toolbar->setSpacing(8);

        m_addBtn = new QPushButton("Add Package", page);
        m_editBtn = new QPushButton("Edit Details", page);
        m_removeBtn = new QPushButton("Remove Selected", page);
        m_selectAllBtn = new QPushButton("Select All", page);
        //m_saveBtn = new QPushButton("Save Changes", page);
        //m_loadBtn = new QPushButton("Reload Data", page);
       

        m_exportCsvBtn = new QPushButton("Export CSV", page);
        m_importCsvBtn = new QPushButton("Import CSV", page);
        m_exportJsonBtn = new QPushButton("Export JSON", page);
        m_importJsonBtn = new QPushButton("Import JSON", page);

        m_addBtn->setStyleSheet(buttonStyle("#00B96B"));
        m_editBtn->setStyleSheet(buttonStyle("#4299E1"));
        m_removeBtn->setStyleSheet(buttonStyle("#E53E3E"));
        m_selectAllBtn->setStyleSheet(buttonStyle("#718096"));
        //m_saveBtn->setStyleSheet(buttonStyle("#805AD5"));
        //m_loadBtn->setStyleSheet(buttonStyle("#718096"));

        m_exportCsvBtn->setStyleSheet(buttonStyle("#805AD5"));
        m_importCsvBtn->setStyleSheet(buttonStyle("#6B46C1"));
        m_exportJsonBtn->setStyleSheet(buttonStyle("#319795"));
        m_importJsonBtn->setStyleSheet(buttonStyle("#2C7A7B"));

        toolbar->addWidget(m_addBtn);
        toolbar->addWidget(m_editBtn);
        toolbar->addWidget(m_removeBtn);
        toolbar->addWidget(m_selectAllBtn);
        toolbar->addStretch();
        toolbar->addWidget(m_exportCsvBtn);
        toolbar->addWidget(m_importCsvBtn);
        toolbar->addWidget(m_exportJsonBtn);
        toolbar->addWidget(m_importJsonBtn);

        layout->addLayout(toolbar);

        connect(filterBtn, &QPushButton::clicked, this, [this]() {
            dialogs::PackageFilterDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                auto criteria = dlg.getCriteria();
                try {
                    auto filteredPackages = m_gateway->queryPackages(criteria);
                    m_tableModel->refresh(filteredPackages);
                    if (m_summaryLabel) {
                        m_summaryLabel->setText(
                            QStringLiteral("Total filtered packages: %1")
                            .arg(m_tableModel->rowCount())
                        );
                    }
                }
                catch (const std::exception& error) {
                    showOperationError("Filter Error", error);
                }
            }
        });


        connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddPackage);
        connect(m_editBtn, &QPushButton::clicked, this, &MainWindow::onEditPackage);
        connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
        connect(m_selectAllBtn, &QPushButton::clicked, this, &MainWindow::onSelectAll);
        //connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
        //connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoad);

        connect(m_exportCsvBtn, &QPushButton::clicked, this, &MainWindow::onExportCsv);
        connect(m_importCsvBtn, &QPushButton::clicked, this, &MainWindow::onImportCsv);
        connect(m_exportJsonBtn, &QPushButton::clicked, this, &MainWindow::onExportJson);
        connect(m_importJsonBtn, &QPushButton::clicked, this, &MainWindow::onImportJson);

        connect(m_packageTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::onSelectionChanged);
    }

    // Page 2: State Operations Setup
    void MainWindow::setupOperationsPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        // ── Top header: title + Filter button ─────────────────────────────
        auto* topLayout = new QHBoxLayout();
        auto* title = new QLabel("State Transition Operations", page);
        title->setProperty("class", "pageTitle");
        topLayout->addWidget(title);
        topLayout->addStretch();

        auto* opsFilterBtn = new QPushButton("Filter Packages", page);
        opsFilterBtn->setStyleSheet(buttonStyle("#D69E2E"));
        topLayout->addWidget(opsFilterBtn);
        layout->addLayout(topLayout);

        auto* bodyLayout = new QHBoxLayout();
        bodyLayout->setSpacing(15);
        layout->addLayout(bodyLayout);

        auto* leftPanel = new QVBoxLayout();
        m_opsTableView = new QTableView(page);
        m_opsModel = new PackageCompactTableModel(page);

        // ── Sort proxy ────────────────────────────────────────────────────
        m_opsSortProxy = new QSortFilterProxyModel(page);
        m_opsSortProxy->setSourceModel(m_opsModel);
        m_opsSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);

        // ── Standard header ───────────────────────────────────────────────
        auto* opsHeader = new NoSortHeaderView(Qt::Horizontal, 0, m_opsTableView);
        m_opsTableView->setHorizontalHeader(opsHeader);
        m_opsTableView->setModel(m_opsSortProxy);

        // Col 0 – ID       : fixed narrow width, interactive resize
        // Col 1 – Name      : interactive with cap so long names don't eat everything
        // Col 2–5           : fit to content (category, zone, status, import date)
        // Col 6 – Export Date: fit to content; NOT Stretch so it cannot shrink to 0
        opsHeader->setSectionResizeMode(QHeaderView::ResizeToContents); // default all
        opsHeader->setSectionResizeMode(0, QHeaderView::Interactive);   // ID
        opsHeader->setSectionResizeMode(1, QHeaderView::Interactive);   // Name – capped
        opsHeader->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Export Date

        opsHeader->setMinimumSectionSize(90);  // every column keeps at least 90px
        opsHeader->setSectionsClickable(true);
        opsHeader->setSortIndicatorShown(true);
        opsHeader->resizeSection(0, 90);        // ID column
        opsHeader->resizeSection(1, 180);       // Name column: readable but capped
        opsHeader->setSortIndicator(1, Qt::AscendingOrder);
        m_opsSortProxy->sort(1, Qt::AscendingOrder);

        m_opsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_opsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_opsTableView->setSortingEnabled(true);
        m_opsTableView->verticalHeader()->setVisible(false);
        m_opsTableView->setAlternatingRowColors(true);
        m_opsTableView->setStyleSheet(tableStyle());

        leftPanel->addWidget(new QLabel("Select a Package to execute warehouse operations:", page));
        leftPanel->addWidget(m_opsTableView);
        bodyLayout->addLayout(leftPanel, 3);

        auto* rightCard = new QFrame(page);
        rightCard->setFixedWidth(380);
        rightCard->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; }");
        auto* rightLayout = new QVBoxLayout(rightCard);
        rightLayout->setContentsMargins(15, 15, 15, 15);
        rightLayout->setSpacing(15);

        auto* detailHeader = new QLabel("Package Operations Center", rightCard);
        detailHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: #1A202C; border-bottom: 2px solid #EDF2F7; padding-bottom: 8px;");
        rightLayout->addWidget(detailHeader);

        m_opsDetailsLabel = new QLabel(rightCard);
        m_opsDetailsLabel->setWordWrap(true);
        m_opsDetailsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_opsDetailsLabel->setStyleSheet("font-size: 13px; color: #4A5568; line-height: 1.5;");
        m_opsDetailsLabel->setText("Select a package from the table to view details and execute state actions.");

        m_opsDetailsScroll = new QScrollArea(rightCard);
        m_opsDetailsScroll->setWidget(m_opsDetailsLabel);
        m_opsDetailsScroll->setWidgetResizable(true);
        m_opsDetailsScroll->setFrameShape(QFrame::NoFrame);
        m_opsDetailsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_opsDetailsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_opsDetailsScroll->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"
            "QScrollBar:vertical { width: 6px; background: #EDF2F7; border-radius: 3px; }"
            "QScrollBar::handle:vertical { background: #CBD5E0; border-radius: 3px; min-height: 20px; }"
            "QScrollBar::handle:vertical:hover { background: #A0AEC0; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");
        rightLayout->addWidget(m_opsDetailsScroll, 1); // stretch=1: fills available space

        auto* btnGroup = new QFrame(rightCard);
        btnGroup->setStyleSheet("QFrame { border: none; }");
        auto* btnLayout = new QVBoxLayout(btnGroup);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(8);

        m_opsReceiveBtn = new QPushButton("Receive Package (Inbound)", btnGroup);
        m_opsDispatchBtn = new QPushButton("Dispatch Package (Outbound)", btnGroup);
        m_opsMissingBtn = new QPushButton("Mark as Missing", btnGroup);
        m_opsFoundBtn = new QPushButton("Mark as Found (Recovered)", btnGroup);

        m_opsReceiveBtn->setStyleSheet(buttonStyle("#48BB78"));
        m_opsDispatchBtn->setStyleSheet(buttonStyle("#3182CE"));
        m_opsMissingBtn->setStyleSheet(buttonStyle("#ED8936"));
        m_opsFoundBtn->setStyleSheet(buttonStyle("#ECC94B"));

        btnLayout->addWidget(m_opsReceiveBtn);
        btnLayout->addWidget(m_opsDispatchBtn);
        btnLayout->addWidget(m_opsMissingBtn);
        btnLayout->addWidget(m_opsFoundBtn);

        rightLayout->addWidget(btnGroup);
        bodyLayout->addWidget(rightCard, 2);

        connect(m_opsReceiveBtn, &QPushButton::clicked, this, &MainWindow::onOpsReceivePackage);
        connect(m_opsDispatchBtn, &QPushButton::clicked, this, &MainWindow::onOpsDispatchPackage);
        connect(m_opsMissingBtn, &QPushButton::clicked, this, &MainWindow::onOpsMarkMissing);
        connect(m_opsFoundBtn, &QPushButton::clicked, this, &MainWindow::onOpsMarkFound);

        // ── Filter button wiring ──────────────────────────────────────────
        connect(opsFilterBtn, &QPushButton::clicked, this, [this]() {
            dialogs::PackageFilterDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                auto criteria = dlg.getCriteria();
                try {
                    auto filteredPackages = m_gateway->queryPackages(criteria);
                    m_opsModel->refresh(filteredPackages);
                    if (m_opsFilterLabel) {
                        m_opsFilterLabel->setText(
                            QStringLiteral("Total filtered packages: %1")
                            .arg(m_opsModel->rowCount())
                        );
                    }
                    updateOpsButtonStates();
                }
                catch (const std::exception& error) {
                    showOperationError("Filter Error", error);
                }
            }
        });

        connect(m_opsTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::onOpsSelectionChanged);

        // ── Filter status label (below the table) ─────────────────────────
        m_opsFilterLabel = new QLabel(page);
        m_opsFilterLabel->setStyleSheet(
            "background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; "
            "padding: 12px; color: #2D3748; font-weight: bold;");
        layout->addWidget(m_opsFilterLabel);
    }

    // Page 3: Reports & Alerts Setup
    void MainWindow::setupReportsPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("Warehouse Reports & Action Items", page);
        title->setProperty("class", "pageTitle");
        layout->addWidget(title);

        auto* bodyLayout = new QHBoxLayout();
        bodyLayout->setSpacing(15);
        layout->addLayout(bodyLayout);

        auto* leftPanel = new QFrame(page);
        leftPanel->setFixedWidth(300);
        leftPanel->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 15px; }");
        auto* leftLayout = new QVBoxLayout(leftPanel);

        auto* summaryHeader = new QLabel("Storage Breakdown", leftPanel);
        summaryHeader->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D3748; border-bottom: 1px solid #EDF2F7; padding-bottom: 5px;");
        leftLayout->addWidget(summaryHeader);

        m_repStatsLabel = new QLabel(leftPanel);
        m_repStatsLabel->setStyleSheet("font-size: 13px; color: #4A5568; line-height: 1.6;");
        leftLayout->addWidget(m_repStatsLabel);
        leftLayout->addStretch();
        bodyLayout->addWidget(leftPanel);

        auto* rightPanel = new QVBoxLayout();
        rightPanel->setSpacing(10);

        // ── Overdue table ─────────────────────────────────────────────────
        rightPanel->addWidget(new QLabel("Overdue Packages (Action Required):", page));
        m_repOverdueTableView = new QTableView(page);
        m_repOverdueModel = new PackageCompactTableModel(page);

        m_repOverdueSortProxy = new QSortFilterProxyModel(page);
        m_repOverdueSortProxy->setSourceModel(m_repOverdueModel);
        m_repOverdueSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
        auto* repOverdueHeader = new NoSortHeaderView(Qt::Horizontal, 0, m_repOverdueTableView);
        m_repOverdueTableView->setHorizontalHeader(repOverdueHeader);
        m_repOverdueTableView->setModel(m_repOverdueSortProxy);
        repOverdueHeader->setSectionResizeMode(QHeaderView::ResizeToContents); // default all
        repOverdueHeader->setSectionResizeMode(0, QHeaderView::Interactive);   // ID
        repOverdueHeader->setSectionResizeMode(1, QHeaderView::Interactive);   // Name – capped
        repOverdueHeader->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Export Date – never Stretch
        repOverdueHeader->setMinimumSectionSize(90);
        repOverdueHeader->setSectionsClickable(true);
        repOverdueHeader->setSortIndicatorShown(true);
        repOverdueHeader->resizeSection(0, 90);   // ID
        repOverdueHeader->resizeSection(1, 180);  // Name – readable but capped
        repOverdueHeader->setSortIndicator(1, Qt::AscendingOrder);
        m_repOverdueSortProxy->sort(1, Qt::AscendingOrder);
        m_repOverdueTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_repOverdueTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_repOverdueTableView->setSortingEnabled(true);
        m_repOverdueTableView->verticalHeader()->setVisible(false);
        m_repOverdueTableView->setAlternatingRowColors(true);
        m_repOverdueTableView->setStyleSheet(tableStyle());
        rightPanel->addWidget(m_repOverdueTableView);

        // ── Missing table ─────────────────────────────────────────────────
        rightPanel->addWidget(new QLabel("Missing Packages (Under Investigation):", page));
        m_repMissingTableView = new QTableView(page);
        m_repMissingModel = new PackageCompactTableModel(page);

        m_repMissingSortProxy = new QSortFilterProxyModel(page);
        m_repMissingSortProxy->setSourceModel(m_repMissingModel);
        m_repMissingSortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
        auto* repMissingHeader = new NoSortHeaderView(Qt::Horizontal, 0, m_repMissingTableView);
        m_repMissingTableView->setHorizontalHeader(repMissingHeader);
        m_repMissingTableView->setModel(m_repMissingSortProxy);
        repMissingHeader->setSectionResizeMode(QHeaderView::ResizeToContents); // default all
        repMissingHeader->setSectionResizeMode(0, QHeaderView::Interactive);   // ID
        repMissingHeader->setSectionResizeMode(1, QHeaderView::Interactive);   // Name – capped
        repMissingHeader->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Export Date – never Stretch
        repMissingHeader->setMinimumSectionSize(90);
        repMissingHeader->setSectionsClickable(true);
        repMissingHeader->setSortIndicatorShown(true);
        repMissingHeader->resizeSection(0, 90);   // ID
        repMissingHeader->resizeSection(1, 180);  // Name – readable but capped
        repMissingHeader->setSortIndicator(1, Qt::AscendingOrder);
        m_repMissingSortProxy->sort(1, Qt::AscendingOrder);
        m_repMissingTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_repMissingTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_repMissingTableView->setSortingEnabled(true);
        m_repMissingTableView->verticalHeader()->setVisible(false);
        m_repMissingTableView->setAlternatingRowColors(true);
        m_repMissingTableView->setStyleSheet(tableStyle());
        rightPanel->addWidget(m_repMissingTableView);

        bodyLayout->addLayout(rightPanel);
    }


    void MainWindow::refreshDashboard()
    {
        const auto packages = m_gateway->getAllPackages();

        int total = static_cast<int>(packages.size());
        int storage = 0;
        int onRoute = 0;
        int dispatched = 0;
        int overdue = 0;
        int missing = 0;

        for (const auto& pkg : packages)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::InStorage:
                storage++;
                break;
            case wms::domain::PackageStateId::OnRoute:
                onRoute++;
                break;
            case wms::domain::PackageStateId::Dispatched:
                dispatched++;
                break;
            case wms::domain::PackageStateId::Overdue:
                overdue++;
                break;
            case wms::domain::PackageStateId::Missing:
                missing++;
                break;
            }
        }

        if (m_dbPlaceholderSlice)
        {
            const bool showEmptyPlaceholder = (total == 0);
            m_dbPlaceholderSlice->setValue(showEmptyPlaceholder ? 1 : 0);
            // A 0-value slice still has zero angular width but was keeping its
            // dashed border, rendering as a stray line from center to edge
            // once real data existed. Clear the pen whenever it's not
            // actively showing the "no data yet" circle.
            m_dbPlaceholderSlice->setPen(showEmptyPlaceholder
                ? QPen(QColor("#9E9E9E"), 2, Qt::DashLine)
                : QPen(Qt::NoPen));
        }
        if (m_dbStorageSlice) m_dbStorageSlice->setValue(storage);
        if (m_dbOnRouteSlice) m_dbOnRouteSlice->setValue(onRoute);
        if (m_dbDispatchedSlice) m_dbDispatchedSlice->setValue(dispatched);
        if (m_dbOverdueSlice) m_dbOverdueSlice->setValue(overdue);
        if (m_dbMissingSlice) m_dbMissingSlice->setValue(missing);

        if (m_dbCapacityProgress)
        {
            // Occupancy = packages physically present in the warehouse.
            // InStorage  : normal stored packages.
            // Overdue    : past their export date but still on the shelf.
            // OnRoute / Dispatched / Missing do NOT occupy warehouse space.
            const int occupied = storage + overdue;
            m_dbCapacityProgress->setValue(occupied);
            double percent = (static_cast<double>(occupied) / WAREHOUSE_MAX) * 100;
            if (m_dbCapacityLabel)
            {
                m_dbCapacityLabel->setText(QString("Occupancy<br>%1 / %3 (%2%)")
                    .arg(occupied)
                    .arg(percent, 0, 'f', 1)
                    .arg(WAREHOUSE_MAX));
            }

            if (percent >= 90.0)
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #E53E3E; border-radius: 6px; }");
            }
            else if (percent >= 75.0)
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #DD6B20; border-radius: 6px; }");
            }
            else
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #48BB78; border-radius: 6px; }");
            }
        }

        if (m_dbRecentModel)
        {
            m_dbRecentModel->refresh(packages);
        }

        auto toDoList = m_gateway->getDailyTodoList();
        if (m_dbTodoModel)
        {
            std::vector<domain::Package> all = toDoList.importedToday;
            all.insert(all.end(), toDoList.exportDueToday.begin(), toDoList.exportDueToday.end());
            m_dbTodoModel->refresh(all);
        }
    }

    void MainWindow::refreshOperations()
    {
        const auto packages = m_gateway->getAllPackages();
        if (m_opsModel)
        {
            m_opsModel->refresh(packages);
        }
        if (m_opsFilterLabel)
        {
            m_opsFilterLabel->setText(
                QStringLiteral("Total packages in system: %1").arg(m_opsModel ? m_opsModel->rowCount() : 0));
        }
        updateOpsButtonStates();
    }

    void MainWindow::refreshReports()
    {
        const auto packages = m_gateway->getAllPackages();

        if (m_repOverdueModel)
        {
            m_repOverdueModel->refresh(m_gateway->getOverdue());
        }
        if (m_repMissingModel)
        {
            m_repMissingModel->refresh(m_gateway->getMissing());
        }

        int standard = 0, fragile = 0, perishable = 0, hazmat = 0, oversized = 0, liquid = 0;
        int total = 0;
        for (const auto& pkg : packages)
        {
            total++;
            switch (pkg.metadata().category)
            {
            case wms::domain::Category::Standard:   standard++;   break;
            case wms::domain::Category::Fragile:    fragile++;    break;
            case wms::domain::Category::Perishable: perishable++; break;
            case wms::domain::Category::Hazmat:     hazmat++;     break;
            case wms::domain::Category::Oversized:  oversized++;  break;
            case wms::domain::Category::Liquid:     liquid++;     break;
            }
        }

        if (m_repStatsLabel)
        {
            QString statsHtml = QString(
                "<h3>Warehouse Summary</h3>"
                "<p><b>Total Packages:</b> %1</p>"
                "<br>"
                "<h3>Category Breakdown</h3>"
                "<p>📦 <b>Standard:</b> %2</p>"
                "<p>🍷 <b>Fragile:</b> %3</p>"
                "<p>🍎 <b>Perishable:</b> %4</p>"
                "<p>☣️ <b>Hazmat:</b> %5</p>"
                "<p>🏋️ <b>Oversized:</b> %6</p>"
                "<p>🧪 <b>Liquid:</b> %7</p>"
            )
                .arg(total)
                .arg(standard)
                .arg(fragile)
                .arg(perishable)
                .arg(hazmat)
                .arg(oversized)
                .arg(liquid);

            m_repStatsLabel->setText(statsHtml);
        }
    }

    void MainWindow::setupToolbar(QVBoxLayout* /*contentLayout*/)
    {
        // No longer used, empty stub for backward compatibility
    }

void MainWindow::applyFilters()
{

    if (!m_tableModel)
        return;

    // Fetch complete package collection from warehouse gateway
    auto packages = m_gateway->getAllPackages();
    m_tableModel->refresh(packages);

    // Update total package count in summary label
    if (m_summaryLabel) {
        m_summaryLabel->setText(
            QStringLiteral("Total packages in system: %1").arg(m_tableModel->rowCount()));
    }
    // Update selection-dependent action states (Edit/Remove buttons)
    updateActionStates();
}


    void MainWindow::onAddPackage()
    {
        dialogs::AddPackageDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        try
        {
            m_gateway->addPackage(dialog.packageData());
        }
        catch (const std::exception& error)
        {
            showOperationError("Add Package", error);
        }
    }

    void MainWindow::onEditPackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            wms::domain::Package package = m_gateway->getPackage(id.toStdString());
            dialogs::EditPackageDialog dialog(package, this);
            if (dialog.exec() != QDialog::Accepted)
                return;

            m_gateway->updatePackage(dialog.updatedPackage());
        }
        catch (const std::exception& error)
        {
            showOperationError("Edit Package", error);
        }
    }

    void MainWindow::onRemovePackage()
    {
        const QStringList ids = selectedPackageIds();
        if (ids.isEmpty())
        {
            QMessageBox::warning(this, "Selection Required", "Please select at least one package first.");
            return;
        }

        const int count = ids.size();
        const QString confirmMsg = (count == 1)
            ? QStringLiteral("Remove this package from the system?")
            : QStringLiteral("Remove %1 selected packages from the system?").arg(count);

        const auto reply = QMessageBox::question(
            this,
            "Remove Package",
            confirmMsg,
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        QStringList failed;
        {
            // Block packagesChanged() during the loop so each individual
            // removal doesn't trigger a full UI refresh (applyFilters +
            // refreshDashboard + refreshOperations + refreshReports).
            // One single refresh at the end is enough and makes bulk
            // deletes O(1) in UI cost instead of O(N).
            QSignalBlocker blocker(m_gateway);
            for (const QString& id : ids)
            {
                try
                {
                    m_gateway->removePackage(id.toStdString());
                }
                catch (const std::exception& error)
                {
                    failed.append(QString::fromUtf8(error.what()));
                }
            }
        } // blocker destroyed here — signals re-enabled on m_gateway

        // Single UI refresh after all deletions are committed
        onPackagesChanged();

        if (!failed.isEmpty())
        {
            QMessageBox::critical(
                this,
                "Remove Package",
                QStringLiteral("Some packages could not be removed:\n%1").arg(failed.join('\n')));
        }
    }


    void MainWindow::onSave()
    {
        try
        {
            m_gateway->save();
            m_dirty = false;
            QMessageBox::information(this, "Saved", "Package data saved successfully.");
        }
        catch (const std::exception& error)
        {
            showOperationError("Save", error);
        }
    }

    void MainWindow::onLoad()
    {
        const auto reply = QMessageBox::question(
            this,
            "Reload Data",
            "Reload from disk? Unsaved changes will be lost.",
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        try
        {
            // load() and checkOverduePackages() both emit packagesChanged()
            // internally, each triggering a full refresh mid-sequence - but
            // resetControls() below still needs its own explicit final
            // refresh afterward, since applyFilters() must run again with
            // the filter panel actually cleared, not with whatever state it
            // was in when the automatic mid-sequence refreshes fired.
            m_gateway->load();
            m_gateway->checkOverduePackages();
            onPackagesChanged();
            m_dirty = false;   // reload/refresh here is not an unsaved user
            // edit - matches the original behaviour.
        }
        catch (const std::exception& error)
        {
            showOperationError("Reload", error);
        }
    }

    void MainWindow::onReceivePackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            // Warn when the package is being received ahead of its scheduled date.
            const auto pkg        = m_gateway->getPackage(id.toStdString());
            const auto today      = std::chrono::floor<std::chrono::days>(
                std::chrono::system_clock::now());
            const auto importDate = pkg.logistics().importDate;

            if (today < importDate)
            {
                // Convert importDate to a displayable string (YYYY-MM-DD)
                const auto sysTime = std::chrono::sys_days{ importDate };
                const auto ymd     = std::chrono::year_month_day{ sysTime };
                const QString scheduledStr = QString("%1-%2-%3")
                    .arg(static_cast<int>(ymd.year()),  4, 10, QChar('0'))
                    .arg(static_cast<unsigned>(ymd.month()), 2, 10, QChar('0'))
                    .arg(static_cast<unsigned>(ymd.day()),   2, 10, QChar('0'));

                const auto reply = QMessageBox::question(
                    this,
                    "Early Receipt",
                    QString("This package is not scheduled to arrive until %1.\n"
                            "Receiving it now will record today as the actual arrival date.\n\n"
                            "Proceed with early receipt?")
                        .arg(scheduledStr),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (reply != QMessageBox::Yes)
                    return;
            }

            m_gateway->receivePackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Receive Package", error);
        }
    }

    void MainWindow::onDispatchPackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->dispatchPackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Dispatch Package", error);
        }
    }

    void MainWindow::onMarkMissing()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->markMissing(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Missing", error);
        }
    }

    void MainWindow::onMarkFound()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->markFound(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Found", error);
        }
    }

    void MainWindow::onCheckOverdue()
    {
        const int count = m_gateway->checkOverduePackages();
        if (count > 0)
        {
            QMessageBox::information(
                this,
                "Overdue Check",
                QString("%1 package(s) moved to Overdue status.").arg(count));
        }
        else
        {
            QMessageBox::information(this, "Overdue Package Check", "No packages updated to overdue.");
        }
    }

    void MainWindow::onCheckLate()
    {
        const int count = m_gateway->checkLatePackages();
        if (count > 0)
        {
            QMessageBox::information(
                this,
                "Late Package Check",
                QString("%1 package(s) moved to Missing status (failed to arrive).").arg(count));
        }
        else
        {
            QMessageBox::information(this, "Late Package Check", "No packages updated to late.");
        }
    }

    // --Export / Import helpers--

    /**
     * @brief  Returns the fixed export/import directory (<app_dir>/exports/).
     *
     *  The directory is created automatically if it does not exist yet, so
     *  the first export call will always have a valid destination folder.
     */
    static QString exportDir()
    {
        QDir dir{ QCoreApplication::applicationDirPath() + "/exports" };
        if (!dir.exists())
            dir.mkpath(".");
        return dir.absolutePath();
    }

    // --Export / Import slots--

    void MainWindow::onExportCsv()
    {
        const QString dir = exportDir();
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Export Packages to CSV",
            dir + "/packages_export.csv",
            "CSV Files (*.csv);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->exportDataCsv(path.toStdString());
            QMessageBox::information(
                this,
                "Export Successful",
                QString("All packages exported successfully to:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Export CSV", error);
        }
    }

    void MainWindow::onImportCsv()
    {
        const QString dir = exportDir();
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Import Packages from CSV",
            dir,
            "CSV Files (*.csv);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->importDataCsv(path.toStdString());
            // packagesChanged() is emitted by the gateway; onPackagesChanged()
            // already refreshes every view - no extra call needed here.
            QMessageBox::information(
                this,
                "Import Successful",
                QString("Packages imported successfully from:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Import CSV", error);
        }
    }

    void MainWindow::onExportJson()
    {
        const QString dir = exportDir();
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Export Packages to JSON",
            dir + "/packages_export.json",
            "JSON Files (*.json);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->exportDataJson(path.toStdString());
            QMessageBox::information(
                this,
                "Export Successful",
                QString("All packages exported successfully to:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Export JSON", error);
        }
    }

    void MainWindow::onImportJson()
    {
        const QString dir = exportDir();
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Import Packages from JSON",
            dir,
            "JSON Files (*.json);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->importDataJson(path.toStdString());
            // packagesChanged() is emitted by the gateway; onPackagesChanged()
            // already refreshes every view - no extra call needed here.
            QMessageBox::information(
                this,
                "Import Successful",
                QString("Packages imported successfully from:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Import JSON", error);
        }
    }

    void MainWindow::onSelectionChanged()
    {
        updateActionStates();
    }

    void MainWindow::onTimerExec()
    {
        // Only the overdue scan runs automatically on the hourly timer.
        // Late-package checking is a deliberate manual action ("Check Late"
        // button) and must NOT run silently in the background.
        m_gateway->checkOverduePackages();
    }

    void MainWindow::updateActionStates()
    {
        if (!m_packageTableView)
            return;
        const bool hasSelection = m_packageTableView->selectionModel()->hasSelection();
        const int selCount = m_packageTableView->selectionModel()->selectedRows().count();
        if (m_editBtn)   m_editBtn->setEnabled(selCount == 1);  // Edit only one at a time
        if (m_removeBtn) m_removeBtn->setEnabled(hasSelection);
    }

    QString MainWindow::selectedPackageId() const
    {
        if (!m_packageTableView || !m_tableModel)
            return {};

        const QModelIndex sortIndex = m_packageTableView->currentIndex();
        if (!sortIndex.isValid())
        {
            QMessageBox::warning(
                const_cast<MainWindow*>(this),
                "Selection Required",
                "Please select a package first.");
            return {};
        }

        // Map: sort proxy → source model
        const QModelIndex sourceIndex = m_invSortProxy
            ? m_invSortProxy->mapToSource(sortIndex)
            : sortIndex;

        return m_tableModel->packageIdAt(sourceIndex.row());
    }

    QStringList MainWindow::selectedPackageIds() const
    {
        if (!m_packageTableView || !m_tableModel)
            return {};

        const QModelIndexList sortedRows =
            m_packageTableView->selectionModel()->selectedRows();
        if (sortedRows.isEmpty())
            return {};

        QStringList ids;
        ids.reserve(sortedRows.size());
        for (const QModelIndex& sortIndex : sortedRows)
        {
            const QModelIndex sourceIndex = m_invSortProxy
                ? m_invSortProxy->mapToSource(sortIndex)
                : sortIndex;
            const QString id = m_tableModel->packageIdAt(sourceIndex.row());
            if (!id.isEmpty())
                ids.append(id);
        }
        return ids;
    }


    void MainWindow::showOperationError(const char* title, const std::exception& error)
    {
        QMessageBox::critical(this, title, error.what());
    }

    void MainWindow::onPackagesChanged()
    {
        m_dirty = true;
        applyFilters();
        refreshDashboard();
        refreshOperations();
        refreshReports();
    }

    // New slots implementation
    void MainWindow::onSidebarCurrentRowChanged(int row)
    {
        if (m_stackedWidget)
        {
            m_stackedWidget->setCurrentIndex(row);
        }
        switch (row)
        {
        case 0:
            refreshDashboard();
            break;
        case 1:
            applyFilters();
            break;
        case 2:
            refreshOperations();
            break;
        case 3:
            refreshReports();
            break;
        }
    }

    QString MainWindow::selectedOpsPackageId() const
    {
        if (!m_opsTableView || !m_opsModel)
            return {};
        const QModelIndex sortIdx = m_opsTableView->currentIndex();
        if (!sortIdx.isValid())
            return {};
        // Map: sort proxy → source model
        const QModelIndex sourceIdx = m_opsSortProxy
            ? m_opsSortProxy->mapToSource(sortIdx) : sortIdx;
        return m_opsModel->packageIdAt(sourceIdx.row());
    }

    void MainWindow::onOpsSelectionChanged()
    {
        updateOpsButtonStates();
    }

    void MainWindow::onOpsReceivePackage()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            // Warn when the package is being received ahead of its scheduled date.
            const auto pkg        = m_gateway->getPackage(id.toStdString());
            const auto today      = std::chrono::floor<std::chrono::days>(
                std::chrono::system_clock::now());
            const auto importDate = pkg.logistics().importDate;

            if (today < importDate)
            {
                // Convert importDate to a displayable string (YYYY-MM-DD)
                const auto sysTime = std::chrono::sys_days{ importDate };
                const auto ymd     = std::chrono::year_month_day{ sysTime };
                const QString scheduledStr = QString("%1-%2-%3")
                    .arg(static_cast<int>(ymd.year()),  4, 10, QChar('0'))
                    .arg(static_cast<unsigned>(ymd.month()), 2, 10, QChar('0'))
                    .arg(static_cast<unsigned>(ymd.day()),   2, 10, QChar('0'));

                const auto reply = QMessageBox::question(
                    this,
                    "Early Receipt",
                    QString("This package is not scheduled to arrive until %1.\n"
                            "Receiving it now will record today as the actual arrival date.\n\n"
                            "Proceed with early receipt?")
                        .arg(scheduledStr),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (reply != QMessageBox::Yes)
                    return;
            }

            m_gateway->receivePackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Receive Package", error);
        }
    }

    void MainWindow::onOpsDispatchPackage()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->dispatchPackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Dispatch Package", error);
        }
    }

    void MainWindow::onOpsMarkMissing()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->markMissing(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Missing", error);
        }
    }

    void MainWindow::onOpsMarkFound()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->markFound(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Found", error);
        }
    }

    void MainWindow::updateOpsButtonStates()
    {
        if (!m_opsTableView || !m_opsModel || !m_opsDetailsLabel)
            return;

        const QModelIndex sortIdx = m_opsTableView->currentIndex();
        if (!sortIdx.isValid())
        {
            m_opsDetailsLabel->setText("Select a package from the table to view details and execute state actions.");
            if (m_opsReceiveBtn) m_opsReceiveBtn->setEnabled(false);
            if (m_opsDispatchBtn) m_opsDispatchBtn->setEnabled(false);
            if (m_opsMissingBtn) m_opsMissingBtn->setEnabled(false);
            if (m_opsFoundBtn) m_opsFoundBtn->setEnabled(false);
            return;
        }

        const QModelIndex sourceIdx = m_opsSortProxy
            ? m_opsSortProxy->mapToSource(sortIdx) : sortIdx;
        const wms::domain::Package* pkg = m_opsModel->packageAt(sourceIdx.row());

        if (!pkg)
            return;

        auto formatDate = [](const wms::domain::Date& date) {
            return QString("%1-%2-%3")
                .arg(static_cast<int>(date.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.day()), 2, 10, QChar('0'));
            };

        QString categoryStr;
        switch (pkg->metadata().category)
        {
        case wms::domain::Category::Standard:   categoryStr = "Standard"; break;
        case wms::domain::Category::Fragile:    categoryStr = "Fragile"; break;
        case wms::domain::Category::Perishable: categoryStr = "Perishable"; break;
        case wms::domain::Category::Hazmat:     categoryStr = "Hazmat"; break;
        case wms::domain::Category::Oversized:  categoryStr = "Oversized"; break;
        case wms::domain::Category::Liquid:     categoryStr = "Liquid"; break;
        }

        QString details = QString(
            "<b>ID:</b> %1<br>"
            "<b>Name:</b> %2<br>"
            "<b>Description:</b> %3<br>"
            "<b>Category:</b> %4<br>"
            "<b>Weight:</b> %5 kg<br>"
            "<b>Status:</b> %6<br>"
            "<b>Location:</b> Zone %7, Aisle %8, Shelf %9, Slot %10<br>"
            "<b>Source:</b> %11<br>"
            "<b>Destination:</b> %12<br>"
            "<b>Import Date:</b> %13<br>"
            "<b>Export Date:</b> %14"
        )
            .arg(QString::fromStdString(pkg->id()))
            .arg(QString::fromStdString(pkg->metadata().name))
            .arg(QString::fromStdString(pkg->metadata().description))
            .arg(categoryStr)
            .arg(pkg->metadata().weight)
            .arg(QString::fromUtf8(pkg->currentState().getStateLabel().data(), static_cast<int>(pkg->currentState().getStateLabel().size())))
            .arg(QString::fromStdString(pkg->location().zone))
            .arg(QString::fromStdString(pkg->location().aisle))
            .arg(pkg->location().shelf)
            .arg(pkg->location().slot)
            .arg(QString::fromStdString(pkg->source().city))
            .arg(QString::fromStdString(pkg->destination().city))
            .arg(formatDate(pkg->logistics().importDate))
            .arg(formatDate(pkg->logistics().expectedExportDate));

        m_opsDetailsLabel->setText(details);

        const auto stateId = pkg->currentStateId();

        if (m_opsReceiveBtn) m_opsReceiveBtn->setEnabled(stateId == wms::domain::PackageStateId::OnRoute);
        if (m_opsDispatchBtn) m_opsDispatchBtn->setEnabled(stateId == wms::domain::PackageStateId::InStorage ||
            stateId == wms::domain::PackageStateId::Overdue);
        if (m_opsMissingBtn) m_opsMissingBtn->setEnabled(stateId != wms::domain::PackageStateId::Dispatched &&
            stateId != wms::domain::PackageStateId::Missing);
        if (m_opsFoundBtn) m_opsFoundBtn->setEnabled(stateId == wms::domain::PackageStateId::Missing);
    }

    void MainWindow::onSelectAll()
    {
        if (m_packageTableView)
        {
            m_packageTableView->selectAll();
            updateActionStates();
        }
    }

} // namespace wms::gui

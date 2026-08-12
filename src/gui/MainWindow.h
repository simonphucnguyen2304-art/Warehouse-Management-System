/**
 * @file    MainWindow.h
 * @brief   Main WMS interface wired to WarehouseGateway.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Extended UI to multi-page layout (Dashboard, Inventory, Operations, Reports)
 *   - Added sidebar navigation and page-specific refresh helpers
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-12
 * @changelog
 *   - Replaced Dashboard statistics with pie chart
 *   - Fixed unsaved changes prompt appearing when no changes were made
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Commented out Save and Load buttons due to redundancy with SQLite database
 *
 * @update
 * @author  Do Minh Khang
 * @date    2026-07-23
 * @changelog
 *   - Replaced the direct WarehouseManager* dependency with WarehouseGateway*.
 *     MainWindow no longer calls persistAndRefresh() manually after each
 *     mutation - WarehouseGateway emits packagesChanged() after every
 *     successful mutation, and MainWindow's new onPackagesChanged() slot
 *     (replacing persistAndRefresh()) reacts to that signal instead.
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-25
 * @changelog
 *   - Added Export/Import buttons (CSV and JSON) to the Inventory toolbar.
 *     Each button triggers a QFileDialog, then calls the corresponding
 *     WarehouseGateway method. Import calls also call updateTable() via
 *     the existing packagesChanged() → onPackagesChanged() signal chain.
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 * @changelog
 *   - Added FilterableHeaderView* and MultiColumnFilterProxy* members for
 *     every table page (Dashboard, Inventory, Operations, Reports x2).
 *   - Added showColumnFilterPopup() private helper method declaration.
 *   - Included MultiColumnFilterProxy.h and FilterableHeaderView.h.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Add onLateCheck() slots for newly create checkLatePackages().
 *   - Add associated button.
 *   - Re-name onOverdueTimer to onTimerExec since it now manage both
 *     overdue and missing (late) check.
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-31
 * @changelog
 *   - Removed FilterableHeaderView* and MultiColumnFilterProxy* members
 *     from all table pages (Dashboard, Inventory, Operations, Reports x2).
 *   - Removed showColumnFilterPopup() private helper method declaration.
 *   - Removed #include of MultiColumnFilterProxy.h and FilterableHeaderView.h.
 *   - Replaced MultiColumnFilterProxy with QSortFilterProxyModel (sort only).
 *   - Enabled column header sort indicators via setSectionsClickable(true)
 *     and setSortIndicatorShown(true) on all QHeaderView instances.
 *   - Added SVG sort arrow icons (sort_asc.svg, sort_desc.svg) embedded
 *     via qt_add_resources in CMakeLists.txt; referenced in QSS stylesheets.
 */

#pragma once

#include <QMainWindow>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QProgressBar>
#include <QPieSlice>
#include <QScrollArea>

#include <QSortFilterProxyModel>
#include "PackageTableModel.h"
#include "WarehouseGateway.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace wms::gui {

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(WarehouseGateway* gateway, QWidget* parent = nullptr);
        ~MainWindow() override;

    protected:
        void closeEvent(QCloseEvent* event) override;

    private slots:
        /**
         * @brief  Observer reaction to WarehouseGateway::packagesChanged().
         *         Replaces persistAndRefresh() - same body, but triggered
         *         by the Subject's signal instead of called manually from
         *         every mutating handler.
         */
        void onPackagesChanged();
        void onAddPackage();
        void onEditPackage();
        void onRemovePackage();
        void onSelectAll();
        void onSave();
        void onLoad();
        void onReceivePackage();
        void onDispatchPackage();
        void onMarkMissing();
        void onMarkFound();
        void onCheckOverdue();
        void onCheckLate();
        void onSelectionChanged();
        void onTimerExec();

        // Export / Import slots
        void onExportCsv();
        void onImportCsv();
        void onExportJson();
        void onImportJson();

        // New slots for the multi-page interface
        void onSidebarCurrentRowChanged(int row);
        void onOpsSelectionChanged();
        void onOpsReceivePackage();
        void onOpsDispatchPackage();
        void onOpsMarkMissing();
        void onOpsMarkFound();

    private:
        void setupToolbar(QVBoxLayout* contentLayout);
        void updateActionStates();
        void applyFilters();
        QString selectedPackageId() const;
        QStringList selectedPackageIds() const;
        QString selectedOpsPackageId() const;
        void showOperationError(const char* title, const std::exception& error);

        // Setup helpers for each page
        void setupDashboardPage(QWidget* page);
        void setupInventoryPage(QWidget* page);
        void setupOperationsPage(QWidget* page);
        void setupReportsPage(QWidget* page);

        // Refresh helpers
        void refreshDashboard();
        void refreshOperations();
        void refreshReports();
        void updateOpsButtonStates();

        Ui::MainWindow* ui{ nullptr };
        WarehouseGateway* m_gateway{ nullptr };
        bool m_dirty{ false };

        QListWidget* m_sidebarMenu{ nullptr };
        QStackedWidget* m_stackedWidget{ nullptr };

        // Page 0: Dashboard
        QTableView*             m_dbTodoTableView{ nullptr };

        // Page 1: Inventory
        QTableView* m_packageTableView{ nullptr };
        PackageTableModel* m_tableModel{ nullptr };
        QSortFilterProxyModel*  m_invSortProxy{ nullptr };

        // Page 2: State Operations
        QSortFilterProxyModel*  m_opsSortProxy{ nullptr };

        // Page 3: Reports
        QSortFilterProxyModel*  m_repOverdueSortProxy{ nullptr };
        QSortFilterProxyModel*  m_repMissingSortProxy{ nullptr };

        QTimer* m_overdueTimer{ nullptr };

        QPushButton* m_addBtn{ nullptr };
        QPushButton* m_editBtn{ nullptr };
        QPushButton* m_removeBtn{ nullptr };
        QPushButton* m_selectAllBtn{ nullptr };
        //QPushButton* m_saveBtn{ nullptr };
        //QPushButton* m_loadBtn{ nullptr };
        QPushButton* m_receiveBtn{ nullptr };
        QPushButton* m_dispatchBtn{ nullptr };
        QPushButton* m_missingBtn{ nullptr };
        QPushButton* m_foundBtn{ nullptr };
        QPushButton* m_overdueBtn{ nullptr };
        QPushButton* m_lateBtn{ nullptr };
        QLabel* m_summaryLabel{ nullptr };

        // Export / Import buttons (Inventory page toolbar)
        QPushButton* m_exportCsvBtn { nullptr };
        QPushButton* m_importCsvBtn { nullptr };
        QPushButton* m_exportJsonBtn{ nullptr };
        QPushButton* m_importJsonBtn{ nullptr };

        // Page 0: Dashboard metrics
        QPieSlice* m_dbPlaceholderSlice{ nullptr };
        QPieSlice* m_dbStorageSlice{ nullptr };
        QPieSlice* m_dbOnRouteSlice{ nullptr };
        QPieSlice* m_dbDispatchedSlice{ nullptr };
        QPieSlice* m_dbOverdueSlice{ nullptr };
        QPieSlice* m_dbMissingSlice{ nullptr };
        QProgressBar* m_dbCapacityProgress{ nullptr };
        QLabel* m_dbCapacityLabel{ nullptr };
        QTableView* m_dbRecentTableView{ nullptr };
        PackageSmallTableModel* m_dbTodoModel{ nullptr };
        PackageTableModel* m_dbRecentModel{ nullptr };

        // Page 2: State Operations
        QTableView* m_opsTableView{ nullptr };
        PackageTableModel* m_opsModel{ nullptr };
        QLabel* m_opsDetailsLabel{ nullptr };
        QScrollArea* m_opsDetailsScroll{ nullptr };  ///< Scroll wrapper for m_opsDetailsLabel
        QLabel* m_opsFilterLabel{ nullptr };   ///< Shows current package count / filter status
        QPushButton* m_opsReceiveBtn{ nullptr };
        QPushButton* m_opsDispatchBtn{ nullptr };
        QPushButton* m_opsMissingBtn{ nullptr };
        QPushButton* m_opsFoundBtn{ nullptr };

        // Page 3: Reports & Analytics
        QTableView* m_repOverdueTableView{ nullptr };
        PackageTableModel* m_repOverdueModel{ nullptr };
        QTableView* m_repMissingTableView{ nullptr };
        PackageTableModel* m_repMissingModel{ nullptr };
        QLabel* m_repStatsLabel{ nullptr };
    };

} // namespace wms::gui
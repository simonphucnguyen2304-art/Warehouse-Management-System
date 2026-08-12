# WarehouseMS - Warehouse Management System
A warehouse/package tracking application built in C++20 with Qt6, for the Object-Oriented Programming project in HCMUS.

## Structure
```
├── main.cpp                          # Application entry point
├── docs/
│   ├── QTINSTRUCTION.md              # Qt setup / environment instructions
│   └── GUIDELINE.md                  # Project / coding guidelines
├── resources/
│   ├── theme.qss                     # Application stylesheet
│   ├── icons.qrc                     # Qt resource collection for icons
│   ├── icons/                        # SVG icons (sort, chevrons, etc.)
│   ├── data/packages.json            # Sample / seed package data
│   └── db/schema.sql                 # SQLite database schema
├── src/
│   ├── domain/
│   │   ├── entities/
│   │   │   ├── Address.h
│   │   │   ├── Category.h
│   │   │   ├── Date.h
│   │   │   ├── Dimension.h
│   │   │   ├── LogisticsInfo.h
│   │   │   ├── Package.h
│   │   │   ├── Package.cpp
│   │   │   ├── PackageMetadata.h
│   │   │   └── StorageLocation.h
│   │   ├── states/
│   │   │   ├── IPackageState.h
│   │   │   ├── PackageStateId.h
│   │   │   ├── InStorageState.*
│   │   │   ├── OnRouteState.*
│   │   │   ├── DispatchedState.*
│   │   │   ├── MissingState.*
│   │   │   ├── OverdueState.*
│   │   └── queries/
│   │       └── PackageQueryCriteria.h
│   ├── repository/
│   │   ├── IPackageRepository.h
│   │   ├── SqlitePackageRepository.*  
│   │   ├── JsonPackageRepository.*
│   │   ├── DatabaseConnection.*
│   │   └── RepositoryHelpers.h
│   ├── service/
│   │   └── WarehouseManager.* 
│   └── gui/
│       ├── MainWindow.*
│       ├── WarehouseGateway.* 
│       ├── PackageTableModel.*
│       └── dialogs/
│           ├── AddPackageDialog.*
│           ├── EditPackageDialog.*
│           ├── PackageFilterDialog.*
├── CMakeLists.txt                    # Build configuration
├── CMakePresets.json                 # CMake presets
└── CMakeUserPresets.json.example     # Local Qt path template
```

## Requirements
- C++20 compiler (MSVC Visual Studio 2022, "Desktop development with C++" workload)
- CMake ≥ 3.23
- Qt 6 with the Core, Widgets, Gui, Charts, Sql, Svg components installed
See ``docs/QTINSTRUCTION.md`` for detailed Qt environment setup.

## Build and Run
- Copy the ``CMakeUserPresets.json.example`` file and rename it to ``CMakeUserPresets.json``.
Change ``"CMAKE_PREFIX_PATH"`` to the Qt installation's path (``C:/Qt/6.11.1/msvc2022_64`` by default).
- Run ``cmake --build build``. The executable file can be found at ``build/RelWithDebInfo/WarehouseMS.exe``.

## Contributors
- Dương Anh Hào
- Huỳnh Phúc Nguyên
- Lâm Hồng Hải Hoàng Lê
- Nguyễn Viết Bách
- Đỗ Minh Khang
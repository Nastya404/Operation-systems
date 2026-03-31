# Лабораторная работа №2 — Создание потоков

## Задание

Консольное приложение Windows с тремя потоками: `main`, `min_max` и `average`.

- **main** — создаёт массив, запускает потоки, ждёт их завершения, заменяет min и max элементы средним арифметическим.
- **min_max** — находит минимальный и максимальный элементы массива (Sleep 7 мс после каждого сравнения).
- **average** — вычисляет среднее арифметическое элементов массива (Sleep 12 мс после каждого суммирования).

## Структура проекта

```
├── CMakeLists.txt
├── include/
│   ├── shared_data.h       # Структура SharedData для обмена данными между потоками
│   └── threads.h           # Прототипы функций потоков
├── src/
│   ├── main.cpp            # Главный поток
│   └── threads.cpp         # Реализация MinMaxThread и AverageThread
└── tests/
    └── UnitTest.cpp        # Юнит-тесты (Microsoft CppUnitTestFramework)
```

## Сборка через CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
.\Debug\lab2.exe
```

## Запуск юнит-тестов

Тесты используют Microsoft CppUnitTestFramework и запускаются через Visual Studio:

1. Откройте решение в Visual Studio
2. Test → Test Explorer
3. Run All

## Используемые технологии

- C++17
- Windows API (CreateThread, WaitForSingleObject, CloseHandle)
- CMake
- Microsoft CppUnitTestFramework

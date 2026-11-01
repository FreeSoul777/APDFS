**Читать на [английском](README.md)**

# APDFS: Adaptive Parallel Depth-First Search

APDFS — это высокопроизводительный алгоритм для перечисления **всех минимальных (S,T)-разрезов** в ориентированных графах. Реализует параллельный depth-first search с work-stealing балансировкой нагрузки.

## Сборка

### Зависимости

- Linux (x86-64). Поддерживается только Linux: код использует `<immintrin.h>`/`_mm_pause`
  и `<sys/resource.h>`/`getrusage`
- CMake >= 3.20
- Компилятор с поддержкой C++20: GCC или Clang (MSVC не поддерживается)
- POSIX Threads (glibc)

### Базовая сборка

```bash
git clone https://github.com/FreeSoul777/APDFS.git
cd APDFS

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Опции сборки

| Опция | По умолчанию | Описание |
|-------|-------------|-----------|
| `APDFS_BUILD_TESTS` | ON | Сборка unit-тестов |
| `APDFS_BUILD_BENCHMARKS` | OFF | Сборка бенчмарков |
| `APDFS_BUILD_TOOLS` | ON | Сборка инструментов (graphgen, cutreader) |
| `APDFS_ENABLE_CLANG_FORMAT` | ON | Включить цель clang-format |
| `APDFS_ENABLE_CLANG_TIDY` | OFF | Включить цель clang-tidy |

Пример полной сборки:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DAPDFS_BUILD_TESTS=ON \
    -DAPDFS_BUILD_BENCHMARKS=ON \
    -DAPDFS_BUILD_TOOLS=ON

cmake --build build -j$(nproc)
```

## Использование

### Запуск

```bash
./build/src/apdfs_cli <graph_file> <source> <sink> [threads] [output_dir] [--progress] [--max-cuts=N] [--max-cut-size=N] [--max-examined=N]
```

Справочник параметров (`apdfs --help` печатает тот же текст):

| Аргумент | Значение | Когда нужен |
|---|---|---|
| `graph_file` | текстовый граф: `# комментарии`, `<вершин> <рёбер>`, затем по строке `<u> <v>` | всегда |
| `source`, `sink` | идентификаторы вершин `S*` и `T*` | всегда |
| `threads` | рабочие потоки, по умолчанию — число ядер | фиксировать для воспроизводимых замеров |
| `output_dir` | каталог вывода, по умолчанию `./output` | |
| `--threads=N` | то же, что позиционный `threads` | явные флаги перекрывают позиционные |
| `--output=DIR` | то же, что позиционный `output_dir` | явные флаги перекрывают позиционные |
| `--progress` | живая строка: разрезы, дубликаты, куча, память, ETA | длинные прогоны |
| `--max-cuts=N` | остановиться после `N` **выданных** разрезов | ранняя остановка на больших выдачах |
| `--max-cut-size=N` | выдавать только разрезы не длиннее `N` рёбер | в надёжности интересны малые сечения |
| `--max-examined=N` | остановиться после `N` **просмотренных** разрезов | единственный ограничитель, когда фильтр по размеру отсекает почти всё |
| `-h`, `--help` | подсказка и коды возврата | |
| `--version` | версия программы | |

Два флага выглядят симметрично, но таковыми не являются. `--max-cut-size` — **фильтр вывода**: поиск
всё равно проходит через крупные разрезы (у большого разреза бывают мелкие потомки), поэтому полнота
гарантируется только для разрезов в пределах границы. `--max-cuts` считает **выданные** разрезы,
поэтому прогону с фильтром по размеру нужен `--max-examined`, иначе он не остановится. Неизвестные
опции, отсутствующие значения и нечисловые значения отвергаются с кодом возврата 2.

Пример:

```bash
./build/src/apdfs_cli graph.txt 0 99 4 output --progress
```

### Формат входного файла

```
# Комментарий
<число_вершин> <число_рёбер>
<u> <v>
<u> <v>
...
```

Пример:

```
# Простой граф
4 5
0 1
0 2
1 3
2 3
1 2
```

## Тестирование

```bash
# Сборка с тестами
cmake -S . -B build -DAPDFS_BUILD_TESTS=ON
cmake --build build -j$(nproc)

# Запуск всех тестов
cd build
ctest --output-on-failure

# Запуск конкретного теста
./tests/apdfs_tests --gtest_filter=TestBfsEngine.*
```

## Бенчмарки

Бенчмарки измеряют производительность алгоритма на различных графах и с разным числом потоков. Используют Google Benchmark.

### Запуск

```bash
# Сборка с бенчмарками
cmake -S . -B build -DAPDFS_BUILD_BENCHMARKS=ON
cmake --build build -j$(nproc)

# Все бенчмарки
./build/benchmarks/bench_apdfs

# Конкретный бенчмарк
./build/benchmarks/bench_apdfs --benchmark_filter=Chain

# Только с 1 потоком
./build/benchmarks/bench_apdfs --benchmark_filter=Chain1T

# С 4 потоками
./build/benchmarks/bench_apdfs --benchmark_filter=Chain4T

# Сохранение результатов
./build/benchmarks/bench_apdfs --benchmark_out=results.json --benchmark_out_format=json
```

### Что измеряют

- **Chain1T/Chain4T** — цепочки алмазов (K=10, 12, 15, 20) с 1 и 4 потоками
- **Random1T/Random4T** — случайный граф (20 вершин, 60 рёбер) с 1 и 4 потоками

## Инструменты

### Генератор графов

```bash
./build/tools/graphgen <mode> [parameters]
```

Режимы:

**chain** — цепочка K "алмазов" (последовательное соединение ромбов):
```bash
./build/tools/graphgen chain <K> [output_file]
```
- `K` — число алмазов в цепочке
- Каждый алмаз добавляет 3 вершины и 4 ребра
- По умолчанию: `chain_K.txt`

**random** — случайный граф с основным путём 0→1→...→(n-1) и дополнительными рёбрами:
```bash
./build/tools/graphgen random <vertices> <edges> <side_rate> <cycle_rate> [output_file]
```
- `vertices` — число вершин
- `edges` — число рёбер
- `side_rate` — доля боковых ветвей (рёбра вперёд, не соседние)
- `cycle_rate` — доля обратных рёбер (создают циклы)
- По умолчанию: `random_V_E.txt`

Пример:
```bash
# Цепочка из 5 алмазов
./build/tools/graphgen chain 5

# Случайный граф: 20 вершин, 60 рёбер, 20% ветвей, 20% циклов
./build/tools/graphgen random 20 60 0.2 0.2
```

### Просмотр разрезов

```bash
./build/tools/cutreader <output_dir> [--page N] [--human]
```

Параметры:
- `output_dir` — директория с `.bin` файлами разрезов
- `--page N` — постраничный вывод по N разрезов
- `--human` — показать рёбра как `source->target` (требует `edges.map` в output_dir)

Пример:
```bash
# Весь список
./build/tools/cutreader output

# По 50 разрезов с ожиданием Enter
./build/tools/cutreader output --page 50

# Человекочитаемый формат
./build/tools/cutreader output --human
```

## Формат результатов

Алгоритм записывает разрезы в бинарные `.bin` файлы в указанную директорию. Каждый файл содержит последовательность разрезов.

## Алгоритм

APDFS реализует параллельный DFS по неявному графу минимальных разрезов. Подробное описание алгоритма, доказательства корректности и полноты: [docs/APDFS_SPECIFICATION_RU.md](docs/APDFS_SPECIFICATION_RU.md)

## License

APDFS is licensed under the Apache License, Version 2.0.

Copyright 2026 FreeSoul777

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Author

**FreeSoul777** (https://github.com/FreeSoul777)

## Citation

Если вы используете APDFS в своих исследованиях, пожалуйста, укажите ссылку на него:

```bibtex
@software{apdfs2026,
  author = {FreeSoul777},
  title = {APDFS: Adaptive Parallel Depth-First Search for Enumerating All Minimal (S,T)-Cuts},
  year = {2026},
  url = {https://github.com/FreeSoul777/APDFS}
}
```

## Вклад в проект

Перед созданием pull request убедитесь, что выполнены следующие шаги:

1. **Форматирование кода**:
```bash
cmake --build build --target clang-format-check
```

2. **Статический анализ**:
```bash
cmake --build build --target clang-tidy-check
```

3. **Тесты**:
```bash
ctest --test-dir build --output-on-failure
```

4. **Опишите изменения**: что сделано, зачем и какие проблемы это решает
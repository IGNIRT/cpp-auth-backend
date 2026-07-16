# C++ Auth Backend (Test Task)

Реализация HTTP-эндпоинта `/auth/register` на C++ с сохранением пользователя в PostgreSQL.

## Стек технологий
- **C++17**
- **HTTP-сервер:** `cpp-httplib` (header-only)
- **JSON:** `nlohmann/json` (header-only)
- **Хеширование:** Argon2id (`libargon2`)
- **База данных:** PostgreSQL (`libpq`)

## Особенности реализации
- **Безопасность:** Использование параметризованных запросов (`PQexecParams`) для полной защиты от SQL-инъекций.
- **Хеширование:** Пароли хешируются с помощью Argon2id с уникальной солью для каждого пользователя.
- **Конфигурация:** Все настройки читаются из **переменных окружения** (`DB_HOST`, `DB_PORT`, `DB_NAME`, `DB_USER`, `DB_PASSWORD`, `SERVER_PORT`).
- **Валидация:** Строгая проверка всех полей согласно спецификации (длина, формат email, сложность пароля, обязательные поля для бизнес-аккаунтов).

## Как запустить

### 1. Установка зависимостей
**Windows (vcpkg):**
```bash
vcpkg install libpq:x64-windows argon2:x64-windows
```
**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install libpq-dev libargon2-dev cmake g++
```

### 2. Настройка базы данных
Создайте базу данных и выполните скрипт `init.sql`:
```bash
psql -U postgres -d mydb -f init.sql
```

### 3. Настройка переменных окружения
Создайте файл `.env` в корне проекта или экспортируйте переменные в терминале:
```bash
export DB_HOST=localhost
export DB_PORT=5432
export DB_NAME=mydb
export DB_USER=postgres
export DB_PASSWORD=your_secure_password
export SERVER_PORT=8090
```

### 4. Сборка и запуск
Соберите проект в Visual Studio или через CMake и запустите исполняемый файл. Сервер будет доступен по адресу `http://localhost:8090`.

## Пример запроса
```bash
curl -X POST http://localhost:8090/auth/register \
-H "Content-Type: application/json" \
-d '{
  "username": "testuser",
  "email": "test@mail.com",
  "phone": "+79001234567",
  "password": "SecurePass1"
}'
```

## Примечание для ревью
1. **Пул соединений:** В текущей реализации используется глобальное соединение с БД (`g_conn`) для упрощения тестового задания. В продакшен-среде, учитывая многопоточную природу `cpp-httplib`, необходимо использовать пул соединений (connection pool) для обеспечения потокобезопасности.
2. **Пароль по умолчанию:** Значение пароля по умолчанию в `getEnvOrDefault` оставлено только для удобства локального тестирования этого задания. В реальном проекте, если переменная окружения `DB_PASSWORD` не задана, программа должна выбрасывать исключение или завершаться с ошибкой, чтобы не использовать небезопасные значения по умолчанию.
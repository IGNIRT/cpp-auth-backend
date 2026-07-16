# C++ бэкенд: регистрация пользователя
Реализация HTTP-эндпоинта `/auth/register` на C++ с сохранением пользователя в PostgreSQL.

## Стек технологий
- **C++ 17** (Visual Studio 2026)
- **HTTP-сервер:** `httplib.h` (header-only)
- **JSON:** `nlohmann/json.hpp`
- **Хеширование:** Argon2id (`argon2.h`)
- **База данных:** PostgreSQL 18 (`libpq`)

## Структура проекта
main.cpp # HTTP-сервер и обработчик
valid.h / valid.cpp # Валидация данных
database.h / database.cpp # Подключение к PostgreSQL
httplib.h # Библиотека для HTTP
json.hpp # Библиотека для JSON
argon2.h # Библиотека для хеширования
init.sql # SQL-скрипт для создания таблицы
README.md # Эта инструкция

## База данных
Перед запуском проекта выполни скрипт `init.sql` в PostgreSQL.

### 1. Создание базы данных
```sql
CREATE DATABASE mydb;
```
### 2. Запуск скрипта
```bash
psql -U postgres -d mydb -f init.sql
```
### 3. Подключение с вашим паролем 
В файле database.cpp замените пароль на свой:
```cpp
string getConnectionString() {
    return "host=localhost port=5432 dbname=mydb user=postgres password=";
}
```
### 4. Откройте проект в Visual Studio
Соберите проект: Ctrl + Shift + B
Запустите: F5
В консоли увидите: Server starting on port 8090...

## Примеры запроса
POST http://localhost:8090/auth/register
Body (JSON):
```json
{
  "username": "testuser",
  "email": "test@mail.com",
  "phone": "+79001234567",
  "password": "SecurePass1"
}
```
Успешный ответ:
```json
{
  "success": true,
  "message": "User registered successfully.",
  "user": {
    "username": "testuser",
    "email": "test@mail.com",
    "role": "individual",
    "id": "550e8400-e29b-41d4-a716-446655440000"
  }
}
```

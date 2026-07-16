# Тестовое задание: C++ разработчик (backend)

## Задача

Реализовать HTTP-эндпоинт `/auth/register` на C++ с сохранением пользователя в PostgreSQL.

## Спецификация эндпоинта

**POST** `/auth/register`

### Request Body (JSON)

```json
{
  "username": "string",      // обязательно, 1-64 символа
  "email": "string",         // обязательно, валидный email, макс. 255
  "phone": "string",         // обязательно, макс. 32 символа
  "password": "string",      // обязательно, 8-20 символов, минимум 1 заглавная + 1 строчная + 1 цифра
  "role": "string",          // опционально: individual | logistics_partner | business_account (по умолчанию individual)
  "company_name": "string",  // обязательно для role=business_account, макс. 256
  "inn": "string",           // опционально, 10 или 12 цифр
  "metadata": "object"       // опционально, макс. 4KB (JSON-объект)
}

-----------------------------------------------------------------------------------------------------
 Примеры запросов:

1. Физическое лицо (role не указан → individual):
{
  "username": "ivan_petrov",
  "email": "ivan@example.com",
  "phone": "+79001234567",
  "password": "SecurePass1"
}


2. Компания-партнер (business_account):
{
  "username": "ooo_stroymaterial",
  "email": "director@stroymaterial.ru",
  "phone": "+74951234567",
  "password": "CompanyPass1",
  "role": "business_account",
  "company_name": "ООО СтройМатериал",
  "inn": "7712345678"
}

-----------------------------------------------------------------------------------------------------
Успешный ответ (200 OK)
{
  "success": true,
  "message": "User registered successfully.",
  "user": {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "username": "ivan_petrov",
    "email": "ivan@example.com",
    "role": "individual"
  }
}

-----------------------------------------------------------------------------------------------------

Ошибки

| HTTP статус | Код ошибки              | Описание                                                                 |
|-------------|-------------------------|--------------------------------------------------------------------------|
| 400         | `validation_error`      | Невалидные поля (неверный формат, длина, требования к паролю и т.д.)     |
| 400         | `registration_failed`   | Пользователь с таким username или email уже существует                   |
| 403         | `registration_forbidden`| Попытка использовать запрещённую роль (например `admin` и другие)        |
| 500         | `internal_error`        | Внутренняя ошибка сервера                                                |

-----------------------------------------------------------------------------------------------------
Технические требования:
- Идемпотентность регистрации | Повторный запрос с теми же данными не должен создавать дубликат | корректная обработка уже существующего пользователя
- Хеширование пароля | bcrypt или argon2 | хранить только хеш
- Обязательная защита от SQL-инъекций → только параметризованные запросы
- Обработка ошибок подключения к БД и выполнения запросов
- Чтение конфигурации из **переменных окружения** (или аргументов командной строки):
  - `DB_HOST`
  - `DB_PORT`
  - `DB_NAME`
  - `DB_USER`
  - `DB_PASSWORD`
  - `SERVER_PORT`
- Желательно наличие хотя бы минимального логирования (stdout или spdlog / fmtlog и т.п.)

-----------------------------------------------------------------------------------------------------
База данных:

CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(64) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    phone VARCHAR(32) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(32) NOT NULL DEFAULT 'individual',
    company_name VARCHAR(256),
    inn VARCHAR(12),
    metadata JSONB,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

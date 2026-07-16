#define _CRT_SECURE_NO_WARNINGS
#include "database.h"
#include <libpq-fe.h>     
#include <iostream>
#include <cstdlib>        
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace std;

// Глобальное соединение с БД (одно на всё время работы сервера)
static PGconn* g_conn = nullptr;

// Возвращает значение переменной окружения или значение по умолчанию
static string getEnvOrDefault(const string& var, const string& def) {
    const char* val = getenv(var.c_str());
    return val ? string(val) : def;
}

// Формирует строку подключения к PostgreSQL из переменных окружения
static string getConnectionString() {
    string host = getEnvOrDefault("DB_HOST", "localhost");
    string port = getEnvOrDefault("DB_PORT", "5432");
    string dbname = getEnvOrDefault("DB_NAME", "mydb");
    string user = getEnvOrDefault("DB_USER", "postgres");
    string password = getEnvOrDefault("DB_PASSWORD", "Fallout1803");
    return "host=" + host + " port=" + port + " dbname=" + dbname +
        " user=" + user + " password=" + password;
}

// Простая функция логирования с меткой времени
static void logInfo(const string& msg) {
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    cout << put_time(localtime(&now), "[%Y-%m-%d %H:%M:%S]") << " [INFO] " << msg << endl;
}

static void logError(const string& msg) {
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    cerr << put_time(localtime(&now), "[%Y-%m-%d %H:%M:%S]") << " [ERROR] " << msg << endl;
}

// Инициализация подключения к БД (вызывается один раз при старте)
bool initDatabase() {
    string connStr = getConnectionString();
    logInfo("Connecting to PostgreSQL: " + connStr);
    g_conn = PQconnectdb(connStr.c_str());

    if (PQstatus(g_conn) != CONNECTION_OK) {
        logError("Connection failed: " + string(PQerrorMessage(g_conn)));
        PQfinish(g_conn);
        g_conn = nullptr;
        return false;
    }
    logInfo("Connected to PostgreSQL successfully!");
    return true;
}

// Закрытие соединения (вызывается при завершении программы)
void closeDatabase() {
    if (g_conn) {
        PQfinish(g_conn);
        g_conn = nullptr;
        logInfo("Database connection closed.");
    }
}

// Проверка наличия пользователя с заданным username или email
bool userExists(const string& username, const string& email) {
    if (!g_conn || PQstatus(g_conn) != CONNECTION_OK) {
        logError("No database connection in userExists");
        return true; 
    }

    // Параметры подготовленного запроса
    const char* paramValues[2] = { username.c_str(), email.c_str() };
    int paramLengths[2] = { (int)username.size(), (int)email.size() };
    int paramFormats[2] = { 0, 0 };

    // Выполнение запроса с параметрами
    PGresult* res = PQexecParams(g_conn,
        "SELECT 1 FROM users WHERE username = $1 OR email = $2",
        2, nullptr, paramValues, paramLengths, paramFormats, 0
    );

    bool exists = false;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        exists = PQntuples(res) > 0;
    }
    else {
        logError("userExists query failed: " + string(PQresultErrorMessage(res)));
    }

    PQclear(res);
    return exists;
}

// Сохранение нового пользователя в БД
string saveUser(const Valid& user, const string& password_hash) {
    if (!g_conn || PQstatus(g_conn) != CONNECTION_OK) {
        logError("No database connection in saveUser");
        return "";
    }

    // Извлекаем данные через геттеры
    string username = user.getUsername();
    string email = user.getEmail();
    string phone = user.getPhone();
    string role = user.getRole();
    string company_name = user.getCompanyName();
    string inn = user.getInn();
    string metadata = user.getMetadata().dump(); 

    // Если поля пустые, передаём nullptr (в БД попадёт NULL)
    const char* company_name_ptr = company_name.empty() ? nullptr : company_name.c_str();
    const char* inn_ptr = inn.empty() ? nullptr : inn.c_str();

    // Массивы для параметризованного запроса
    const char* paramValues[8] = {
        username.c_str(),
        email.c_str(),
        phone.c_str(),
        password_hash.c_str(),
        role.c_str(),
        company_name_ptr,
        inn_ptr,
        metadata.c_str()
    };
    int paramLengths[8] = {
        (int)username.size(),
        (int)email.size(),
        (int)phone.size(),
        (int)password_hash.size(),
        (int)role.size(),
        company_name.empty() ? 0 : (int)company_name.size(),
        inn.empty() ? 0 : (int)inn.size(),
        (int)metadata.size()
    };
    int paramFormats[8] = { 0 };

    // Вставка записи и возврат сгенерированного UUID
    PGresult* res = PQexecParams(g_conn,
        "INSERT INTO users (username, email, phone, password_hash, role, company_name, inn, metadata) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
        8, nullptr, paramValues, paramLengths, paramFormats, 0
    );

    string userId;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        userId = PQgetvalue(res, 0, 0); 
        logInfo("User saved with ID: " + userId);
    }
    else {
        logError("Insert failed: " + string(PQresultErrorMessage(res)));
    }

    PQclear(res);
    return userId;
}
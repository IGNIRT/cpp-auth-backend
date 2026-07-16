#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>

#include "httplib.h"
#include <nlohmann/json.hpp>
#include "valid.h"
#include "database.h"
#include <argon2.h>

using namespace std;

// Вывод информационного сообщения с меткой времени
static void logInfo(const string& msg) {
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    cout << put_time(localtime(&now), "[%Y-%m-%d %H:%M:%S]") << " [INFO] " << msg << endl;
}

// Вывод сообщения об ошибке с меткой времени
static void logError(const string& msg) {
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    cerr << put_time(localtime(&now), "[%Y-%m-%d %H:%M:%S]") << " [ERROR] " << msg << endl;
}

// Получение значения переменной окружения или значения по умолчанию
static string getEnvOrDefault(const string& var, const string& def) {
    const char* val = getenv(var.c_str());
    return val ? string(val) : def;
}

// Вспомогательные функции для Argon2
vector<uint8_t> generateSalt(size_t length = 16) {
    vector<uint8_t> salt(length);
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < length; ++i)
        salt[i] = static_cast<uint8_t>(dist(rng));
    return salt;
}

string toHex(const vector<uint8_t>& data) {
    stringstream ss;
    ss << hex << setfill('0');
    for (uint8_t byte : data)
        ss << setw(2) << static_cast<int>(byte);
    return ss.str();
}

vector<uint8_t> fromHex(const string& hex) {
    vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Хеширование пароля алгоритмом Argon2id
string hashPassword(const string& password) {
    vector<uint8_t> salt = generateSalt(16);
    vector<uint8_t> hash(32);

    int result = argon2id_hash_raw(
        3,               // количество итераций
        65536,           // использование памяти (64 МБ)
        1,               // однопоточность
        password.c_str(), password.size(),
        salt.data(), salt.size(),
        hash.data(), hash.size()
    );

    if (result != ARGON2_OK)
        throw runtime_error("Argon2 hashing failed");

    return toHex(salt) + ":" + toHex(hash);  
}

// Проверка пароля по сохранённому хешу
bool verifyPassword(const string& password, const string& stored_hash) {
    size_t colon_pos = stored_hash.find(':');
    if (colon_pos == string::npos)
        throw runtime_error("Invalid stored hash format");

    string salt_hex = stored_hash.substr(0, colon_pos);
    string hash_hex = stored_hash.substr(colon_pos + 1);

    vector<uint8_t> salt = fromHex(salt_hex);
    vector<uint8_t> new_hash(32);

    int result = argon2id_hash_raw(
        3, 65536, 1,
        password.c_str(), password.size(),
        salt.data(), salt.size(),
        new_hash.data(), new_hash.size()
    );
    if (result != ARGON2_OK)
        throw runtime_error("Argon2 verification failed");

    return toHex(new_hash) == hash_hex;
}

int main() {
    // Инициализация подключения к БД
    if (!initDatabase()) {
        logError("Failed to connect to database. Exiting...");
        return 1;
    }

    httplib::Server svr;

    // Регистрируем обработчик POST-запроса /auth/register
    svr.Post("/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // 1. Парсинг JSON из тела запроса
            auto json = nlohmann::json::parse(req.body);
            Valid user(json);

            // 2. Валидация полей
            Result result = user.validate();
            if (!result.success) {
                if (result.error_code == "registration_forbidden")
                    res.status = 403;   
                else
                    res.status = 400; 

                nlohmann::json err = {
                    {"error", result.error_code},
                    {"message", result.message}
                };
                res.set_content(err.dump(), "application/json");
                return;
            }

            // 3. Проверка уникальности (идемпотентность)
            if (userExists(user.getUsername(), user.getEmail())) {
                res.status = 400;
                nlohmann::json err = {
                    {"error", "registration_failed"},
                    {"message", "A user with this username or email already exists"}
                };
                res.set_content(err.dump(), "application/json");
                return;
            }

            // 4. Хеширование пароля
            string password_hash = hashPassword(user.getPassword());
            logInfo("Hashed password for user: " + user.getUsername());

            // 5. Сохранение пользователя в БД
            string user_id = saveUser(user, password_hash);
            if (user_id.empty()) {
                res.status = 500;
                nlohmann::json err = {
                    {"error", "internal_error"},
                    {"message", "Failed to save user"}
                };
                res.set_content(err.dump(), "application/json");
                return;
            }

            // 6. Успешный ответ
            res.status = 200;  
            nlohmann::json user_json = user.toResponseJson();
            user_json["id"] = user_id;   

            nlohmann::json response = {
                {"success", true},
                {"message", "User registered successfully."},
                {"user", user_json}
            };
            res.set_content(response.dump(), "application/json");

        }
        catch (const exception& e) {
            // Любая неожиданная ошибка
            logError("Exception in /auth/register: " + string(e.what()));
            res.status = 500;
            nlohmann::json err = {
                {"error", "internal_error"},
                {"message", string("Internal server error: ") + e.what()}
            };
            res.set_content(err.dump(), "application/json");
        }
        });

    // Чтение порта из переменной окружения или значение по умолчанию 8090
    int server_port = stoi(getEnvOrDefault("SERVER_PORT", "8090"));
    logInfo("Starting server on port " + to_string(server_port));

    if (!svr.listen("0.0.0.0", server_port)) {
        logError("Failed to start server");
        closeDatabase();
        return 1;
    }

    logInfo("Server stopped.");
    closeDatabase();
    return 0;
}
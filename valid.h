#pragma once
#include <string>
#include <nlohmann/json.hpp>

using namespace std;

// Структура для возврата результата валидации
struct Result {
    bool success = true;   // true — проверка пройдена, false — есть ошибка
    string error_code;     // Код ошибки (например "validation_error")
    string message;       
};

// Класс для хранения и проверки данных, пришедших в запросе регистрации
class Valid {
private:
    string username_;
    string email_;
    string phone_;
    string password_;
    string role_;
    string company_name_;
    string inn_;
    nlohmann::json metadata_;

public:

    explicit Valid(const nlohmann::json& json);

    string getUsername() const { return username_; }
    string getEmail() const { return email_; }
    string getPhone() const { return phone_; }
    string getPassword() const { return password_; }
    string getRole() const { return role_; }
    string getCompanyName() const { return company_name_; }
    string getInn() const { return inn_; }
    nlohmann::json getMetadata() const { return metadata_; }

    // Проверка всех полей по заданным правилам
    Result validate() const;

    // Формирует JSON-ответ с публичными данными (без пароля и телефона)
    nlohmann::json toResponseJson() const;
};
#define _CRT_SECURE_NO_WARNINGS
#include "valid.h"
#include <regex>
#include <cctype>   

using namespace std;

// Конструктор: извлекает данные из JSON, подставляя значения по умолчанию для отсутствующих полей
Valid::Valid(const nlohmann::json& json) {
    username_ = json.value("username", "");
    email_ = json.value("email", "");
    phone_ = json.value("phone", "");
    password_ = json.value("password", "");
    role_ = json.value("role", "individual");               
    company_name_ = json.value("company_name", "");
    inn_ = json.value("inn", "");
    metadata_ = json.value("metadata", nlohmann::json::object()); 
}

// Основной метод проверки всех полей
Result Valid::validate() const {
    // 1. Проверка username: обязательный, длина 1–64 символа
    if (username_.empty() || username_.size() > 64) {
        return { false, "validation_error", "Username must be 1-64 characters" };
    }

    // 2. Проверка email: обязательный, длина до 255, соответствие формату
    if (email_.empty() || email_.size() > 255) {
        return { false, "validation_error", "Email must be 1-255 characters" };
    }
    regex email_pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!regex_match(email_, email_pattern)) {
        return { false, "validation_error", "Invalid email format" };
    }

    // 3. Проверка телефона: обязательный, длина до 32 символов
    if (phone_.empty() || phone_.size() > 32) {
        return { false, "validation_error", "Phone must be 1-32 characters" };
    }

    // 4. Проверка пароля: длина 8-20 символов, минимум 1 заглавная, 1 строчная, 1 цифра
    if (password_.size() < 8 || password_.size() > 20) {
        return { false, "validation_error", "Password must be 8-20 characters" };
    }
    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (char c : password_) {
        if (isupper(c)) hasUpper = true;
        if (islower(c)) hasLower = true;
        if (isdigit(c)) hasDigit = true;
    }
    if (!hasUpper || !hasLower || !hasDigit) {
        return { false, "validation_error", "Password must contain uppercase, lowercase, and digit" };
    }

    // 5. Проверка роли: только три допустимых значения
    if (role_ != "individual" && role_ != "logistics_partner" && role_ != "business_account") {
        return { false, "registration_forbidden", "Invalid role. Allowed: individual, logistics_partner, business_account" };
    }

    // 6. Для business_account обязательны company_name и ИНН
    if (role_ == "business_account") {
        if (company_name_.empty()) {
            return { false, "validation_error", "Company name is required for business_account" };
        }
        if (company_name_.size() > 256) {
            return { false, "validation_error", "Company name must be 1-256 characters" };
        }
        if (inn_.empty()) {
            return { false, "validation_error", "INN is required for business_account" };
        }
    }

    // 7. Проверка ИНН (если заполнен) – ровно 10 или 12 цифр
    if (!inn_.empty()) {
        if (inn_.size() != 10 && inn_.size() != 12) {
            return { false, "validation_error", "INN must be 10 or 12 digits" };
        }
        for (char c : inn_) {
            if (!isdigit(c)) {
                return { false, "validation_error", "INN must contain only digits" };
            }
        }
    }

    // 8. Проверка metadata: должен быть объектом, размер не более 4 КБ
    if (!metadata_.is_null()) {
        if (!metadata_.is_object()) {
            return { false, "validation_error", "Metadata must be a JSON object" };
        }
        if (metadata_.dump().size() > 4096) {
            return { false, "validation_error", "Metadata must not exceed 4KB" };
        }
    }

    // Все проверки пройдены
    return { true, "", "" };
}

// Возвращает публичные данные пользователя 
nlohmann::json Valid::toResponseJson() const {
    nlohmann::json j;
    j["username"] = username_;
    j["email"] = email_;
    j["role"] = role_;
    if (role_ == "business_account") {
        j["company_name"] = company_name_;
        j["inn"] = inn_;
    }
    return j;
}
#pragma once
#include <string>
#include "valid.h"
using namespace std;
// Подключение к базе данных (читает параметры из переменных окружения)
bool initDatabase();

// Закрытие соединения с БД
void closeDatabase();

// Проверка, существует ли пользователь с таким username или email
bool userExists(const string& username, const string& email);

// Сохранение пользователя в БД, возвращает его UUID или пустую строку при ошибке
string saveUser(const Valid& user, const string& password_hash);
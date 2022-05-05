#ifndef PERSON_H
#define PERSON_H
#include <QDebug>
#include <QFile>

struct Bank{
    QString name;
    int bankAccount;
    int moneyValue;
};

struct People{
    int id;
    QString userName;
    QString name;
    QString surname;
    QString password;
    Bank banka;
};

class Person
{
public:
    People *database();

};

#endif // PERSON_H

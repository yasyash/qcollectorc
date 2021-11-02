#ifndef GLOBAL_H
#define GLOBAL_H

#include <QHash>
#include <QMutex>

QMutex* _rest_mutex;

const QHash<QString, QString> aspiap_dir = {
    {"Пыль общая", "P001"},
    {"PM1", "PM1"},
    {"PM2.5", "P301"},
    {"PM10", "P201"},
    {"NO2", "P005"},
    {"NO", "P006"},
    {"NH3", "P019"},
    {"бензол", "P028"},
    {"HF", "P030"},
    {"HCl", "P015"},
    {"м,п-ксилол", "м,п-ксилол"},
    {"о-ксилол", "о-ксилол"},
    {"O3", "P007"},
    {"H2S", "P008"},
    {"SO2", "P002"},
    {"стирол", "P068"},
    {"толуол", "P071"},
    {"CO", "P004"},
    {"фенол", "P010"},
    {"CH2O", "P022"},
    {"хлорбензол", "P077"},
    {"этилбензол", "P083"}
};

#endif // GLOBAL_H

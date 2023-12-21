//
// Created by Ярослав on 20.12.2023.
//
#include <map>
#include <iostream>
#include <cassert>

std::map<std::string, std::string> initRu(){
    std::map<std::string, std::string> m;

    m["L_OFF"] = "Декодер Отключен(Не слушает радио)";
    m["L_RAW"] = "Прослушивает Радио в RAW - Режиме.";
    m["L_WFM_S"] = "Прослушивает Радио в FM(Stereo) - Режиме.";
    m["L_NFM"] = "Прослушивает Радио в NarrowFM - Режиме.";
    m["L_WFM"] = "Прослушивает Радио в FM - Режиме.";
    m["L_AMSYNC"] = "Прослушивает Радио в AMSync - Режиме.";
    m["L_AM"] = "Прослушивает Радио в AM - Режиме.";
    m["L_OTHER"] = "Прослушивает радио-частоту:";

    m["RDS_EMPTY"] = "Пусто (Сигнал не устойчивый)";
    m["G_OFF"] = "Приёмник Выключен(Не слушает радио)";

    return m;
}

std::map<std::string, std::string> initEn(){
    std::map<std::string, std::string> m;

    m["L_OFF"] = "Decoder is off.";
    m["L_RAW"] = "Listening radio in RAW mode.";
    m["L_WFM_S"] = "Listening radio in FM(Stereo) mode.";
    m["L_NFM"] = "Listening radio in NarrowFM mode.";
    m["L_WFM"] = "Listening radio in FM mode.";
    m["L_AMSYNC"] = "Listening radio in AMSync mode";
    m["L_AM"] = "Listening radio in AM mode";
    m["L_OTHER"] = "Listening radio:";

    m["RDS_EMPTY"] = "Empty (Signal not stable)";
    m["G_OFF"] = "Radio/SDR off";

    return m;
}
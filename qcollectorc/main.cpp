#include <QCoreApplication>
#include <QHttpMultiPart>
#include <QDataStream>

#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlField>
#include <QDateTime>
#include <QSharedMemory>

#include "../requester/requester.h"
#include "global.h"

extern const QHash<QString, QString> aspiap_dir;

void funcSuccess( QJsonObject _resp, QString uri, QDateTime _date_time, QDateTime _last_time, int _msg_id, QSqlDatabase * m_conn, QString idd)
{
    QDateTime _now = QDateTime::currentDateTime();
    bool _result = _resp.value("success").toBool();
    if (_result)
    {
        QString _update = QString("UPDATE injection set date_time = '"+ _last_time.toString("yyyy-MM-ddTHH:mm:ss") +
                                  "', last_time = '"+ _date_time.toString("yyyy-MM-ddTHH:mm:ss")+"', msg_id_out = "+
                                  QString::number(_msg_id) + ", msg_id = 0 WHERE idd = '" + idd +"'");

        QSqlQuery *query_inj = new QSqlQuery(*m_conn);

        query_inj->prepare(_update);
        query_inj->exec();

        if(!query_inj ->lastError().isValid())
        {
            query_inj->finish();

            QString _insert = QString("INSERT INTO injection_logs (date_time, transaction, uri, request_time, response_time, is_ok, errors, msg_id_out) VALUES ('"+
                                      _last_time.toString("yyyy-MM-ddTHH:mm:ss")+"', '"+_resp.value("transaction").toString()+ "','" + uri + "','"+
                                      _date_time.toString("yyyy-MM-ddTHH:mm:ss")+"', "+QString::number(_now.toMSecsSinceEpoch() -  _date_time.toMSecsSinceEpoch())+
                                      +",'true', '', " + QString::number(_msg_id)  +")");
            query_inj->prepare(_insert);
            query_inj->exec();
            query_inj->finish();

            _insert = QString("INSERT INTO injected (date_time, msg_id, uri, transaction, msg_time, idd, msg_id_out) VALUES ('"+
                              _last_time.toString("yyyy-MM-ddTHH:mm:ss")+"'," +QString::number(_msg_id) +",'" + uri + "','"+_resp.value("transaction").toString()+ "', '"+
                              _date_time.toString("yyyy-MM-ddTHH:mm:ss")+"', '"+ idd + "', " + QString::number(_msg_id)  +")");
            query_inj->prepare(_insert);
            query_inj->exec();

        }
        query_inj->finish();
        QTextStream(stdout) << "Success: Time is "+ _last_time.toString("yyyy-MM-dd HH:mm:ss") + " ID message = "  + QString::number(_msg_id) + " \n\r";

    } else {

        QString _update = QString("UPDATE injection SET  last_time = '"+ _date_time.toString("yyyy-MM-ddTHH:mm:ss") + "', msg_id = " + QString::number(_msg_id) +" WHERE idd = '" + idd +"'");

        QSqlQuery *query_inj = new QSqlQuery(*m_conn);

        query_inj->prepare(_update);
        query_inj->exec();

        /* if(!query_inj ->lastError().isValid())
        {
            query_inj->finish();

            QString _insert = QString("INSERT INTO injection_logs (date_time, transaction, request_time, response_time, is_ok, errors, msg_id_out) VALUES ('"+
                                      _last_time.toString("yyyy-MM-ddTHH:mm:ss")+"', '"+_resp.value("transaction").toString()+ "', '"+
                                      _date_time.toString("yyyy-MM-ddTHH:mm:ss")+"', "+QString().arg(_now.toMSecsSinceEpoch() -  _date_time.toMSecsSinceEpoch())+
                                      +"'false'," +_resp.value("error").toString() + "', " + QString().arg(_msg_id)  +")");
            query_inj->prepare(_insert);
            query_inj->exec();
        }*/
        query_inj->finish();
        QTextStream(stdout) << "!!! Error: Time is "+ _date_time.toString("yyyy-MM-dd HH:mm:ss") + " ID message = "  + QString::number(_msg_id) + " \n\r" +
                               " Reason: " +  _resp.value("error").toString() + " \n\r Response time (ms):  "+ QString::number(_now.toMSecsSinceEpoch() -  _date_time.toMSecsSinceEpoch())+" ms\n\r";



    }

}

void funcError(QJsonObject _resp, QString uri, QDateTime _date_time, QDateTime _last_time, int _msg_id, QSqlDatabase * m_conn, QString idd)
{

    /* if(!query_inj ->lastError().isValid())
    {
        query_inj->finish();

        QString _insert = QString("INSERT INTO injection_logs (date_time, transaction, request_time, response_time, is_ok, errors, msg_id_out) VALUES ('"+
                                  _last_time.toString("yyyy-MM-ddTHH:mm:ss")+"', '"+_resp.value("transaction").toString()+ "', '"+
                                  _date_time.toString("yyyy-MM-ddTHH:mm:ss")+"', "+QString().arg(_now.toMSecsSinceEpoch() -  _date_time.toMSecsSinceEpoch())+
                                  +"'false'," +_resp.value("error").toString() + "', " + QString().arg(_msg_id)  +")");
        query_inj->prepare(_insert);
        query_inj->exec();
    }*/
    QTextStream(stdout) << "!!! Error: Time is "+ _date_time.toString("yyyy-MM-dd HH:mm:ss") + " \n\r" +
                           " Reason caught: " +  _resp.value("QNetworkReply").toString() + " \n\r";

}

void fetch_data (QSqlDatabase * m_conn, QSharedMemory *_shared, QString idd, QDateTime _from_t, QDateTime _to_t, QDateTime _last_t, QString uri, int code, QString token, QString locality, double msg_id, int msg_id_out)

{
    qint64 i = 0;
    QString select_eq = QString("SELECT * FROM equipments WHERE is_present = true and idd = '" + idd +"'");
    QSqlQuery *query_eq = new QSqlQuery(*m_conn);
    QSqlRecord rs_sensor;
    QDateTime _begin_t = _from_t;

    query_eq->prepare(select_eq);
    query_eq->exec();

    QString select_data = QString("SELECT * FROM sensors_data WHERE idd ='").append(idd).append("' AND date_time between '" + _from_t.toString("yyyy-MM-ddTHH:mm:ss")+"' AND '" + _to_t.toString("yyyy-MM-ddTHH:mm:ss") +"' order by date_time ASC");
    QSqlQuery *query_data = new QSqlQuery(*m_conn);

    query_data->prepare(select_data);
    query_data->exec();

    bool _go_out = true;
    QSqlRecord rs_data;

    if(!query_data->lastError().isValid())
    {
        if (query_data->size() > 0)
        {
            _go_out = false;
            query_data->first();
            rs_data = query_data->record();
            QDateTime _first_time = QDateTime::fromString(rs_data.field("date_time").value().toString().left(19),"yyyy-MM-ddTHH:mm:ss");
            if (_first_time > _from_t)
                _begin_t = _first_time;
        }
    } else {
        _go_out = false;
    }

    //begin work
    if(!query_eq->lastError().isValid())
    {
        select_data = QString("SELECT * FROM sensors_data WHERE date_time between :_from AND :_to AND serialnum = :_serialnum ORDER BY date_time ASC");

        while (!_go_out) {

            QDateTime _from_t_local = _begin_t.addSecs(i*60);
            QDateTime _to_t_local = _begin_t.addMSecs(i*60000+59999 ); //add 999 mSec for 'between' sql frame
            if (_to_t_local > _to_t)
                _go_out = true;

            QJsonArray _params;

            // query_eq->first();

            while (query_eq->next()) {
                rs_sensor = query_eq->record();
                if (!rs_sensor.field("typemeasure").value().toString().contains(QString("Напряжение")) && !rs_sensor.field("typemeasure").value().toString().contains(QString("ИБП")))
                {
                    query_data->finish();

                    query_data->prepare(select_data);
                    query_data->bindValue(":_from", _from_t_local.toString("yyyy-MM-ddTHH:mm:ss"));
                    query_data->bindValue(":_to", _to_t_local.toString("yyyy-MM-ddTHH:mm:ss"));
                    query_data->bindValue(":_serialnum", rs_sensor.field("serialnum").value().toString());
                    QString _t =  rs_sensor.field("serialnum").value().toString();
                    query_data->exec();
                    //query_data->first();
                    if(!query_data->lastError().isValid())
                    {
                        if (query_data->size() > 0){
                            QJsonObject _data_one;
                            double _measure = 0.0;
                            while (query_data->next()) {
                                rs_data = query_data->record();
                                _measure += rs_data.field("measure").value().toDouble();
                            }
                            _data_one = {{"date_time",rs_data.field("date_time").value().toString().append(QString("+").append((QDateTime::currentDateTime().offsetFromUtc()/3600 < 10) ? (QString("0").append(QString::number( QDateTime::currentDateTime().offsetFromUtc()/3600))) : (QString::number(QDateTime::currentDateTime().offsetFromUtc()/3600 ))))},{"unit",rs_sensor.field("unit_name").value().toString()},{"measure", _measure/query_data->size()}};
                            QJsonObject _chemical_one = {{aspiap_dir.value(rs_sensor.field("typemeasure").value().toString()),_data_one}};
                            _params.append(QJsonValue( _chemical_one));
                        }
                    }
                }
            }


            if (_params.size() > 0){
                Requester *_req = new Requester();

                _req->initRequester(uri,443, nullptr);

                QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
                QHttpPart _http_form;
                QByteArray _body_tmp;

                QDateTime _now = QDateTime::currentDateTime();

                QJsonObject _header = {{"token", token},{"message",QString().number( (msg_id_out+=1))},{"locality", locality},{"object", code},{"date_time", _now.toString("yyyy-MM-dd HH:mm:ss").append(QString("+").append((QDateTime::currentDateTime().offsetFromUtc()/3600 < 10) ? (QString("0").append(QString::number( QDateTime::currentDateTime().offsetFromUtc()/3600))) : (QString::number(QDateTime::currentDateTime().offsetFromUtc()/3600 ))))},
                                       {"params", _params}};

                QJsonDocument doc(_header);
                _body_tmp = doc.toJson();


                _http_form.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data;  name=\"data\""));
                _http_form.setBody(_body_tmp);

                multiPart->append(_http_form);
                multiPart->setBoundary("---");

                //try to send request

                _req->sendRequest(&funcSuccess, &funcError,Requester::Type::POST,multiPart, uri, _now,_from_t_local,msg_id_out,m_conn, idd);

            }
            query_eq->exec();
            i++;
        }


    }
    QTextStream(stdout) << "The collector process is completed...\n\r";
    _shared->detach();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QDateTime local(QDateTime::currentDateTime());
    QDateTime UTC(local.toUTC());
    qint64 tz = QDateTime::currentDateTime().offsetFromUtc()/3600; //time zone
    QString o = QString("+").append((QDateTime::currentDateTime().offsetFromUtc()/3600 < 10) ? (QString("0").append(QString::number( QDateTime::currentDateTime().offsetFromUtc()/3600))) : (QString::number(QDateTime::currentDateTime().offsetFromUtc()/3600 )));

    QSharedMemory _shared("77777777-3333-7777-3333-4dafd2077c46");

     QTextStream(stdout) << "The collector application (version 1.0) is started...\n\r";

    if( !_shared.create( 512, QSharedMemory::ReadWrite) )
    {
        QTextStream(stdout) << "Can't start more than one instance of the application...\n\r";
        exit(-1);
    }

    QString db = QString(argv[1]);
    if (db == "")
    {
        // releaseModbus();

        QTextStream(stdout) << "Fatal error: wrong data of the database parameter\n\r";
        _shared.detach();
        exit(-1);

    }

    QString user = QString(argv[2]);
    if (user == "")
    {
        // releaseModbus();

        QTextStream(stdout) << "Fatal error: wrong data of the user parameter\n\r";
        _shared.detach();
        exit(-1);

    }

    QString pw = QString(argv[3]);
    if (pw == "")
    {
        QTextStream(stdout) << "Fatal error: wrong data of the password parameter\n\r";
        _shared.detach();
        exit(-1);

    }

    QSqlDatabase * m_conn = new QSqlDatabase();
    *m_conn = QSqlDatabase::addDatabase("QPSQL");
    m_conn->setHostName("localhost");
    m_conn->setDatabaseName(db);
    m_conn->setUserName(user);
    m_conn->setPassword(pw);

    bool status = m_conn->open();

    if (!status)
    {
        QTextStream(stdout) << ( QString("Connection fatal error: " + m_conn->lastError().text()).toLatin1().constData()) <<   " \n\r";
        _shared.detach();
        exit(-1);

    }

    QString select_st = "SELECT * FROM injection WHERE is_present = true and id = 2";
    QSqlQuery *query = new QSqlQuery(*m_conn);
    QSqlRecord rs_station;

    query->prepare(select_st);
    query->exec();

    //begin work
    if(!query->lastError().isValid())
    {
        while (query->next()) {
            rs_station = query->record();

            QString uri = QString(rs_station.value("uri").toString());
            QString token = QString(rs_station.value("token").toString());
            //QString locality = QString(rs_station.value("locality").toString());
            QString indx = QString(rs_station.value("indx").toString());
            QString idd = QString(rs_station.value("idd").toString());
            int code = rs_station.value("code").toInt();
            double msg_id = rs_station.value("msg_id").toDouble();
            int msg_id_out = rs_station.value("msg_id_out").toInt();


            QString begin_t = QString(rs_station.value("date_time").toString());
            QDateTime _begin_t = QDateTime::fromString(begin_t.left(19),"yyyy-MM-ddTHH:mm:ss");

            QString last_t = QString(rs_station.value("last_time").toString());
            QDateTime _last_t = QDateTime::fromString(last_t.left(19),"yyyy-MM-ddTHH:mm:ss");

            QString curr_t = QDateTime(QDateTime::currentDateTime()).toString("yyyy-MM-ddThh:mm:ss");
            QDateTime _curr_t = QDateTime::fromString(curr_t.left(19),"yyyy-MM-ddTHH:mm:ss");

            QDateTime _from_t = _begin_t.addSecs(1);

            if (_curr_t.addDays(-1) > _begin_t)
                _from_t = _curr_t.addDays(-1);

            fetch_data(m_conn, &_shared, idd, _from_t, _curr_t.addSecs(1), _last_t, uri, code, token,  indx, msg_id, msg_id_out);

        }
    } else {
        _shared.detach(); //we should detach memory for another instance
    }

    //return a.exec();
}


/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtTest/QtTest>

#include <vector>
#include <cstring>
#include "io/aep/gradient_xml.hpp"

using namespace glaxnimate::io::aep;


#define COS_VALUE(val, type_index, expected) \
    QCOMPARE(val.type(), CosValue::Index::type_index); \
    QCOMPARE(val.get<CosValue::Index::type_index>(), (expected));


class TestCase: public QObject
{
    Q_OBJECT

    void compare(QString error)
    {
        QCOMPARE(error, "0:0: ");
    }

    QDomDocument xml(QString xml)
    {
        QDomDocument dom;
        QString error;
        int line = 0, col = 0;
        dom.setContent(xml.trimmed(), false, &error, &line, &col);
        if ( error != "" )
        {
            auto msg = QString("%1:%2: %3").arg(line).arg(col).arg(error);
            auto lines = xml.split("\n");
            int li = line - 1;
            if ( li >= 0 && li < lines.size() )
            {
                msg += "\n" + lines[li];
                msg += "\n" + QString(col-2, ' ') + '^';
            }
            std::string stdmsg = msg.toStdString();
            QTest::qFail(stdmsg.c_str(), "xml", line);
        }
        return dom;
    }

    QDomElement element(QString xml)
    {
        return this->xml(xml).documentElement();
    }


private slots:
    void test_xml_value_scalar()
    {
        COS_VALUE(xml_value(element("<float>2.5</float>")), Number, 2.5);
        COS_VALUE(xml_value(element("<int>25</int>")), Number, 25);
        COS_VALUE(xml_value(element("<string>2.5</string>")), String, "2.5");
    }

    void test_xml_array()
    {
        auto arr = xml_array(element(R"(
            <array>
                <array.type>
                    <int/>
                </array.type>
                <int>1</int>
                <int>2</int>
                <int>3</int>
            </array>
        )"));
        QCOMPARE(arr->size(), 3);
        COS_VALUE(arr->at(0), Number, 1);
        COS_VALUE(get_value(arr, 1), Number, 2);
        COS_VALUE(get_value(arr, 2), Number, 3);

        CosValue val(std::move(arr));
        COS_VALUE(get(val, 0), Number, 1);
        COS_VALUE(get_value(val, 1), Number, 2);
        COS_VALUE(get(val, 2), Number, 3);
    }

    void test_xml_list()
    {

        auto list = xml_list(element(R"(
            <prop.list>
                <prop.pair><key>a</key><int>1</int></prop.pair>
                <prop.pair><key>b</key><int>2</int></prop.pair>
                <prop.pair><key>c</key><int>3</int></prop.pair>
                <prop.pair><key>d</key><int>4</int></prop.pair>
            </prop.list>
        )"));
        QCOMPARE(list->size(), 4);
        COS_VALUE(list->at("a"), Number, 1);
        COS_VALUE(list->at("b"), Number, 2);
        COS_VALUE(get_value(list, "c"), Number, 3);
        COS_VALUE(get_value(list, "d"), Number, 4);

        CosValue list_val(std::move(list));
        COS_VALUE(get(list_val, "a"), Number, 1);
        COS_VALUE(get_value(list_val, "b"), Number, 2);
        COS_VALUE(get(list_val, "c"), Number, 3);
        COS_VALUE(get_value(list_val, "d"), Number, 4);
    }

    void test_xml_value_array()
    {
        auto val = xml_value(element(R"(
            <array>
                <array.type>
                    <int/>
                </array.type>
                <int>1</int>
                <int>2</int>
                <int>3</int>
            </array>
        )"));

        QCOMPARE(val.type(), CosValue::Index::Array);
        COS_VALUE(get(val, 0), Number, 1);
        COS_VALUE(get_value(val, 1), Number, 2);
        COS_VALUE(get(val, 2), Number, 3);
    }

    void test_xml_value_list()
    {
        auto list = xml_value(element(R"(
            <prop.list>
                <prop.pair><key>a</key><int>1</int></prop.pair>
                <prop.pair><key>b</key><int>2</int></prop.pair>
                <prop.pair><key>c</key><int>3</int></prop.pair>
                <prop.pair><key>d</key><int>4</int></prop.pair>
            </prop.list>
        )"));
        QCOMPARE(list.type(), CosValue::Index::Object);
        COS_VALUE(get(list, "a"), Number, 1);
        COS_VALUE(get_value(list, "b"), Number, 2);
        COS_VALUE(get(list, "c"), Number, 3);
        COS_VALUE(get_value(list, "d"), Number, 4);
    }

    void test_xml_value_map()
    {
        auto val = xml_value(element(R"(
            <prop.map>
                <prop.list>
                    <prop.pair>
                        <key>array</key>
                        <array>
                            <array.type>
                                <int/>
                            </array.type>
                            <int>1</int>
                            <int>2</int>
                            <int>3</int>
                        </array>
                    </prop.pair>
                    <prop.pair>
                        <key>foo</key>
                        <prop.list>
                            <prop.pair>
                                <key>bar</key>
                                <prop.list>
                                    <prop.pair>
                                        <key>array</key>
                                        <array>
                                            <array.type>
                                                <int/>
                                            </array.type>
                                            <int>1</int>
                                            <int>2</int>
                                            <int>3</int>
                                        </array>
                                    </prop.pair>
                                </prop.list>
                            </prop.pair>
                        </prop.list>
                    </prop.pair>
                </prop.list>
            </prop.map>
        )"));
        QCOMPARE(val.type(), CosValue::Index::Object);
        COS_VALUE(get(val, "array", 1), Number, 2);
        COS_VALUE(get(val, "foo", "bar", "array", 2), Number, 3);
    }

    void test_gradient_structure()
    {
        auto val = xml_value(element(R"(
<?xml version='1.0'?>
<prop.map version='4'>
    <prop.list>
        <prop.pair>
            <key>Gradient Color Data</key>
            <prop.list>
                <prop.pair>
                    <key>Alpha Stops</key>
                    <prop.list>
                        <prop.pair>
                            <key>Stops List</key>
                            <prop.list>
                                <prop.pair>
                                    <key>Stop-0</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Alpha</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>0</float>
                                                <float>0.5</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                                <prop.pair>
                                    <key>Stop-1</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Alpha</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>1</float>
                                                <float>0.5</float>
                                                <float>0</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                            </prop.list>
                        </prop.pair>
                        <prop.pair>
                            <key>Stops Size</key>
                            <int type='unsigned' size='32'>2</int>
                        </prop.pair>
                    </prop.list>
                </prop.pair>
                <prop.pair>
                    <key>Color Stops</key>
                    <prop.list>
                        <prop.pair>
                            <key>Stops List</key>
                            <prop.list>
                                <prop.pair>
                                    <key>Stop-0</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Color</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>0</float>
                                                <float>0.5</float>
                                                <float>1</float>
                                                <float>0</float>
                                                <float>0</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                                <prop.pair>
                                    <key>Stop-1</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Color</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>1</float>
                                                <float>0.5</float>
                                                <float>0</float>
                                                <float>1</float>
                                                <float>0</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                            </prop.list>
                        </prop.pair>
                        <prop.pair>
                            <key>Stops Size</key>
                            <int type='unsigned' size='32'>2</int>
                        </prop.pair>
                    </prop.list>
                </prop.pair>
            </prop.list>
        </prop.pair>
        <prop.pair>
            <key>Gradient Colors</key>
            <string>1.0</string>
        </prop.pair>
    </prop.list>
</prop.map>
        )"));

        QCOMPARE(int(val.type()), int(CosValue::Index::Object));
        COS_VALUE(get(val, "Gradient Colors"), String, "1.0");
        COS_VALUE(get(val, "Gradient Color Data", "Alpha Stops", "Stops List", "Stop-0", "Stops Alpha", 1), Number, 0.5);
    }

#if 0
    void test_gradient_2c2a()
    {
        auto grad = parse_gradient_xml(QString(R"(
           <?xml version='1.0'?>
<prop.map version='4'>
    <prop.list>
        <prop.pair>
            <key>Gradient Color Data</key>
            <prop.list>
                <prop.pair>
                    <key>Alpha Stops</key>
                    <prop.list>
                        <prop.pair>
                            <key>Stops List</key>
                            <prop.list>
                                <prop.pair>
                                    <key>Stop-0</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Alpha</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>0</float>
                                                <float>0.5</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                                <prop.pair>
                                    <key>Stop-1</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Alpha</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>1</float>
                                                <float>0.5</float>
                                                <float>0</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                            </prop.list>
                        </prop.pair>
                        <prop.pair>
                            <key>Stops Size</key>
                            <int type='unsigned' size='32'>2</int>
                        </prop.pair>
                    </prop.list>
                </prop.pair>
                <prop.pair>
                    <key>Color Stops</key>
                    <prop.list>
                        <prop.pair>
                            <key>Stops List</key>
                            <prop.list>
                                <prop.pair>
                                    <key>Stop-0</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Color</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>0</float>
                                                <float>0.5</float>
                                                <float>1</float>
                                                <float>0</float>
                                                <float>0</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                                <prop.pair>
                                    <key>Stop-1</key>
                                    <prop.list>
                                        <prop.pair>
                                            <key>Stops Color</key>
                                            <array>
                                                <array.type>
                                                    <float/>
                                                </array.type>
                                                <float>1</float>
                                                <float>0.5</float>
                                                <float>0</float>
                                                <float>1</float>
                                                <float>0</float>
                                                <float>1</float>
                                            </array>
                                        </prop.pair>
                                    </prop.list>
                                </prop.pair>
                            </prop.list>
                        </prop.pair>
                        <prop.pair>
                            <key>Stops Size</key>
                            <int type='unsigned' size='32'>2</int>
                        </prop.pair>
                    </prop.list>
                </prop.pair>
            </prop.list>
        </prop.pair>
        <prop.pair>
            <key>Gradient Colors</key>
            <string>1.0</string>
        </prop.pair>
    </prop.list>
</prop.map>
        )"));

        QCOMPARE(grad.size(), 2);
        QCOMPARE(grad[0].first, 0);
        QCOMPARE(grad[1].first, 0);
        QCOMPARE(grad[0].second, QColor::fromRgbF(1, 0, 0, 1));
        QCOMPARE(grad[1].second, QColor::fromRgbF(0, 1, 0, 0));
    }
#endif
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_aep_gradient_xml.moc"




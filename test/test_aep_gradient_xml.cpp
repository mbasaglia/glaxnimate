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

    QDomDocument xml(QString xml, int local_line = 0)
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
            QTest::qFail(stdmsg.c_str(), line == 0 ? "xml" : __FILE__, line + local_line);
        }
        return dom;
    }

    QDomElement element(QString xml, int local_line = 0)
    {
        return this->xml(xml, local_line).documentElement();
    }

#define Element(xml) element(xml, __LINE__)


private slots:
    void test_xml_value_scalar()
    {
        COS_VALUE(xml_value(Element("<float>2.5</float>")), Number, 2.5);
        COS_VALUE(xml_value(Element("<int>25</int>")), Number, 25);
        COS_VALUE(xml_value(Element("<string>2.5</string>")), String, "2.5");
    }

    void test_xml_array()
    {
        auto arr = xml_array(Element(R"(
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

        auto list = xml_list(Element(R"(
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
        auto val = xml_value(Element(R"(
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
        auto list = xml_value(Element(R"(
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
        auto val = xml_value(Element(R"(
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
        auto val = xml_value(Element(R"(
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
        auto& data = get(val, "Gradient Color Data");
        QCOMPARE(int(data.type()), int(CosValue::Index::Object));
    }

    void test_get_gradient_stops_color()
    {
        auto data = xml_value(Element(R"(
            <prop.list>
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
                                                <float>0.4</float>
                                                <float>0.5</float>
                                                <float>0.1</float>
                                                <float>0.2</float>
                                                <float>0.3</float>
                                                <float>0.23</float>
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
                                                <float>0.6</float>
                                                <float>0.5</float>
                                                <float>0.7</float>
                                                <float>0.8</float>
                                                <float>0.9</float>
                                                <float>0.34</float>
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
        )"));

        QCOMPARE(int(data.type()), int(CosValue::Index::Object));
        QCOMPARE(int(get(data, "Color Stops").type()), int(CosValue::Index::Object));
        QCOMPARE(int(get(data, "Color Stops", "Stops List").type()), int(CosValue::Index::Object));
        auto& stop = get_as<CosValue::Index::Object>(data, "Color Stops", "Stops List", "Stop-0");
        auto& color_arr = get_as<CosValue::Index::Array>(stop, "Stops Color");
        COS_VALUE(get(color_arr, 0), Number, 0.4);
        auto stops = get_gradient_stops<GradientStopColor>(data);
        QCOMPARE(stops.size(), 2);
        QCOMPARE(stops[0].offset, 0.4);
        QCOMPARE(stops[0].value, QColor::fromRgbF(0.1, 0.2, 0.3));
    }

    void test_get_gradient_stops_alpha()
    {
        auto data = xml_value(Element(R"(
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
                                                <float>0.6</float>
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
                                                <float>0.5</float>
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
        )"));

        QCOMPARE(int(data.type()), int(CosValue::Index::Object));
        QCOMPARE(int(get(data, "Alpha Stops").type()), int(CosValue::Index::Object));
        QCOMPARE(int(get(data, "Alpha Stops", "Stops List").type()), int(CosValue::Index::Object));
        auto& stop = get_as<CosValue::Index::Object>(data, "Alpha Stops", "Stops List", "Stop-0");
        auto& color_arr = get_as<CosValue::Index::Array>(stop, "Stops Alpha");
        COS_VALUE(get(color_arr, 0), Number, 0);
        COS_VALUE(get(color_arr, 1), Number, 0.5);
        COS_VALUE(get(color_arr, 2), Number, 0.6);
        auto stops = get_gradient_stops<GradientStopAlpha>(data);
        QCOMPARE(stops.size(), 2);
        QCOMPARE(stops[0].offset, 0);
        QCOMPARE(stops[0].value, 0.6);
        QCOMPARE(stops[1].offset, 1);
        QCOMPARE(stops[1].value, 0.5);
    }

    void test_get_alpha_at_2()
    {
        GradientStops<double> stops;
        stops.push_back({0, 0.5, 0.6});
        stops.push_back({1, 0.5, 0.5});
        int index = 0;
        QCOMPARE(qRound(255 * stops.value_at(0, index)), qRound(255 * 0.6));
        QCOMPARE(index, 0);
        QCOMPARE(qRound(255 * stops.value_at(1, index)), qRound(255 * 0.5));
        QCOMPARE(index, 2);
        index = 0;
        QCOMPARE(qRound(255 * stops.value_at(0.5, index)), qRound(255 * 0.55));
        QCOMPARE(index, 0);
    }

    void test_gradient_2c2a()
    {
        auto aegrad = parse_gradient_xml(QString(R"(
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
                                                <float>0.6</float>
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
                                                <float>0.5</float>
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
                                                <float>0.1</float>
                                                <float>0.2</float>
                                                <float>0.3</float>
                                                <float>0.4</float>
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
                                                <float>0.7</float>
                                                <float>0.8</float>
                                                <float>0.9</float>
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

        auto grad = aegrad.to_qt();
        QCOMPARE(grad.size(), 2);
        QCOMPARE(grad[0].first, 0);
        QCOMPARE(grad[1].first, 1);
        QCOMPARE(grad[0].second, QColor::fromRgbF(0.1, 0.2, 0.3, 0.6));
        QCOMPARE(grad[1].second, QColor::fromRgbF(0.7, 0.8, 0.9, 0.5));
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_aep_gradient_xml.moc"




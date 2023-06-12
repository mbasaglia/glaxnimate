/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtTest/QtTest>

#include <vector>
#include "io/aep/riff.hpp"

using namespace glaxnimate::io::aep;

class TestCase: public QObject
{
    Q_OBJECT

private slots:
    void test_chunk_id()
    {
        ChunkId chunk = QByteArrayLiteral("rawr");
        QCOMPARE(chunk, "rawr");
        QVERIFY(chunk != "abcd");
    }

    void test_chunk_find()
    {
        RiffChunk parent{QByteArrayLiteral("rawr"), 0};
        parent.children.push_back(std::make_unique<RiffChunk>(RiffChunk{QByteArrayLiteral("itm1")}));
        parent.children.push_back(std::make_unique<RiffChunk>(RiffChunk{QByteArrayLiteral("itm3")}));
        parent.children.push_back(std::make_unique<RiffChunk>(RiffChunk{QByteArrayLiteral("rept"), 1}));
        parent.children.push_back(std::make_unique<RiffChunk>(RiffChunk{QByteArrayLiteral("rept"), 2}));
        parent.children.push_back(std::make_unique<RiffChunk>(RiffChunk{QByteArrayLiteral("itm2")}));

        auto it1 = parent.find("itm3");
        QVERIFY(it1 != parent.children.end());
        QCOMPARE((*it1)->header, "itm3");
        auto it2 = parent.find("rept", it1);
        QVERIFY(it2 != parent.children.end());
        QCOMPARE((*it2)->header, "rept");
        QCOMPARE(parent.child("itm3")->header, "itm3");
        QCOMPARE(parent.child("itm7"), nullptr);

        std::vector<const RiffChunk*> items(5);
        std::vector<const RiffChunk**> ptrs;
        for ( auto& p : items )
            ptrs.push_back(&p);

        parent.find_multiple(ptrs, {"rept", "itm1", "itm2", "itm3", "itm4"});
        QCOMPARE(items[0]->length, 1);
        QCOMPARE(items[1]->header, "itm1");
        QCOMPARE(items[2]->header, "itm2");
        QCOMPARE(items[3]->header, "itm3");
        QCOMPARE(items[4], nullptr);
    }

    void test_big_endian_u8()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x69");
        auto value = 0x69;
        QCOMPARE(endian.read_uint<1>(data), value);
        QCOMPARE(endian.read_uint<std::uint8_t>(data), value);
    }

    void test_big_endian_u16()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x13\x37");
        auto value = 0x1337;
        QCOMPARE(endian.read_uint<2>(data), value);
        QCOMPARE(endian.read_uint<std::uint16_t>(data), value);
    }

    void test_big_endian_u32()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x13\x37\x04\x20");
        auto value = 0x13370420;
        QCOMPARE(endian.read_uint<4>(data), value);
        QCOMPARE(endian.read_uint<std::uint32_t>(data), value);
    }

    void test_big_endian_s8()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x69");
        auto value = 0x69;
        QCOMPARE(endian.read_sint<1>(data), value);
        QCOMPARE(endian.read_sint<std::int8_t>(data), value);
    }

    void test_big_endian_s16()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x13\x37");
        auto value = 0x1337;
        QCOMPARE(endian.read_sint<2>(data), value);
        QCOMPARE(endian.read_sint<std::uint16_t>(data), value);
    }

    void test_big_endian_s32()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x13\x37\x04\x20");
        auto value = 0x13370420;
        QCOMPARE(endian.read_sint<4>(data), value);
        QCOMPARE(endian.read_sint<std::int32_t>(data), value);
    }


    void test_big_endian_s8_neg()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\x97");
        auto value = -0x69;
        QCOMPARE(endian.read_sint<1>(data), value);
        QCOMPARE(endian.read_sint<std::int8_t>(data), value);
    }

    void test_big_endian_s16_neg()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\xff\x97");
        auto value = -0x69;
        QCOMPARE(std::int16_t(std::uint16_t(0xff97)), value);
        QCOMPARE(endian.read_sint<2>(data), value);
        QCOMPARE(endian.read_sint<std::int16_t>(data), value);
    }

    void test_big_endian_s32_neg()
    {
        auto endian = Endianness::Big();
        auto data = QByteArrayLiteral("\xff\xff\xff\x97");
        auto value = -0x69;
        QCOMPARE(endian.read_sint<4>(data), value);
        QCOMPARE(endian.read_sint<std::int32_t>(data), value);
    }


    void test_little_endian_u8()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x69");
        auto value = 0x69;
        QCOMPARE(endian.read_uint<1>(data), value);
        QCOMPARE(endian.read_uint<std::uint8_t>(data), value);
    }

    void test_little_endian_u16()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x37\x13");
        auto value = 0x1337;
        QCOMPARE(endian.read_uint<2>(data), value);
        QCOMPARE(endian.read_uint<std::uint16_t>(data), value);
    }

    void test_little_endian_u32()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x20\x04\x37\x13");
        auto value = 0x13370420;
        QCOMPARE(endian.read_uint<4>(data), value);
        QCOMPARE(endian.read_uint<std::uint32_t>(data), value);
    }

    void test_little_endian_s8()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x69");
        auto value = 0x69;
        QCOMPARE(endian.read_sint<1>(data), value);
        QCOMPARE(endian.read_sint<std::int8_t>(data), value);
    }

    void test_little_endian_s16()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x37\x13");
        auto value = 0x1337;
        QCOMPARE(endian.read_sint<2>(data), value);
        QCOMPARE(endian.read_sint<std::uint16_t>(data), value);
    }

    void test_little_endian_s32()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x20\x04\x37\x13");
        auto value = 0x13370420;
        QCOMPARE(endian.read_sint<4>(data), value);
        QCOMPARE(endian.read_sint<std::int32_t>(data), value);
    }


    void test_little_endian_s8_neg()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x97");
        auto value = -0x69;
        QCOMPARE(endian.read_sint<1>(data), value);
        QCOMPARE(endian.read_sint<std::int8_t>(data), value);
    }

    void test_little_endian_s16_neg()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x97\xff");
        auto value = -0x69;
        QCOMPARE(std::int16_t(std::uint16_t(0xff97)), value);
        QCOMPARE(endian.read_sint<2>(data), value);
        QCOMPARE(endian.read_sint<std::int16_t>(data), value);
    }

    void test_little_endian_s32_neg()
    {
        auto endian = Endianness::Little();
        auto data = QByteArrayLiteral("\x97\xff\xff\xff");
        auto value = -0x69;
        QCOMPARE(endian.read_sint<4>(data), value);
        QCOMPARE(endian.read_sint<std::int32_t>(data), value);
    }

    void test_big_endian_f32()
    {
        auto endian = Endianness::Big();
        QCOMPARE(endian.read_float32(QByteArrayLiteral("\x3f\x80\x00\x00")), 1.f);
        QCOMPARE(endian.read_float32(QByteArrayLiteral("\x44\xa7\x20\x00")), 1337.f);
        QCOMPARE(endian.read_float32(QByteArrayLiteral("\xc2\x8a\x00\x00")), -69.f);
    }

    void test_big_endian_f64()
    {
        auto endian = Endianness::Big();
        QCOMPARE(endian.read_float64(QByteArrayLiteral("\x3f\xf0\x00\x00\x00\x00\x00\x00")), 1.);
        QCOMPARE(endian.read_float64(QByteArrayLiteral("\x40\x94\xe4\x00\x00\x00\x00\x00")), 1337.);
        QCOMPARE(endian.read_float64(QByteArrayLiteral("\xc0\x51\x40\x00\x00\x00\x00\x00")), -69.);
    }

    void test_rifx_reader_root()
    {
        QByteArray arr = QByteArrayLiteral(
            "RIFX\x00\x00\x00\x04rawr"
        );
        QBuffer file(&arr);
        file.open(QIODevice::ReadOnly);
        RiffReader reader;
        auto chunk = reader.parse(&file);
        QCOMPARE(chunk.header, "RIFX");
        QCOMPARE(chunk.length, 4);
        QCOMPARE(chunk.subheader, "rawr");
        QCOMPARE(chunk.data().size(), 0);
        QCOMPARE(chunk.children.size(), 0);
    }

    void test_rifx_reader_chunk()
    {
        QByteArray arr = QByteArrayLiteral(
            "RIFX\x00\x00\x00\x28rawrawoo\0\0\0\4\1\2\3\4awoo\0\0\0\3\1\2\3\0awoo\0\0\0\4\1\2\3\4"
        );
        QBuffer file(&arr);
        file.open(QIODevice::ReadOnly);
        RiffReader reader;
        auto chunk = reader.parse(&file);
        QCOMPARE(chunk.header, "RIFX");
        QCOMPARE(chunk.length, 40);
        QCOMPARE(chunk.subheader, "rawr");
        QCOMPARE(chunk.data().size(), 0);
        QCOMPARE(chunk.children.size(), 3);

        auto child = chunk.children[0].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);

        child = chunk.children[1].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 3);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3"));
        QCOMPARE(child->children.size(), 0);

        child = chunk.children[2].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);
    }


    void test_rifx_reader_list()
    {
        QByteArray arr = QByteArrayLiteral(
            "RIFX\x00\x00\x00\x34rawrLIST\0\0\0\x28listawoo\0\0\0\4\1\2\3\4awoo\0\0\0\3\1\2\3\0awoo\0\0\0\4\1\2\3\4"
        );
        QBuffer file(&arr);
        file.open(QIODevice::ReadOnly);
        RiffReader reader;
        auto chunk = reader.parse(&file);
        QCOMPARE(chunk.header, "RIFX");
        QCOMPARE(chunk.length, 52);
        QCOMPARE(chunk.subheader, "rawr");
        QCOMPARE(chunk.data().size(), 0);
        QCOMPARE(chunk.children.size(), 1);

        auto list = chunk.children[0].get();
        QCOMPARE(list->header, "LIST");
        QCOMPARE(list->subheader, "list");
        QCOMPARE(list->length, 40);
        QCOMPARE(list->data().size(), 0);
        QCOMPARE(list->children.size(), 3);

        auto child = list->children[0].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().size(), 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);

        child = list->children[1].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 3);
        QCOMPARE(child->data().size(), 3);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3"));
        QCOMPARE(child->children.size(), 0);

        child = list->children[2].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().size(), 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);
    }

    void test_riff_reader_list()
    {
        QByteArray arr = QByteArrayLiteral(
            "RIFF\x34\x00\x00\x00rawrLIST\x28\0\0\0listawoo\4\0\0\0\1\2\3\4awoo\3\0\0\0\1\2\3\0awoo\4\0\0\0\1\2\3\4"
        );
        QBuffer file(&arr);
        file.open(QIODevice::ReadOnly);
        RiffReader reader;
        auto chunk = reader.parse(&file);
        QCOMPARE(chunk.header, "RIFF");
        QCOMPARE(chunk.length, 52);
        QCOMPARE(chunk.subheader, "rawr");
        QCOMPARE(chunk.data().size(), 0);
        QCOMPARE(chunk.children.size(), 1);

        auto list = chunk.children[0].get();
        QCOMPARE(list->header, "LIST");
        QCOMPARE(list->subheader, "list");
        QCOMPARE(list->length, 40);
        QCOMPARE(list->data().size(), 0);
        QCOMPARE(list->children.size(), 3);

        auto child = list->children[0].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().size(), 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);

        child = list->children[1].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 3);
        QCOMPARE(child->data().size(), 3);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3"));
        QCOMPARE(child->children.size(), 0);

        child = list->children[2].get();
        QCOMPARE(child->header, "awoo");
        QCOMPARE(child->length, 4);
        QCOMPARE(child->data().size(), 4);
        QCOMPARE(child->data().read(), QByteArrayLiteral("\1\2\3\4"));
        QCOMPARE(child->children.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_riff.moc"


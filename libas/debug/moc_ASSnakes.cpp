/****************************************************************************
** Meta object code from reading C++ file 'ASSnakes.h'
**
** Created: Sun Jan 20 12:10:40 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../include/ASSnakes.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ASSnakes.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ASSnakes[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   10,    9,    9, 0x0a,
      34,   30,    9,    9, 0x0a,
      57,   50,    9,    9, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ASSnakes[] = {
    "ASSnakes\0\0min\0setSMin(double)\0max\0"
    "setSMax(double)\0radius\0setCoverRadius(double)\0"
};

void ASSnakes::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ASSnakes *_t = static_cast<ASSnakes *>(_o);
        switch (_id) {
        case 0: _t->setSMin((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 1: _t->setSMax((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 2: _t->setCoverRadius((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ASSnakes::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ASSnakes::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ASSnakes,
      qt_meta_data_ASSnakes, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ASSnakes::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ASSnakes::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ASSnakes::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ASSnakes))
        return static_cast<void*>(const_cast< ASSnakes*>(this));
    return QObject::qt_metacast(_clname);
}

int ASSnakes::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE

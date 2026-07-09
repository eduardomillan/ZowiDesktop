/****************************************************************************
** Meta object code from reading C++ file 'sessioncontroller.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/sessioncontroller.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sessioncontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SessionController_t {
    QByteArrayData data[12];
    char stringdata0[190];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SessionController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SessionController_t qt_meta_stringdata_SessionController = {
    {
QT_MOC_LITERAL(0, 0, 17), // "SessionController"
QT_MOC_LITERAL(1, 18, 14), // "sessionChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 27), // "loadActiveZowiDeviceAddress"
QT_MOC_LITERAL(4, 62, 18), // "loadActiveZowiName"
QT_MOC_LITERAL(5, 81, 18), // "hasDismissedWizard"
QT_MOC_LITERAL(6, 100, 27), // "saveActiveZowiDeviceAddress"
QT_MOC_LITERAL(7, 128, 7), // "address"
QT_MOC_LITERAL(8, 136, 18), // "saveActiveZowiName"
QT_MOC_LITERAL(9, 155, 4), // "name"
QT_MOC_LITERAL(10, 160, 19), // "saveWizardDismissed"
QT_MOC_LITERAL(11, 180, 9) // "dismissed"

    },
    "SessionController\0sessionChanged\0\0"
    "loadActiveZowiDeviceAddress\0"
    "loadActiveZowiName\0hasDismissedWizard\0"
    "saveActiveZowiDeviceAddress\0address\0"
    "saveActiveZowiName\0name\0saveWizardDismissed\0"
    "dismissed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SessionController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       3,    0,   50,    2, 0x02 /* Public */,
       4,    0,   51,    2, 0x02 /* Public */,
       5,    0,   52,    2, 0x02 /* Public */,
       6,    1,   53,    2, 0x02 /* Public */,
       8,    1,   56,    2, 0x02 /* Public */,
      10,    1,   59,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::QString,
    QMetaType::QString,
    QMetaType::Bool,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::Bool,   11,

       0        // eod
};

void SessionController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SessionController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sessionChanged(); break;
        case 1: { QString _r = _t->loadActiveZowiDeviceAddress();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 2: { QString _r = _t->loadActiveZowiName();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->hasDismissedWizard();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->saveActiveZowiDeviceAddress((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->saveActiveZowiName((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->saveWizardDismissed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SessionController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SessionController::sessionChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SessionController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SessionController.data,
    qt_meta_data_SessionController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SessionController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SessionController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SessionController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SessionController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void SessionController::sessionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

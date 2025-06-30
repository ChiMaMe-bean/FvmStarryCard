/****************************************************************************
** Meta object code from reading C++ file 'stellarforge.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../stellarforge.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'stellarforge.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN17CardSettingDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN17CardSettingDialogE = QtMocHelpers::stringData(
    "CardSettingDialog",
    "applyMainCardToAll",
    "",
    "row",
    "cardType",
    "bind",
    "unbound",
    "applySubCardToAll",
    "onApplyMainCardToAll",
    "onApplySubCardToAll",
    "onConfigChanged"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN17CardSettingDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    4,   44,    2, 0x06,    1 /* Public */,
       7,    4,   53,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   62,    2, 0x08,   11 /* Private */,
       9,    0,   63,    2, 0x08,   12 /* Private */,
      10,    0,   64,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,    3,    4,    5,    6,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,    3,    4,    5,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject CardSettingDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN17CardSettingDialogE.offsetsAndSizes,
    qt_meta_data_ZN17CardSettingDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN17CardSettingDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CardSettingDialog, std::true_type>,
        // method 'applyMainCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'applySubCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onApplyMainCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onApplySubCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConfigChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CardSettingDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CardSettingDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->applyMainCardToAll((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 1: _t->applySubCardToAll((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 2: _t->onApplyMainCardToAll(); break;
        case 3: _t->onApplySubCardToAll(); break;
        case 4: _t->onConfigChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (CardSettingDialog::*)(int , const QString & , bool , bool );
            if (_q_method_type _q_method = &CardSettingDialog::applyMainCardToAll; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (CardSettingDialog::*)(int , const QString & , bool , bool );
            if (_q_method_type _q_method = &CardSettingDialog::applySubCardToAll; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *CardSettingDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CardSettingDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN17CardSettingDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CardSettingDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void CardSettingDialog::applyMainCardToAll(int _t1, const QString & _t2, bool _t3, bool _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CardSettingDialog::applySubCardToAll(int _t1, const QString & _t2, bool _t3, bool _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
namespace {
struct qt_meta_tag_ZN12StellarForgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN12StellarForgeE = QtMocHelpers::stringData(
    "StellarForge",
    "changeBackground",
    "",
    "theme",
    "selectCustomBackground",
    "updateSelectBtnState",
    "startMouseTracking",
    "stopMouseTracking",
    "trackMousePosition",
    "pos",
    "updateHandleDisplay",
    "HWND",
    "hwnd",
    "handleButtonRelease",
    "startEnhancement",
    "stopEnhancement",
    "performEnhancement",
    "addLog",
    "message",
    "LogType",
    "type",
    "SetDPIAware",
    "onCaptureAndRecognize",
    "onEnhancementConfigChanged",
    "saveEnhancementConfig",
    "loadEnhancementConfig",
    "loadEnhancementConfigFromFile",
    "onCardSettingButtonClicked",
    "onApplyMainCardToAll",
    "row",
    "cardType",
    "bind",
    "unbound",
    "onApplySubCardToAll",
    "onMaxEnhancementLevelChanged"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN12StellarForgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  152,    2, 0x08,    1 /* Private */,
       4,    0,  155,    2, 0x08,    3 /* Private */,
       5,    1,  156,    2, 0x08,    4 /* Private */,
       6,    0,  159,    2, 0x08,    6 /* Private */,
       7,    0,  160,    2, 0x08,    7 /* Private */,
       8,    1,  161,    2, 0x08,    8 /* Private */,
      10,    1,  164,    2, 0x08,   10 /* Private */,
      13,    0,  167,    2, 0x08,   12 /* Private */,
      14,    0,  168,    2, 0x08,   13 /* Private */,
      15,    0,  169,    2, 0x08,   14 /* Private */,
      16,    0,  170,    2, 0x08,   15 /* Private */,
      17,    2,  171,    2, 0x08,   16 /* Private */,
      17,    1,  176,    2, 0x28,   19 /* Private | MethodCloned */,
      21,    0,  179,    2, 0x08,   21 /* Private */,
      22,    0,  180,    2, 0x08,   22 /* Private */,
      23,    0,  181,    2, 0x08,   23 /* Private */,
      24,    0,  182,    2, 0x08,   24 /* Private */,
      25,    0,  183,    2, 0x08,   25 /* Private */,
      26,    0,  184,    2, 0x08,   26 /* Private */,
      27,    0,  185,    2, 0x08,   27 /* Private */,
      28,    4,  186,    2, 0x08,   28 /* Private */,
      33,    4,  195,    2, 0x08,   33 /* Private */,
      34,    0,  204,    2, 0x08,   38 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 19,   18,   20,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Int,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,   29,   30,   31,   32,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,   29,   30,   31,   32,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject StellarForge::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ZN12StellarForgeE.offsetsAndSizes,
    qt_meta_data_ZN12StellarForgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN12StellarForgeE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StellarForge, std::true_type>,
        // method 'changeBackground'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectCustomBackground'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateSelectBtnState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'startMouseTracking'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopMouseTracking'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'trackMousePosition'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QPoint, std::false_type>,
        // method 'updateHandleDisplay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<HWND, std::false_type>,
        // method 'handleButtonRelease'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startEnhancement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopEnhancement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'performEnhancement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<LogType, std::false_type>,
        // method 'addLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'SetDPIAware'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onCaptureAndRecognize'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEnhancementConfigChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveEnhancementConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadEnhancementConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadEnhancementConfigFromFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCardSettingButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onApplyMainCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onApplySubCardToAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onMaxEnhancementLevelChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void StellarForge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<StellarForge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changeBackground((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->selectCustomBackground(); break;
        case 2: _t->updateSelectBtnState((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->startMouseTracking(); break;
        case 4: _t->stopMouseTracking(); break;
        case 5: _t->trackMousePosition((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 6: _t->updateHandleDisplay((*reinterpret_cast< std::add_pointer_t<HWND>>(_a[1]))); break;
        case 7: _t->handleButtonRelease(); break;
        case 8: _t->startEnhancement(); break;
        case 9: _t->stopEnhancement(); break;
        case 10: _t->performEnhancement(); break;
        case 11: _t->addLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<LogType>>(_a[2]))); break;
        case 12: _t->addLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: { int _r = _t->SetDPIAware();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->onCaptureAndRecognize(); break;
        case 15: _t->onEnhancementConfigChanged(); break;
        case 16: _t->saveEnhancementConfig(); break;
        case 17: _t->loadEnhancementConfig(); break;
        case 18: _t->loadEnhancementConfigFromFile(); break;
        case 19: _t->onCardSettingButtonClicked(); break;
        case 20: _t->onApplyMainCardToAll((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 21: _t->onApplySubCardToAll((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 22: _t->onMaxEnhancementLevelChanged(); break;
        default: ;
        }
    }
}

const QMetaObject *StellarForge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StellarForge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN12StellarForgeE.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int StellarForge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    return _id;
}
QT_WARNING_POP

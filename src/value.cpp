#include <charconv>
#include <float.h>
#include <ostream>

#include "value.h"

namespace Json
{
    std::string numberToString(double v)
    {
        // проверка на NaN
        if (!(v == v))
            return "null";

        // проверка на inf
        if (!((v <= DBL_MAX) && (v >= -DBL_MAX)))
            return "null";

        // проверка на ноль
        if (v == 0)
            return std::signbit(v) ? "-0.0" : "0.0";

        int exp = floor(log10(fabs(v)));

        if (exp > 8 || exp < -8)
        {
            char buf[32];
            int n = snprintf(buf, 31, "%.16e", v);
            return std::string(buf, n);
        }
        else
        {
            std::string s = std::to_string(v);
            while (s.size() > 2)
            {
                if (s[s.size() - 2] != '.' && s[s.size() - 1] == '0')
                {
                    s.resize(s.size() - 1);
                }
                else
                {
                    break;
                }
            }
            return s;
        }
    }

    std::string numberToString(long long v)
    {
        return std::to_string(v);
    }

    static void escapestringto(std::string &buff, const std::string &v)
    {
        int pc = -1;
        for (auto &c : v)
        {
            switch (c)
            {
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                buff.push_back('\\');
                buff.push_back(c);
                break;

            case '\"':
            case '\\':
            case '/':
                if (pc == '\\')
                {
                    buff.push_back(c);
                }
                else
                {
                    buff.push_back('\\');
                    buff.push_back(c);
                }
                break;

            default:
                buff.push_back(c);
                break;
            }
            pc = c;
        }
    }

    std::string escapedString(const std::string &s)
    {
        std::string es;
        escapestringto(es, s);
        return es;
    }

    static void unescapestringto(std::string &buff, const char *v, size_t size)
    {
        int pc = -1;
        char c;
        const char *pb = 0;
        for (const char *pe = v + size; c = *v, v < pe;)
        {
            if (pc == '\\')
            {
                pc = -1;
                switch (c)
                {
                case '\"':
                case '\\':
                case '/':
                    buff.push_back(c);
                    break;

                case 'b':
                    buff.push_back('\b');
                    break;
                case 'f':
                    buff.push_back('\f');
                    break;
                case 'n':
                    buff.push_back('\n');
                    break;
                case 'r':
                    buff.push_back('\r');
                    break;
                case 't':
                    buff.push_back('\t');
                    break;
                case 'u':
                {
                    char *ec = (char *)(v + 1);
                    u_int32_t ch = strtol(ec, &ec, 16);
                    if (ch < 0x80)
                    {
                        buff.push_back((char)ch);
                    }
                    else if (ch < 0x800)
                    {
                        buff.push_back((ch >> 6) | 0xC0);
                        buff.push_back((ch & 0x3F) | 0x80);
                    }
                    else if (ch < 0x10000)
                    {
                        if (ch >= 0xD800 && ch <= 0xDBFF)
                        {
                            // surrogate pair
                            if (*ec == '\\' && *(ec + 1) == 'u')
                            {
                                char *ec2 = (char *)(ec + 2);
                                u_int32_t ch2 = strtol(ec2, &ec2, 16);

                                ch = (ch - 0xD800) * 0x400;
                                ch2 = (ch2 - 0xDC00);

                                ch = ch + ch2 + 0x10000;

                                buff.push_back((ch >> 18) | 0xF0);
                                buff.push_back(((ch >> 12) & 0x3F) | 0x80);
                                buff.push_back(((ch >> 6) & 0x3F) | 0x80);
                                buff.push_back((ch & 0x3F) | 0x80);

                                v = ec2;
                                continue;
                            }
                        }
                        {
                            buff.push_back((ch >> 12) | 0xE0);
                            buff.push_back(((ch >> 6) & 0x3F) | 0x80);
                            buff.push_back((ch & 0x3F) | 0x80);
                        }
                    }
                    else if (ch < 0x110000)
                    {
                        buff.push_back((ch >> 18) | 0xF0);
                        buff.push_back(((ch >> 12) & 0x3F) | 0x80);
                        buff.push_back(((ch >> 6) & 0x3F) | 0x80);
                        buff.push_back((ch & 0x3F) | 0x80);
                    }
                    v = ec;
                    continue;
                }
                break;

                default:
                    buff.push_back(pc);
                    buff.push_back(c);
                    break;
                }
            }
            else if (c == '\\')
            {
                if (pb)
                {
                    buff.append(pb, v);
                    pb = 0;
                }
                pc = c;
            }
            else if (pb == 0)
            {
                pb = v;
            }
            ++v;
        }
        if (pb)
        {
            buff.append(pb, v);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //  Value class implementation
    //
    //
    //////////////////////////////////////////////////////////////////////////////
    const Value Value::_emptyValue;

    Value::Value(Type) : _type(Type::UNDEFINED)
    {
    }

    Value::Value(bool v) : _type(Type::BOOLEAN)
    {
        _value._l = v;
    }

    Value::Value(int v) : _type(Type::INTEGER)
    {
        _value._i = v;
    }

    Value::Value(long v) : _type(Type::INTEGER)
    {
        _value._i = v;
    }

    Value::Value(long long v) : _type(Type::INTEGER)
    {
        _value._i = v;
    }

    Value::Value(size_t v) : _type(Type::INTEGER)
    {
        _value._i = (long long)v;
    }

    Value::Value(double v) : _type(Type::NUMBER)
    {
        // проверка на NaN
        if (!(v == v))
        {
            _type = Type::UNDEFINED;
            return;
        }

        // проверка на inf
        if (!((v <= DBL_MAX) && (v >= -DBL_MAX)))
        {
            _type = Type::UNDEFINED;
            return;
        }

        _value._d = v;
    }

    Value::Value(const char *v) : _type(Type::STRING)
    {
        _value._s = new std::string(v);
    }

    Value::Value(const std::string &v) : _type(Type::STRING)
    {
        _value._s = new std::string(v);
    }

    Value::Value(std::string &&v) : _type(Type::STRING)
    {
        _value._s = new std::string(std::move(v));
    }

    Value Value::createArray()
    {
        Value v;
        v._type = Type::ARRAY;
        v._value._a = new ArrayContainer;
        return v;
    }

    Value Value::createObject()
    {
        Value v;
        v._type = Type::OBJECT;
        v._value._o = new ObjectContainer;
        return v;
    }

    void Value::reset()
    {
        switch (_type)
        {
        case Type::OBJECT:
            delete _value._o;
            break;

        case Type::ARRAY:
            delete _value._a;
            break;

        case Type::STRING:
            delete _value._s;
            break;

        default:
            break;
        }
        _type = Type::UNDEFINED;
    }

    Value::~Value()
    {
        reset();
    }

    Value::Value(const Value &v) : _type(v._type)
    {
        switch (_type)
        {
        case Type::OBJECT:
            _value._o = new ObjectContainer(*v._value._o);
            break;

        case Type::ARRAY:
            _value._a = new ArrayContainer(*v._value._a);
            break;

        case Type::STRING:
            _value._s = new std::string(*v._value._s);
            break;

        default:
            _value = v._value;
            break;
        }
    }

    Value::Value(Value &&v) noexcept : _type(v._type), _value(v._value)
    {
        v._type = Type::UNDEFINED;
    }

    Value &Value::operator=(const Value &v)
    {
        if (&v == this)
            return *this;

        Type savedType = v._type;
        _Value savedValue;

        switch (savedType)
        {
        case Type::OBJECT:
            savedValue._o = new ObjectContainer(*v._value._o);
            break;

        case Type::ARRAY:
            savedValue._a = new ArrayContainer(*v._value._a);
            break;

        case Type::STRING:
            savedValue._s = new std::string(*v._value._s);
            break;

        default:
            savedValue = v._value;
            break;
        }

        reset();

        _type = savedType;
        _value = savedValue;

        return *this;
    }

    Value &Value::operator=(Value &&v) noexcept
    {
        if (&v == this)
            return *this;

        reset();

        _type = v._type;
        _value = v._value;

        v._type = Type::UNDEFINED;

        return *this;
    }

    bool Value::asBoolean(bool defaultValue) const
    {
        switch (_type)
        {
        case Type::BOOLEAN:
            return _value._l;
        case Type::STRING:
            return !_value._s->empty();
        case Type::INTEGER:
            return _value._i != 0;
        case Type::NUMBER:
            return _value._d != 0;
        case Type::UNDEFINED:
        case Type::OBJECT:
        case Type::ARRAY:
        default:
            return defaultValue;
        }
    }

    double Value::asNumber(double defaultValue) const
    {
        char *p = 0;
        switch (_type)
        {
        case Type::BOOLEAN:
            return _value._l ? 1 : 0;
        case Type::INTEGER:
            return (double)_value._i;
        case Type::NUMBER:
            return _value._d;
        case Type::STRING:
            return strtod(_value._s->c_str(), &p);
        default:
            return defaultValue;
        }
    }

    long Value::asLong(long defaultValue) const
    {
        return (long)asLongLong(defaultValue);
    }

    long long Value::asLongLong(long long defaultValue) const
    {
        char *p = 0;
        switch (_type)
        {
        case Type::BOOLEAN:
            return _value._l ? 1 : 0;
        case Type::INTEGER:
            return _value._i;
        case Type::NUMBER:
            return (long long)_value._d;
        case Type::STRING:
            return strtoll(_value._s->c_str(), &p, 10);
        default:
            return defaultValue;
        }
    }

    int Value::asInt(int defaultValue) const
    {
        return (int)asLongLong(defaultValue);
    }

    std::string Value::asString(const std::string &defaultValue) const
    {
        switch (_type)
        {
        case Type::ARRAY:
            return "Array[]";
        case Type::OBJECT:
            return "Object{}";
        case Type::BOOLEAN:
            return _value._l ? "true" : "false";
        case Type::INTEGER:
            return numberToString(_value._i);
        case Type::NUMBER:
            return numberToString(_value._d);
        case Type::STRING:
            return *_value._s;
        case Type::UNDEFINED:
        default:
            return defaultValue;
        }
    }

    const std::string &Value::asConstString(const std::string &defaultValue) const
    {
        if (_type == Type::STRING)
            return *_value._s;
        return defaultValue;
    }

    std::string Value::asEscapedString(const std::string &defaultValue) const
    {
        return escapedString(this->asString(defaultValue));
    }

    bool Value::hasKey(const std::string &str) const
    {
        if (_type != Type::OBJECT)
            return false;
        return _value._o->find(str) != _value._o->end();
    }

    Value &Value::operator[](size_t key)
    {
        if (_type != Type::ARRAY)
        {
            reset();
            _type = Type::ARRAY;
            _value._a = new ArrayContainer;
        }

        if (key < _value._a->size())
        {
            return (*_value._a)[key];
        }
        else
        {
            _value._a->resize(key + 1);
            return _value._a->back();
        }
    }

    Value &Value::add(const Value &v)
    {
        return (*this)[size()] = v;
    }

    Value &Value::add(Value &&v)
    {
        return (*this)[size()] = std::move(v);
    }

    Value &Value::operator[](const std::string &key)
    {
        if (_type != Type::OBJECT)
        {
            reset();
            _type = Type::OBJECT;
            _value._o = new ObjectContainer;
        }

        return _value._o->operator[](key);
    }

    Value &Value::operator[](std::string &&key)
    {
        if (_type != Type::OBJECT)
        {
            reset();
            _type = Type::OBJECT;
            _value._o = new ObjectContainer;
        }

        return _value._o->operator[](std::move(key));
    }

    const Value &Value::operator[](const std::string &key) const
    {
        switch (_type)
        {
        case Type::OBJECT:
        {
            ObjectContainer::iterator i = _value._o->find(key);
            if (i != _value._o->end())
                return i->second;
        }
        break;

        default:
            break;
        }
        return _emptyValue;
    }

    const Value &Value::operator[](size_t key) const
    {
        switch (_type)
        {
        case Type::ARRAY:
        {
            if (key < _value._a->size())
                return (*_value._a)[key];
        }
        break;

        default:
            break;
        }
        return _emptyValue;
    }

    size_t Value::size() const
    {
        switch (_type)
        {
        case Type::ARRAY:
            return _value._a->size();
        case Type::OBJECT:
            return _value._o->size();
        default:
            return 0;
        }
    }

    void Value::reserve(size_t size)
    {
        switch (_type)
        {
        case Type::ARRAY:
            _value._a->reserve(size);
            break;
        case Type::OBJECT:
            _value._o->reserve(size);
            break;
        default:
            break;
        }
    }

    void Value::clear()
    {
        switch (_type)
        {
        case Type::ARRAY:
            _value._a->clear();
            break;

        case Type::OBJECT:
            _value._o->clear();
            break;

        default:
            break;
        }
    }

    void Value::erase(const Value &key)
    {
        switch (_type)
        {
        case Type::ARRAY:
        {
            int N = key.asInt();
            if (N >= 0 && (size_t)N < _value._a->size())
                _value._a->erase(_value._a->begin() + (ptrdiff_t)N);
        }
        break;

        case Type::OBJECT:
            _value._o->erase(key.asString());
            break;

        default:
            break;
        }
    }

    std::vector<std::string> Value::indexes() const
    {
        std::vector<std::string> ret;

        if (_type == Type::OBJECT)
        {
            ret.resize(_value._o->size());
            std::vector<std::string>::iterator p = ret.begin();
            for (auto i = _value._o->begin(); i != _value._o->end(); ++i)
            {
                *p = (*i).first;
                ++p;
            }
        }

        return ret;
    }

    /*
     11.9.3 The Abstract Equality Comparison Algorithm
     http://www.ecma-international.org/ecma-262/5.1/#sec-11.9.3
     */
    bool Value::operator==(const Value &v) const
    {
        if (_type == v._type)
        {
            switch (_type)
            {
            case Type::UNDEFINED:
                return true;
            case Type::BOOLEAN:
                return _value._l == v._value._l;
            case Type::INTEGER:
                return _value._i == v._value._i;
            case Type::NUMBER:
                return _value._d == v._value._d;
            case Type::STRING:
                return *_value._s == *v._value._s;
            case Type::ARRAY:
                return *_value._a == *v._value._a;
            case Type::OBJECT:
                return *_value._o == *v._value._o;
            }
        }
        else
        {
            switch (v._type)
            {
            case Type::BOOLEAN:
                return asBoolean() == v._value._l;
            case Type::INTEGER:
                return asLongLong() == v._value._i;
            case Type::NUMBER:
                return asNumber() == v._value._d;
            case Type::STRING:
                return asString() == *v._value._s;
            case Type::UNDEFINED:
            case Type::ARRAY:
            case Type::OBJECT:
                return false;
            }
        }
        return false;
    }

    std::string Value::stringifyThis() const
    {
        return stringify(*this, true);
    }

    std::string Value::prettyStringifyThis() const
    {
        return prettyStringify(*this);
    }

    ObjectContainer *Value::asObject() const
    {
        switch (_type)
        {
        case Type::OBJECT:
            return _value._o;
        default:
            return nullptr;
        }
    }

    ArrayContainer *Value::asArray() const
    {
        switch (_type)
        {
        case Type::ARRAY:
            return _value._a;
        default:
            return nullptr;
        }
    }

    std::ostream &operator<<(std::ostream &os, const Value &value)
    {
        os << value.stringifyThis();
        return os;
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    //
    //  END OF Value class implementation
    //
    //
    ////////////////////////////////////////////////////////////////////////////




    ////////////////////////////////////////////////////////////////////////////////
    Json::Value parseValue(const char *&data, const char *end);

    ////////////////////////////////////////////////////////////////////////////////
    inline char ISXDIGIT(char c)
    {
        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return true;

        default:
            return false;
        }
    }

    inline Json::Value parseNumber(const char *&data, const char *end)
    {
        const char *buf = data;
        bool isDot = false;
        while (data < end)
        {
            switch (*data)
            {
            case '-':
            case '+':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;

            case '.':
            case 'E':
            case 'e':
                isDot = true;
                break;                

            default:
                goto PARSE_NUMBER_END;
            }
            ++data;
        }
    PARSE_NUMBER_END:
        size_t size = data - buf;
        if (isDot || size > 20) {
            return strtod(buf, nullptr);
        }
        else {
            return strtoll(buf, nullptr, 10);
        }
    }

    inline Json::Value parseString(const char *&data, const char *end)
    {
        const char *buf = data;
        while (data < end)
        {
            if ((unsigned char)(*data) < ' ')
            {
                goto PARSE_STRING_END;
            }

            switch (*data)
            {
            case '\\':
            {
                ++data;

                if (data < end)
                {
                    switch (*data)
                    {
                    case '\"':
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                        ++data;
                        break;

                    case 'u':
                        if (
                            end - data > 4 && ISXDIGIT((int)*(data + 1)) && ISXDIGIT((int)*(data + 2)) && ISXDIGIT((int)*(data + 3)) && ISXDIGIT((int)*(data + 4)))
                        {
                            data += 5;
                        }
                        else
                        {
                            goto PARSE_STRING_END;
                        }
                        break;

                    default:
                        goto PARSE_STRING_END;
                    }
                }
                else
                {
                    goto PARSE_STRING_END;
                }
            }
            break;

            case '\"':
                goto PARSE_STRING_END;

            default:
                ++data;
            }
        }

    PARSE_STRING_END:
        std::string s;
        unescapestringto(s, buf, data - buf);
        if (*data == '\"')
            ++data;

        return std::move(s);
    }

    inline Json::Value parseObject(const char *&data, const char *end)
    {
        Json::Value obj = Json::Value::createObject();
        Json::ObjectContainer *ocp = obj.asObject();
        Json::Value key;
        while (data < end)
        {
            switch (*data)
            {
            case ':':
            case ',':
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                ++data;
                break;

            case '}':
            case ']':
                ++data;
                goto PARSE_OBJECT_END;

            case '\"':
                if (key.type() == Json::Value::Type::UNDEFINED) {
                    key = parseString(++data, end);
                    break;
                }

            default:
                if (key.type() == Json::Value::Type::STRING) {
                    ocp->emplace(
                        key.asString(),
                        parseValue(data, end)
                    );
                    key.reset();
                }
                else {
                    ++data;
                }
                break;
            }
        }
    PARSE_OBJECT_END:
        return obj;
    }

    inline Json::Value parseArray(const char *&data, const char *end)
    {
        Json::Value array = Json::Value::createArray();
        Json::ArrayContainer *acp = array.asArray();
        while (data < end)
        {
            switch (*data)
            {
            case ',':
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                ++data;
                break;

            case ']':
            case '}':
                ++data;
                goto PARSE_ARRAY_END;

            default:
                acp->emplace_back(parseValue(data, end));
            }
        }
    PARSE_ARRAY_END:
        return array;
    }

    inline Json::Value parseValue(const char *&data, const char *end)
    {
        while (data < end)
        {
            switch (*data)
            {
            case '{':
                return parseObject(++data, end);
            case '[':
                return parseArray(++data, end);
            case '\"':
                return parseString(++data, end);

            case 'n':
                if ((end - data >= 4) && strncmp(data, "null", 4) == 0) {
                    data += 4;
                    return {};
                }
                ++data;
                break;

            case 't':
                if ((end - data >= 4) && strncmp(data, "true", 4) == 0) {
                    data += 4;
                    return {true};
                }
                ++data;
                break;

            case 'f':
                if ((end - data >= 5) && strncmp(data, "false", 5) == 0) {
                    data += 5;
                    return {false};
                }
                ++data;
                break;

            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return parseNumber(data, end);

            default:
                ++data;
                break;
            }
        }
        return Json::Value();
    }

    Json::Value parseJson(const char *data, const char *end)
    {
        return parseValue(data, end);
    }

    Json::Value parseJson(const char *data)
    {
        return parseJson(data, data + strlen(data));
    }

    Value parse_file(const char *fileName)
    {
        Value res;
        char *js = 0;
        size_t filesize = 0;

        FILE *f = fopen(fileName, "r");
        if (f == 0)
        {
            return res;
        }

        fseek(f, 0, SEEK_END);
        filesize = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);
        js = (char *)malloc(filesize);
        if (js)
        {
            filesize = fread(js, 1, filesize, f);
            fclose(f);
            res = parseJson(js, js + filesize);
            free(js);
        }
        return res;
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    static std::string &prettyStringifyTo(std::string &res, const Value &v,
                                          size_t level, bool sorted)
    {
        switch (v.type())
        {
        case Value::Type::UNDEFINED:
            res.append("null");
            break;

        case Value::Type::INTEGER:
        case Value::Type::NUMBER:
        case Value::Type::BOOLEAN:
            res.append(v.asString());
            break;

        case Value::Type::STRING:
            res.push_back('\"');
            escapestringto(res, v.asString());
            res.push_back('\"');
            break;

        case Value::Type::ARRAY:
        {
            res.push_back('[');
            res.push_back('\n');

            int i = 0;
            const ArrayContainer &ac = *v.asArray();
            for (auto a = ac.begin(), e = ac.end(); a != e; ++a)
            {
                const Value &val = *a;
                if (i)
                {
                    res.push_back(',');
                    res.push_back('\n');
                }

                res.append((level + 1) * 4, ' ');
                prettyStringifyTo(res, val, level + 1, sorted);

                ++i;
            }
            res.push_back('\n');
            res.append(level * 4, ' ');
            res.push_back(']');
            break;
        }

        case Value::Type::OBJECT:
        {
            res.push_back('{');
            res.push_back('\n');

            if (sorted)
            {
                int i = 0;
                std::vector<std::string> keys = v.indexes();
                std::sort(keys.begin(), keys.end());
                for (const auto &key : keys)
                {
                    const Value &val = v[key];
                    if (i)
                    {
                        res.push_back(',');
                        res.push_back('\n');
                    }

                    res.append((level + 1) * 4, ' ');
                    res.push_back('\"');
                    escapestringto(res, key);
                    res.push_back('\"');
                    res.push_back(':');
                    prettyStringifyTo(res, val, level + 1, sorted);

                    ++i;
                }
                res.push_back('\n');
            }
            else
            {
                int i = 0;
                for (const auto &p : *v.asObject())
                {
                    const Value &val = p.second;
                    if (i)
                    {
                        res.push_back(',');
                        res.push_back('\n');
                    }

                    res.append((level + 1) * 4, ' ');
                    res.push_back('\"');
                    escapestringto(res, p.first);
                    res.push_back('\"');
                    res.push_back(':');
                    prettyStringifyTo(res, val, level + 1, sorted);

                    ++i;
                }
                res.push_back('\n');
            }
            res.append(level * 4, ' ');
            res.push_back('}');
        }
        break;

        default:
            break;
        }
        return res;
    }

    std::string prettyStringify(const Value &v, bool sorted)
    {
        std::string res;
        return prettyStringifyTo(res, v, 0, sorted);
    }

    std::string stringify(const Value &v, bool sorted)
    {
        return prettyStringify(v, sorted);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    std::string &stringifyto(std::string &buff, const Value &v)
    {
        switch (v._type)
        {
        case Value::Type::UNDEFINED:
            buff.append("null");
            break;

        case Value::Type::INTEGER:
            buff.append(numberToString(v._value._i));
            break;

        case Value::Type::NUMBER:
            buff.append(numberToString(v._value._d));
            break;

        case Value::Type::BOOLEAN:
            buff.append(v._value._l ? "true" : "false");
            break;

        case Value::Type::STRING:
            buff.push_back('\"');
            escapestringto(buff, *v._value._s);
            buff.push_back('\"');
            break;

        case Value::Type::ARRAY:
        {
            buff.push_back('[');
            int i = 0;
            for (auto &av : *v.asArray())
            {
                if (i++)
                    buff.push_back(',');
                stringifyto(buff, av);
            }
            buff.push_back(']');
        }
        break;

        case Value::Type::OBJECT:
        {
            buff.push_back('{');
            int i = 0;
            for (auto &p : *v.asObject())
            {
                if (i++)
                    buff.push_back(',');
                buff.push_back('\"');
                escapestringto(buff, p.first);
                buff.push_back('\"');
                buff.push_back(':');

                stringifyto(buff, p.second);
            }
            buff.push_back('}');
        }
        break;

        default:
            break;
        }
        return buff;
    }

} // namespace Json

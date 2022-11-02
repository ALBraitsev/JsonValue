#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <vector>
#include <unordered_map>

namespace Json
{
    class Value;

    typedef std::vector<Value> ArrayContainer;
    typedef std::unordered_map<std::string, Value> ObjectContainer;
    class Value
    {
    public:
        enum class Type
        {
            UNDEFINED,
            BOOLEAN,
            NUMBER,
            INTEGER,
            STRING,
            ARRAY,
            OBJECT
        };

        Value(Type v = Type::UNDEFINED);
        ~Value();

        Value(const Value &v);
        Value(Value &&v) noexcept;
        Value &operator=(const Value &v);
        Value &operator=(Value &&v) noexcept;

        Value(bool v);
        Value(int v);
        Value(size_t v);
        Value(long v);
        Value(long long v);
        Value(double v);
        Value(const char *v);
        Value(const std::string &v);
        Value(std::string &&v);

        template <class T>
        Value(const std::vector<T> &v)
            : _type(Type::ARRAY)
        {
            _value._a = new ArrayContainer(v.begin(), v.end());
        }

        Value(std::initializer_list<std::pair<std::string, Value>> v)
            : _type(Type::OBJECT)
        {
            _value._o = new ObjectContainer(v.begin(), v.end());
        }

        template <class T>
        Value(const std::unordered_map<std::string, T> &v)
            : _type(Type::OBJECT)
        {
            _value._o = new ObjectContainer(v.begin(), v.end());
        }

        static Value createArray();
        static Value createObject();

        Type type() const { return _type; }
        bool isUndefined() const { return _type == Type::UNDEFINED; }
        bool isBoolean() const { return _type == Type::BOOLEAN; }
        bool isNumber() const { return _type == Type::NUMBER || _type == Type::INTEGER; }
        bool isInteger() const { return _type == Type::INTEGER; }
        bool isFloatingPoint() const { return _type == Type::NUMBER; }
        bool isString() const { return _type == Type::STRING; }
        bool isArray() const { return _type == Type::ARRAY; }
        bool isObject() const { return _type == Type::OBJECT; }
        bool isDict() const { return isObject(); }

        bool asBoolean(bool defaultValue = false) const;
        double asNumber(double defaultValue = 0) const;
        long asLong(long defaultValue = 0) const;
        long long asLongLong(long long defaultValue = 0) const;
        int asInt(int defaultValue = 0) const;

        std::string asString(const std::string &defaultValue = "") const;
        const std::string &asConstString(const std::string &defaultValue = "") const;

        std::string asEscapedString(const std::string &defaultValue = "") const;
        bool hasKey(const std::string &str) const;

        Value &operator[](const std::string &key);
        Value &operator[](std::string &&key);
        Value &operator[](size_t key);
        Value &add(const Value &v);
        Value &add(Value &&v);

        const Value &operator[](const std::string &key) const;
        const Value &operator[](size_t key) const;

        size_t size() const;

        /* резервирует память в контейнере */
        void reserve(size_t size);

        /* удаляет элемент с ключом key */
        void erase(const Value &key);

        /* удаляет все элементы контейнера */
        void clear();

        /* возвращает список ключей объекта в виде Value */
        std::vector<std::string> indexes() const;

        bool operator==(const Value &id) const;

        std::string stringifyThis() const;
        std::string prettyStringifyThis() const;

        ObjectContainer *asObject() const;
        ArrayContainer *asArray() const;

        void reset();
        friend std::string &stringifyto(std::string &buff, const Value &v);

    private:
        Type _type;

        union _Value
        {
            ObjectContainer *_o;
            ArrayContainer *_a;
            std::string *_s;

            bool _l;
            long long _i;
            double _d;
        } _value;

        static const Value _emptyValue;
    };

    std::ostream &operator<<(std::ostream &os, const Value &value);

    std::string escapedString(const std::string &s);

    Json::Value parseJson(const char *data, const char *end);
    Json::Value parseJson(const char *data);
    Value parse_file(const char *fileName);

    std::string &stringifyto(std::string &buff, const Value &v);
    std::string stringify(const Value &v, bool sorted = false);
    std::string prettyStringify(const Value &v, bool sorted = false);

    std::string numberToString(double v);
    std::string numberToString(long long v);

} // namespace Json

#endif // VALUE_H

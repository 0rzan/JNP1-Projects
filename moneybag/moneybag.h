#ifndef MONEYBAG_H
#define MONEYBAG_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

class Moneybag {
public:
    using coin_number_t = std::uint64_t;

    constexpr Moneybag(coin_number_t livre, coin_number_t solidus, coin_number_t denier) : 
            livre{ livre },
            solidus{ solidus },
            denier{ denier } {}

    constexpr Moneybag(const Moneybag& m_bag) :
        livre{ m_bag.livre},
        solidus{ m_bag.solidus},
        denier{ m_bag.denier} {}

    constexpr coin_number_t livre_number() const {
        return livre;
    }

    constexpr coin_number_t solidus_number() const {
        return solidus;
    }

    constexpr coin_number_t denier_number() const {
        return denier;
    }

    constexpr Moneybag& operator=(const Moneybag& m_bag) {
        livre = m_bag.livre;
        solidus = m_bag.solidus;
        denier = m_bag.denier;

        return *this;
    }

    constexpr Moneybag& operator+=(const Moneybag& m_bag) {
        if (UINT64_MAX - m_bag.livre < livre || UINT64_MAX - m_bag.solidus < solidus ||
            UINT64_MAX - m_bag.denier < denier) throw std::out_of_range("value can not be too large");

        livre += m_bag.livre;
        solidus += m_bag.solidus;
        denier += m_bag.denier;

        return *this;
    }

    constexpr Moneybag operator+(const Moneybag& m_bag) {
        Moneybag to_return = *this;
        to_return += m_bag;

        return to_return;
    }

    constexpr Moneybag& operator-=(const Moneybag& m_bag) {
        if (livre < m_bag.livre || solidus < m_bag.solidus ||
            denier < m_bag.denier) throw std::out_of_range("value can not be negative");
            
        livre -= m_bag.livre;
        solidus -= m_bag.solidus;
        denier -= m_bag.denier;

        return *this;
    }

    constexpr Moneybag operator-(const Moneybag& m_bag) {
        Moneybag to_return = *this;
        to_return -= m_bag;

        return to_return;
    }

    constexpr Moneybag& operator*=(const Moneybag::coin_number_t num) {
        Moneybag::coin_number_t max_available = Moneybag::coin_number_t(UINT64_MAX) / num;
        if (max_available <  std::max({livre, solidus, denier})) 
            throw std::out_of_range("value can not be too large");
        livre *= num;
        solidus *= num;
        denier *= num;

        return *this;
    }

    constexpr Moneybag operator*(const Moneybag::coin_number_t num) {
        Moneybag to_return = *this;
        to_return *= num;

        return to_return;
    }

    constexpr std::partial_ordering operator<=>(const Moneybag& m_bag) const {
        if (livre == m_bag.livre && solidus == m_bag.solidus &&
            denier == m_bag.denier) return std::partial_ordering::equivalent;
        else if (livre >= m_bag.livre && solidus >= m_bag.solidus &&
            denier >= m_bag.denier) return std::partial_ordering::greater;
        else if (livre <= m_bag.livre && solidus <= m_bag.solidus &&
            denier <= m_bag.denier) return std::partial_ordering::less;
        else return std::partial_ordering::unordered;
    }

    constexpr bool operator==(const Moneybag& m_bag) const {
        return livre == m_bag.livre && solidus == m_bag.solidus &&
            denier == m_bag.denier;
    }

    constexpr explicit operator bool() const {
        if (livre == 0 && solidus == 0 && denier == 0) return false;
        else return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const Moneybag& m_bag) {
        os << "("
            << m_bag.livre;
        if (m_bag.livre == 1) os << " livr, ";
        else os << " livres, ";
        os << m_bag.solidus;
        if (m_bag.solidus == 1) os << " solidus, ";
        else os << " soliduses, ";
        os << m_bag.denier;
        if (m_bag.denier == 1) os << " denier)";
        else os << " deniers)";
        return os;
    }

private:
    coin_number_t livre;
    coin_number_t solidus;
    coin_number_t denier;
};

inline Moneybag operator*(Moneybag::coin_number_t x, Moneybag m_bag) {
    return m_bag * x;
}

constexpr Moneybag Livre(1, 0, 0);
constexpr Moneybag Solidus(0, 1, 0);
constexpr Moneybag Denier(0, 0, 1);

class Value {
public:
    constexpr Value(const Moneybag& m_bag) : 
        Value{m_bag.livre_number()} {
        *this *= 20;
        *this += Value(m_bag.solidus_number());
        *this *= 12;
        *this += Value(m_bag.denier_number());
    }
    constexpr Value(const Moneybag::coin_number_t num) : 
        quotient{ num / Moneybag::MOD },
        rest{ num % Moneybag::MOD } {}

    constexpr Value(const Value& num) :
        quotient{ num.quotient },
        rest{ num.rest } {}

    constexpr Value() :
        quotient{ 0 },
        rest{ 0 } {}

    constexpr std::strong_ordering operator<=>(const Value& value) const {
        if (quotient != value.quotient)
            return quotient <=> value.quotient;
        else
            return rest <=> value.rest;
    }
    
    constexpr std::strong_ordering operator<=>(const uint64_t num) const {
        if (quotient != (num / Moneybag::MOD))
            return quotient <=> (num / Moneybag::MOD);
        else
            return rest <=> (num % Moneybag::MOD);
    }

    constexpr Value& operator=(const Value& num) {
        quotient = num.quotient;
        rest = num.rest;
        return *this;
    }

    constexpr bool operator==(const Value& value) const {
        return quotient == value.quotient && rest == value.rest;
    }

    operator std::string() const {
        if (quotient == 0)
            return std::to_string(rest);
        else {
            std::ostringstream os;
            os << quotient
                << std::setfill('0')
                << std::setw(12)
                << rest;
            return std::move(os).str();
        }
    }
private:
    // Value trzymane jest jako liczba w postaci ilorazu z MOD i reszty z dzielenia z MOD
    uint64_t quotient;
    uint64_t rest;
    static const Moneybag::coin_number_t MOD = 1'000'000'000'000;

    constexpr Value& operator*=(const Moneybag::coin_number_t num) {
        quotient *= num;
        rest *= num;
        quotient += rest / MOD;
        rest %= MOD;
        return *this;
    }

    constexpr Value& operator+=(const Value& value) {
        quotient += value.quotient;
        rest += value.rest;
        quotient += value.rest / MOD;
        rest %= MOD;
        return *this;
    }
};
#endif // MONEYBAG_H